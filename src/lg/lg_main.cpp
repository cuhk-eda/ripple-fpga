#include "pack/clb.h"
#include "alg/bipartite.h"
#include "lg_main.h"
#include "lg_dp_utils.h"

bool ExceedClkrgn(Group& group, vector<Site*>& candSites) {
    if (database.crmap_nx == 0) return false;

    for (auto site : candSites) {
        if (group.InClkBox(site)) return false;
    }

    cout << group.lgBox.lx() << " " << group.lgBox.ly() << " " << group.lgBox.ux() << " " << group.lgBox.uy() << endl;

    return true;
}

bool Legalizer::MergeGroupToSite(Site* site, Group& group, bool fixDspRam) {
    if (site->pack == NULL) {
        database.place(database.addPack(site->type), site->x, site->y);
    }
    auto& type = site->type->name;
    if (type == SiteType::SLICE) {
        lgData.invokeCount++;
        if (lgData.clbMap[site->x][site->y]->AddInsts(group)) {
            lgData.successCount++;
            return true;
        }
    } else if (type == SiteType::DSP || type == SiteType::BRAM) {
        if (database.place(group.instances[0], site, 0)) {
            if (fixDspRam) group.instances[0]->fixed = true;
            return true;
        }
    } else if (type == SiteType::IO) {
        for (int i = 0; i < (int)site->pack->instances.size(); i++) {
            if (database.place(group.instances[0], site, i)) return true;
        }
    }
    return false;
}

bool Legalizer::AssignPackToSite(Site* site, Group& group) {
    if (site->pack == NULL && group.IsTypeMatch(site)) {
        database.place(database.addPack(site->type), site->x, site->y);
        for (unsigned i = 0; i < group.instances.size(); i++) {
            if (group.instances[i] != NULL) {
                database.place(group.instances[i], site, i);
            } else {
                site->pack->instances[i] = NULL;
            }
        }
        return true;
    } else
        return false;
}

void Legalizer::SortCandSitesByHpwl(vector<Site*>& candSites, const Group& group) {
    function<double(Site*)> cal_hpwl = [&](Site* site) {
        double wl = 0;
        double gx = lgData.groupsX[group.id], gy = lgData.groupsY[group.id];

        for (auto net : lgData.group2Nets[group.id]) {
            if (net->isClk) continue;
            const auto& b = lgData.netBox[net->id];
            if (b.size() == 1) continue;

            Box<double> tmpBox(b.x.ou(gx), b.y.ou(gy), b.x.ol(gx), b.y.ol(gy));
            tmpBox.fupdate(site->cx(), site->cy());
            wl += tmpBox.uhp();
        }

        return ((int)(wl * 1000)) / 1000.0;
    };
    ComputeAndSort(candSites, cal_hpwl, less<double>());
    // ComputeAndSort(candSites, cal_hpwl, less<double>(), true);
    // originally, sites are in the order of disp
    // changing to non-stable sort will 3% tot runtime improvement
}

void Legalizer::SortCandSitesByPins(vector<Site*>& candSites, const Group& group) {
    function<int(Site*)> cal_npin = [&](Site* site) { return lgData.nPinPerSite[site->x][site->y]; };
    ComputeAndSort(candSites, cal_npin, less<int>());
}

void Legalizer::SortCandSitesByAlign(vector<Site*>& candSites, const Group& group) {
    function<int(Site*)> cal_align = [&](Site* site) {
        int align = 0;
        for (auto net : lgData.group2Nets[group.id]) {
            if (net->isClk) continue;
            for (auto groupId : lgData.net2Gids[net->id]) {
                if (groupId == group.id) continue;
                int gx = lgData.groupsX[groupId];
                int gy = lgData.groupsY[groupId];
                if (site->x == gx) {
                    align += 1;
                }
                if (site->y == gy) {
                    align += 2;
                    if (SWCol(site->x) == SWCol(gx)) align += 1;
                }
            }
        }
        return align;
    };
    ComputeAndSort(candSites, cal_align, greater<int>());
}

void Legalizer::SortGroupsByGroupsize() {
    function<int(int)> cal_gs = [&](int gid) {
        int size = 0;
        for (auto inst : groups[gid].instances) {
            if (inst == NULL) continue;
            if (inst->IsFF())
                size += 3;
            else if (inst->IsLUT())
                size++;
        }
        return size;
    };
    ComputeAndSort(lgData.groupIds, cal_gs, greater<int>());
}

void Legalizer::SortGroupsByPins() {
    function<int(int)> cal_npin = [&](int gid) { return lgData.group2Nets[gid].size(); };
    ComputeAndSort(lgData.groupIds, cal_npin, greater<int>());
}

