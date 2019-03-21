#include "./dp_main.h"
#include "alg/bipartite.h"
#include "lg/lg_dp_utils.h"

// GlobalPackSwap
inline void DPPack::SwapPack(Site* curSite, Site* candSite) {
    Pack* curPack = curSite->pack;

    candSite->pack->site = curSite;
    curSite->pack = candSite->pack;

    curPack->site = candSite;
    candSite->pack = curPack;
}

double DPPack::TrySwap(Site* curSite, Site* candSite) {
    set<Net*> nets;
    for (auto net : packToNets[curSite->pack->id]) {
        if (!net->isClk) nets.insert(net);
    }
    for (auto net : packToNets[candSite->pack->id]) {
        if (!net->isClk) nets.insert(net);
    }

    double prevHpwl = 0;
    for (auto net : nets) prevHpwl += netBox[net->id].uhp();

    double curHpwl = 0;
    PartialUpdate(curSite->pack, candSite, false);
    PartialUpdate(candSite->pack, curSite, false);
    SwapPack(curSite, candSite);

    for (auto net : nets) curHpwl += netBox[net->id].uhp();

    PartialUpdate(curSite->pack, candSite, false);
    PartialUpdate(candSite->pack, curSite, false);
    SwapPack(curSite, candSite);

    return curHpwl - prevHpwl;
}

void DPPack::GlobalPackSwap(SiteType::Name type) {
    double befHpwl = GetHpwl();

    int nNullSwap = 0;
    int nPackSwap = 0;
    int nInOptiRgn = 0;

    for (auto pack : database.packs) {
        if (pack->type->name != type) continue;

        Site* curSite = pack->site;
        Box<double> optrgn = GetOptimalRegion({curSite->cx(), curSite->cy()}, packToNets[pack->id], netBox);
        if ((int)optrgn.dist(curSite->cx(), curSite->cy()) == 0) {
            nInOptiRgn++;
            continue;
        }

        Box<int> box;
        bool isSet = false;
        for (auto& gid : dpData.groupMap[curSite->x][curSite->y]) {
            if (!isSet) {
                isSet = true;
                box.set(groups[gid].lgBox.ux(), groups[gid].lgBox.uy(), groups[gid].lgBox.lx(), groups[gid].lgBox.ly());
            } else {
                int box_lx = max(groups[gid].lgBox.lx(), box.lx());
                int box_ly = max(groups[gid].lgBox.ly(), box.ly());
                int box_hx = min(groups[gid].lgBox.ux(), box.ux());
                int box_hy = min(groups[gid].lgBox.uy(), box.uy());
                box.set(box_hx, box_hy, box_lx, box_ly);
            }
        }

        bool swapped = false;
        vector<Site*> candSites;
        for (int x = optrgn.lx(); x <= (int)optrgn.ux(); x++) {
            for (int y = optrgn.ly(); y <= (int)optrgn.uy(); y++) {
                auto& candSite = database.sites[x][y];
                if (candSite->type == curSite->type && box.dist(candSite->x, candSite->y) == 0 &&
                    !dpData.WorsenCong(curSite, candSite))
                    candSites.push_back(candSite);
            }
        }
        // search for null place first
        for (unsigned s = 0; !swapped && s < candSites.size(); s++) {
            Site* candSite = candSites[s];
            if (candSite->pack == NULL && dpData.IsClkMoveLeg(curSite->pack, candSite)) {
                PartialUpdate(curSite->pack, candSite);
                swap(dpData.groupMap[candSite->x][candSite->y], dpData.groupMap[curSite->x][curSite->y]);
                swap(dpData.clbMap[candSite->x][candSite->y], dpData.clbMap[curSite->x][curSite->y]);
                UpdateGroupXY(candSite);
                database.place(curSite->pack, candSite);
                swapped = true;
                nNullSwap++;
            }
        }
        if (!swapped) {
            // search for sites in optimal region
            Site* selectSite = NULL;
            double minHpwl = DBL_MAX;
            for (unsigned s = 0; s < candSites.size(); s++) {
                Site* candSite = candSites[s];
                if (candSite->pack != NULL &&
                    dpData.IsClkMoveLeg({{curSite->pack, candSite}, {candSite->pack, curSite}})) {
                    double deltaHpwl = TrySwap(curSite, candSite);
                    if (minHpwl > deltaHpwl) {
                        minHpwl = deltaHpwl;
                        selectSite = candSite;
                    }
                }
            }
            if (minHpwl <= 0 && selectSite != NULL) {
                PartialUpdate(curSite->pack, selectSite);
                PartialUpdate(selectSite->pack, curSite);
                swap(dpData.groupMap[selectSite->x][selectSite->y], dpData.groupMap[curSite->x][curSite->y]);
                swap(dpData.clbMap[selectSite->x][selectSite->y], dpData.clbMap[curSite->x][curSite->y]);
                UpdateGroupXY(curSite);
                UpdateGroupXY(selectSite);
                SwapPack(curSite, selectSite);
                nPackSwap++;
            }
        }
    }

    double aftHpwl = GetHpwl();
    double improveRatio = (befHpwl - aftHpwl) * 100.0 / befHpwl;
    printlog(LOG_INFO,
             "#null_swap=%d, #pack_swap=%d, #in_OptiRgn=%d, hpwl_impr_rate=%.3lf%%",
             nNullSwap,
             nPackSwap,
             nInOptiRgn,
             improveRatio);
}