void Legalizer::SortGroupsByLGBox() {
    function<int(int)> cal_boxsz = [&](int gid) { return groups[gid].lgBox.x() * groups[gid].lgBox.y(); };
    ComputeAndSort(lgData.groupIds, cal_boxsz, less<int>(), true);
}

void Legalizer::SortGroupsByOptRgnDist() {
    function<double(int)> cal_ordist = [&](int gid) {
        double lx, hx, ly, hy;
        vector<double> boxX, boxY;
        for (auto net : lgData.group2Nets[gid]) {
            if (net->isClk) continue;
            const auto& b = lgData.netBox[net->id];
            if (b.size() == 1) continue;
            boxX.push_back(b.x.ol(groups[gid].x));
            boxX.push_back(b.x.ou(groups[gid].x));
            boxY.push_back(b.y.ol(groups[gid].y));
            boxY.push_back(b.y.ou(groups[gid].y));
        }
        GetMedianTwo(boxX, lx, hx);
        GetMedianTwo(boxY, ly, hy);

        Box<double> optrgn(hx, hy, lx, ly);

        return optrgn.udist(groups[gid].x, groups[gid].y);
    };
    ComputeAndSort(lgData.groupIds, cal_ordist, greater<int>());
}

Legalizer::Legalizer(vector<Group>& _groups) : groups(_groups), lgData(_groups) {}

void Legalizer::Init(lgPackMethod packMethod) { lgData.Init(packMethod); }

void Legalizer::GetResult(lgRetrunGroup retGroup) {
    lgData.GetDispStatics();
    lgData.GetResult(retGroup);
    lgData.GetPackStatics();
}

bool Legalizer::RunAll(lgSiteOrder siteOrder, lgGroupOrder groupOrder) {
    const int MAX_WIN = 1000;

    // SortGroupsByPins();
    // SortGroupsByGroupsize(); //better for overlap reduction
    // SortGroupsByOptRgnDist(); //better for hpwl reduction
    if (groupOrder == GROUP_LGBOX) {
        printlog(LOG_INFO, "group order: GROUP_LGBOX");
        SortGroupsByLGBox();
    } else {
        printlog(LOG_INFO, "group order: DEFAULT");
    }

    int nChain = 0, chainLen = 0, nSucc = 0;

    for (const auto gid : lgData.groupIds) {
        Group& group = groups[gid];

        bool isFixed = false;
        for (auto inst : group.instances) {
            if (inst != NULL && inst->fixed) {
                isFixed = true;
                break;
            }
        }
        if (isFixed) {
            lgData.placedGroupMap[lgData.groupsX[gid]][lgData.groupsY[gid]].push_back(group.id);
            continue;
        }

        int winWidth = 0;
        bool isPlaced = false;
        Site* curSite = NULL;

        while (!isPlaced) {
            vector<Site*> candSites;
            if (siteOrder != SITE_HPWL_SMALL_WIN) {
                while (candSites.size() < 200) {
                    GetWinElem(candSites, database.sites, {lgData.groupsX[gid], lgData.groupsY[gid]}, winWidth);
                    if (candSites.size() == 0) break;
                    winWidth++;
                }
            } else {
                GetWinElem(candSites, database.sites, {lgData.groupsX[gid], lgData.groupsY[gid]}, winWidth);
                winWidth++;
            }

            if (ExceedClkrgn(group, candSites) || winWidth >= MAX_WIN || candSites.size() == 0) {
                printlog(LOG_WARN, "no candSites available");
                break;
            }

            SqueezeCandSites(candSites, group, !group.IsBLE());

            if (siteOrder == SITE_HPWL || siteOrder == SITE_HPWL_SMALL_WIN)
                SortCandSitesByHpwl(candSites, group);
            else if (siteOrder == SITE_ALIGN)
                SortCandSitesByAlign(candSites, group);

            for (unsigned s = 0; !isPlaced && s < candSites.size(); s++) {
                curSite = candSites[s];
                if (MergeGroupToSite(curSite, group, false)) {
                    isPlaced = true;
                    lgData.PartialUpdate(group, curSite);

                    double disp = abs((int)group.x - (int)lgData.groupsX[group.id]) * 0.5 +
                                  abs((int)group.y - (int)lgData.groupsY[group.id]);
                    if (disp != 0 && database.crmap_nx != 0) {
                        int len = ChainMove(group, DISP_OPT);
                        if (len != -1) {
                            nSucc++;
                            chainLen += len;
                        }
                        nChain++;
                    }

                    disp = abs((int)group.x - (int)lgData.groupsX[group.id]) * 0.5 +
                           abs((int)group.y - (int)lgData.groupsY[group.id]);
                    if (disp >= 2 && database.crmap_nx != 0) {
                        int len = ChainMove(group, MAX_DISP_OPT);
                        if (len != -1) {
                            nSucc++;
                            chainLen += len;
                        }
                        nChain++;
                    }
                }
            }
        }

        if (!isPlaced) return false;
    }

    printlog(LOG_INFO,
             "chain move: avgLen=%.2f, #fail=%d, #success=%d(%.2f%%)",
             chainLen * 1.0 / nSucc,
             nChain - nSucc,
             nSucc,
             nSucc * 100.0 / nChain);

    return true;
}

void Legalizer::RunPartial() {
    for (auto type : {SiteType::DSP, SiteType::BRAM}) BipartiteLeg(type, 10);
}

void Legalizer::BipartiteLeg(SiteType::Name type, int minNumSites) {
    assert(minNumSites <= 100);
    vector<Group*> cells;
    for (auto& g : groups) {
        if (g.GetSiteType() == type) {
            cells.push_back(&g);
            for (auto inst : g.instances) {
                database.unplace(inst);
            }
        }
    }
    database.ClearEmptyPack();

    std::unordered_map<Site*, int> siteIds;
    vector<Site*> sites;
    vector<vector<pair<int, long>>> allCandSites(cells.size());  // (site, score) TODO: secondary obj disp
    // add safe sites
    for (size_t cid = 0; cid < cells.size(); ++cid) {
        auto& cell = *cells[cid];
        int winWidth = 0;
        bool found = false;
        for (; !found; ++winWidth) {
            vector<Site*> candSites;
            GetWinElem(candSites, database.sites, {lgData.groupsX[cell.id], lgData.groupsY[cell.id]}, winWidth);
            if (ExceedClkrgn(cell, candSites)) {
                printlog(LOG_ERROR, "BipartiteLeg: cannot find safe sites");
                exit(1);
            }
            SqueezeCandSites(candSites, cell, true);
            SortCandSitesByHpwl(candSites, cell);
            for (auto site : candSites) {
                auto it = siteIds.find(site);
                if (it == siteIds.end()) {
                    int sid = siteIds.size();
                    siteIds[site] = sid;
                    sites.push_back(site);
                    allCandSites[cid].push_back({sid, -1});
                    found = true;
                    break;
                }
            }
        }
    }
    // add more sites
    for (size_t cid = 0; cid < cells.size(); ++cid) {
        auto& cell = *cells[cid];
        vector<Site*> candSites;
        for (int winWidth = 0; (int)candSites.size() < minNumSites; ++winWidth) {
            GetWinElem(candSites, database.sites, {lgData.groupsX[cell.id], lgData.groupsY[cell.id]}, winWidth);
            SqueezeCandSites(candSites, cell, true);
        }
        for (auto site : candSites) {
            int sid;
            auto it = siteIds.find(site);
            if (it == siteIds.end()) {
                sid = siteIds.size();
                siteIds[site] = sid;
                sites.push_back(site);
            } else
                sid = it->second;
            allCandSites[cid].push_back({sid, -1});
        }
    }
    // cal wl
    for (size_t cid = 0; cid < cells.size(); ++cid) {
        auto& cell = *cells[cid];
        // cell.print();
        for (auto& site_wl : allCandSites[cid]) {
            auto site = sites[site_wl.first];
            double wl = 0;
            for (auto net : lgData.group2Nets[cell.id]) {
                const auto& b = lgData.netBox[net->id];
                Box<double> tmpBox(b.x.ou(lgData.groupsX[cell.id]),
                                   b.y.ou(lgData.groupsY[cell.id]),
                                   b.x.ol(lgData.groupsX[cell.id]),
                                   b.y.ol(lgData.groupsY[cell.id]));
                tmpBox.fupdate(site->cx(), site->cy());
                wl += tmpBox.uhp();
            }
            site_wl.second = wl * 100;
            // cout << site->x << " " << site->y << " " << wl << endl;
        }
    }

    vector<pair<int, long>> res;
    long cost = 0;
    MinCostBipartiteMatching(allCandSites, allCandSites.size(), sites.size(), res, cost);

    for (size_t cid = 0; cid < cells.size(); ++cid) {
        if (res[cid].first < 0)
            printlog(LOG_ERROR, "BipartiteLeg: cannot find ...");
        else {
            auto site = sites[res[cid].first];
            MergeGroupToSite(site, *cells[cid]);
            lgData.PartialUpdate(*cells[cid], site);
        }
    }
}

double Legalizer::GetHpwl() { return lgData.GetHpwl(); }