// BipartiteDP
void DPPack::UpdateDpData(const vector<pair<Pack*, Site*>>& pairs, SiteType::Name type) {
    for (auto& p : pairs) {
        PartialUpdate(p.first, p.second);
    }

    // for (auto& p : pairs){
    //     for (auto gid:dpData.groupMap[p.first->site->cx()][p.first->site->cy()]){
    //         groups[gid].x=p.second->cx();
    //         groups[gid].y=p.second->cy();
    //     }
    // }
    // update groupMap
    auto groupMapTmp = dpData.groupMap;
    for (auto& p : pairs) {
        int x = p.first->site->cx(), y = p.first->site->cy();
        dpData.groupMap[x][y].clear();
    }
    for (auto& p : pairs) {
        int x = p.first->site->cx(), y = p.first->site->cy();
        int newX = p.second->cx(), newY = p.second->cy();
        dpData.groupMap[newX][newY] = move(groupMapTmp[x][y]);
    }
    // update clbMap
    if (type == SiteType::SLICE) {
        auto& clbMap = dpData.clbMap;
        vector<vector<CLBBase*>> clbMapTmp(database.sitemap_nx, vector<CLBBase*>(database.sitemap_ny, NULL));
        // int cnt=0;
        for (auto& p : pairs) {
            int x = p.first->site->x, y = p.first->site->y;
            clbMapTmp[x][y] = clbMap[x][y];  // collect the clbs that will move out
            clbMap[x][y] = NULL;
        }
        for (auto& p : pairs) {
            int x = p.first->site->x, y = p.first->site->y;
            int newX = p.second->x, newY = p.second->y;
            if (clbMap[newX][newY] != NULL) {
                assert(clbMap[newX][newY]->IsEmpty());
                delete clbMap[newX][newY];
                // ++cnt;
            }
            clbMap[newX][newY] = clbMapTmp[x][y];
        }
        for (auto& p : pairs) {
            int x = p.first->site->x, y = p.first->site->y;
            if (clbMap[x][y] == NULL) {  // no one moves in here
                clbMap[x][y] = dpData.NewCLB();
                // --cnt;
            }
        }
        // cout << cnt << endl;
    }
}

void DPPack::ClkAndCongFilter(Pack* pack, vector<Site*>& candSites) {
    vector<Site*> sites;
    for (auto site : candSites) {
        if (dpData.IsClkMoveLeg(pack, site) && (!dpData.WorsenCong(pack->site, site) || site == pack->site)) {
            sites.push_back(site);
        }
    }
    candSites = move(sites);
}

void DPPack::BipartiteDP(SiteType::Name type, int minNumSites) {
    // UpdatePackInfo();
    double befHpwl = GetHpwl();
    int nMove = 0;

    // get all sites & packs of this type
    std::unordered_map<Site*, int> siteIds;
    vector<Site*> sites;
    vector<Pack*> packs;
    for (auto& ss : database.sites)
        for (auto s : ss)
            if (s->type->name == type && siteIds.find(s) == siteIds.end()) {
                siteIds[s] = sites.size();
                sites.push_back(s);
                if (s->pack != NULL) packs.push_back(s->pack);
            }
    // get sites for each pack
    vector<vector<pair<int, long>>> allCandSites(packs.size());  // (site, score) TODO: secondary obj disp
    for (size_t pid = 0; pid < packs.size(); ++pid) {
        auto& pack = *packs[pid];
        vector<Site*> candSites;
        for (int winWidth = 0; (int)candSites.size() < minNumSites; ++winWidth) {
            GetWinElem(candSites, database.sites, {pack.site->cx(), pack.site->cy()}, winWidth);
            SqueezeCandSites(candSites, type);
            ClkAndCongFilter(packs[pid], candSites);
        }
        for (auto site : candSites) {
            int sid = siteIds[site];
            allCandSites[pid].push_back({sid, -1});
        }
    }
    // cal wl
    // double oriTot = 0;
    for (size_t pid = 0; pid < packs.size(); ++pid) {
        auto& pack = *packs[pid];
        double curX = pack.site->cx(), curY = pack.site->cy();
        auto& nets = packToNets[pack.id];
        for (auto& site_wl : allCandSites[pid]) {
            auto site = sites[site_wl.first];
            double wl = 0;
            for (auto net : nets) {
                const auto& b = netBox[net->id];
                if (b.size() == 1) continue;
                Box<double> tmpBox(b.x.ou(curX), b.y.ou(curY), b.x.ol(curX), b.y.ol(curY));
                tmpBox.fupdate(site->cx(), site->cy());
                wl += tmpBox.uhp();
            }
            site_wl.second = wl * 100;
            // if (site == pack.site) oriTot += wl;
            // cout << site->cx() << " " << site->cy() << " " << wl << endl;
        }
    }
    // cout << oriTot << endl;

    // solve
    vector<pair<int, long>> res;
    long cost = 0;
    MinCostBipartiteMatching(allCandSites, allCandSites.size(), sites.size(), res, cost);
    // cout << cost / 100.0 << endl;
    vector<pair<Pack*, Site*>> pairs;
    for (size_t pid = 0; pid < packs.size(); ++pid) {
        if (res[pid].first < 0) {
            printlog(LOG_ERROR, "BipartiteLeg: cannot find ...");
            pairs.clear();
            break;
        } else {
            auto pack = packs[pid];
            auto site = sites[res[pid].first];
            if (pack->site != site) {
                ++nMove;
                pairs.push_back({pack, site});
                // cout << pack->site->cx() << " " << pack->site->cy() << " moves to " << site->cx() << " " <<
                // site->cy() << endl;
            }
        }
    }

    if (dpData.IsClkMoveLeg(pairs)) {
        UpdateDpData(pairs, type);

        // place packs
        for (auto& p : pairs) database.unplace(p.first);
        for (auto& p : pairs) {
            database.place(p.first, p.second);
            UpdateGroupXY(p.second);
        }
        database.ClearEmptyPack();

        double aftHpwl = GetHpwl();
        double improveRatio = (befHpwl - aftHpwl) * 100.0 / befHpwl;
        printlog(LOG_INFO,
                 "BipartiteDP of %s, #move=%d, #pack=%d, #site=%d, hpwl_impr_rate=%.3lf%%",
                 SiteType::NameEnum2String(type).c_str(),
                 nMove,
                 (int)packs.size(),
                 (int)sites.size(),
                 improveRatio);
    } else {
        printlog(LOG_INFO, "BipartiteDP of %s failed", SiteType::NameEnum2String(type).c_str());
    }
}

DPPack::DPPack(vector<Group>& _groups, DPData& _dpData) : groups(_groups), dpData(_dpData) {}

double DPPack::GetHpwl() {
    double r = 0;
    for (auto& b : netBox) r += b.uhp();
    return r;
}

void DPPack::UpdateGroupXY(Site* site) {
    double x = site->cx(), y = site->cy();
    for (auto& g : dpData.groupMap[x][y]) {
        groups[g].x = x;
        groups[g].y = y;
    }
}

void DPPack::PartialUpdate(Pack* curPack, Site* targetSite, bool updateClk) {
    for (auto net : packToNets[curPack->id]) {
        auto site = curPack->site;
        netBox[net->id].erase(site->cx(), site->cy());
        netBox[net->id].insert(targetSite->cx(), targetSite->cy());
    }

    if (updateClk) {
        vector<int> gid2Move = dpData.groupMap[curPack->site->cx()][curPack->site->cy()];
        vector<Site*> sites(gid2Move.size(), targetSite);
        dpData.colrgnData.Update(gid2Move, sites, groups);
    }
}

void DPPack::UpdatePackInfo() {
    // update nets bounding box info with fixed IOs
    netBox.assign(database.nets.size(), {});
    for (auto net : database.nets) {
        for (auto pin : net->pins) {
            auto inst = pin->instance;
            if (inst != NULL && inst->IsIO() && inst->fixed) {
                Site* site = inst->pack->site;
                netBox[net->id].insert(site->cx(), site->cy());
                // netBox[net->id].insert(site->cx(), database.getIOY(inst));
            }
        }
    }

    // pack (excluding fixed IOs) / net mapping
    netToPacks.assign(database.nets.size(), {});
    packToNets.assign(database.packs.size(), {});
    for (auto net : database.nets) {
        unordered_set<Pack*> packs;
        for (auto pin : net->pins) {
            auto inst = pin->instance;
            if (inst != NULL && !(inst->IsIO() && inst->fixed)) packs.insert(inst->pack);
        }
        for (auto pack : packs) {
            netToPacks[net->id].push_back(pack);
            packToNets[pack->id].push_back(net);
        }
    }

    // update nets bounding box info with others
    for (auto net : database.nets) {
        for (auto pack : netToPacks[net->id]) {
            Site* site = pack->site;
            netBox[net->id].insert(site->cx(), site->cy());
        }
    }
}

void DPPack::Run() {
    UpdatePackInfo();
    GlobalPackSwap(SiteType::SLICE);
    for (auto type : {SiteType::DSP, SiteType::BRAM}) BipartiteDP(type, 10);
}