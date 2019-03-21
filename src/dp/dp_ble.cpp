#include "./dp_main.h"
#include "alg/bipartite.h"
#include "lg/lg_dp_utils.h"

void DPBle::SortCandSitesByAlign(Group& group, vector<Site*>& candSites) {
    function<int(Site*)> cal_align = [&](Site* site) {
        int align = 0;
        for (auto net : groupToNets[group.id]) {
            if (net->isClk || net->pins.size() > 50) continue;
            for (auto gid : netToGids[net->id]) {
                if (gid == group.id) continue;
                int gx = groups[gid].x;
                int gy = groups[gid].y;
                if (site->y == gy) {
                    align += 2;
                    if (SWCol(site->x) == SWCol(gx)) align += 1;
                    if (site->x == gx) align += 1;
                } else {
                    if (SWCol(site->x) == SWCol(gx)) align += 1;
                }
            }
        }
        return align;
    };
    ComputeAndSort(candSites, cal_align, greater<int>());
}

void DPBle::SortCandSitesByDisp(Group& group, vector<Site*>& candSites) {
    function<int(Site*)> cal_disp = [&](Site* site) {
        return abs(group.x - site->cx()) * 0.5 + abs(group.y - site->cy());
    };
    ComputeAndSort(candSites, cal_disp, less<int>());
}

bool DPBle::IsGroupMovable(Group& group, CLBBase*& sourceCLB, vector<int>& sourceGroup) {
    if (group.GetSiteType() == SiteType::SLICE) {
        Site* site = database.getSite(group.x, group.y);
        for (unsigned g = 0; g < dpData.groupMap[site->x][site->y].size(); g++) {
            Group& placedGroup = groups[dpData.groupMap[site->x][site->y][g]];
            if (placedGroup.id != group.id) {
                if (!sourceCLB->AddInsts(placedGroup)) {
                    return false;
                }
                sourceGroup.push_back(placedGroup.id);
            }
        }
    }
    return true;
}

bool DPBle::MoveGroupToSite(Site* site, Group& group, CLBBase*& sourceCLB, vector<int>& sourceGroup) {
    if (group.x == site->cx() && group.y == site->cy()) return true;

    if (!dpData.clbMap[site->x][site->y]->AddInsts(group)) return false;

    dpData.groupMap[group.x][group.y] = sourceGroup;
    dpData.groupMap[site->x][site->y].push_back(group.id);

    delete dpData.clbMap[group.x][group.y];
    dpData.clbMap[group.x][group.y] = sourceCLB;

    sourceCLB = NULL;

    return true;
}

void DPBle::DumpToDB() {
    // unplace
    for (auto& g : groups) {
        if (g.GetSiteType() == SiteType::SLICE) {
            for (auto inst : g.instances) {
                if (inst) database.unplace(inst);
            }
        }
    }
    // place
    for (int x = 0; x < database.sitemap_nx; ++x) {
        for (int y = 0; y < database.sitemap_ny; ++y) {
            auto site = database.getSite(x, y);
            if (site->type->name != SiteType::SLICE) continue;
            if (!site->pack) database.place(database.addPack(site->type), x, y);
            Group res;
            dpData.clbMap[x][y]->GetResult(res);
            for (unsigned i = 0; i < res.instances.size(); i++) {
                if (res.instances[i] != NULL) {
                    database.place(res.instances[i], site, i);
                }
            }
        }
    }
}

Site* DPBle::MoveTowardOptrgn(Group& group, Box<double>& optrgn, CLBBase*& sourceCLB, vector<int>& sourceGroup) {
    Site* curSite = database.getSite(group.x, group.y);
    double dist = optrgn.udist(curSite->cx(), curSite->cy());

    int lx = optrgn.lx(), ly = optrgn.ly(), hx = optrgn.ux(), hy = optrgn.uy();
    int nx = database.sitemap_nx, ny = database.sitemap_ny;

    int winWidth = 1;
    while (winWidth < dist * 2) {
        vector<Site*> candSites;

        // left, right
        for (int x = lx - winWidth, y = ly; x >= 0 && y < hy; y++) {
            candSites.push_back(database.getSite(x, y));
        }

        for (int x = hx - 1 + winWidth, y = ly; x < nx && y < hy; y++) {
            candSites.push_back(database.getSite(x, y));
        }
        // up, bottom
        if (winWidth % 2 == 0) {
            for (int x = lx, y = ly - winWidth / 2; y >= 0 && x < hx; x++) {
                candSites.push_back(database.getSite(x, y));
            }
            for (int x = lx, y = hy - 1 + winWidth / 2; y < ny && x < hx; x++) {
                candSites.push_back(database.getSite(x, y));
            }
        }
        // diagonal
        for (int x = lx - (winWidth - 2), y = ly - 1; x >= 0 && y >= 0 && x < lx; x += 2, y--) {
            candSites.push_back(database.getSite(x, y));
        }
        for (int x = hx - 1 + (winWidth - 2), y = ly - 1; x < nx && y >= 0 && x >= hx; x -= 2, y--) {
            candSites.push_back(database.getSite(x, y));
        }
        for (int x = hx - 1 + (winWidth - 2), y = hy; x < nx && y < nx && x >= hx; x -= 2, y++) {
            candSites.push_back(database.getSite(x, y));
        }
        for (int x = lx - (winWidth - 2), y = hy; x >= 0 && y < nx && x < lx; x += 2, y++) {
            candSites.push_back(database.getSite(x, y));
        }

        SqueezeCandSites(candSites, group);
        SortCandSitesByDisp(group, candSites);

        for (auto site : candSites) {
            if (dpData.IsClkMoveLeg(group, site) && MoveGroupToSite(site, group, sourceCLB, sourceGroup)) {
                return site;
            }
        }

        winWidth++;
    }

    return NULL;
}

void DPBle::GlobalEleMove() {
    double befHpwl = GetHpwl();

    int nSliceGroup = 0;

    int nToOptNullMove = 0;
    int nToOptMergeMove = 0;
    int nTowardOptMove = 0;
    int nChainMove = 0;

    int nNotInOptiRgn = 0;
    int nInOptiRgn = 0;

    for (auto& group : groups) {
        if (group.GetSiteType() == SiteType::SLICE) {
            nSliceGroup++;
            Site* curSite = database.getSite(group.x, group.y);

            CLBBase* sourceCLB = dpData.NewCLB();
            vector<int> sourceGroup;

            Box<double> optrgn = GetOptimalRegion({group.x, group.y}, groupToNets[group.id], netBox);
            bool inOptrgn = ((int)optrgn.dist(curSite->cx(), curSite->cy()) == 0);

            if (!inOptrgn) {
                nNotInOptiRgn++;
                IsGroupMovable(group, sourceCLB, sourceGroup);

                bool moved = false;
                Site* candSite = NULL;
                vector<Site*> candSites;
                for (int x = optrgn.lx(); x <= (int)optrgn.ux(); x++) {
                    for (int y = optrgn.ly(); y <= (int)optrgn.uy(); y++) {
                        auto candSite = database.sites[x][y];
                        if (candSite->type == curSite->type && dpData.IsClkMoveLeg(group, candSite) &&
                            !dpData.WorsenCong(curSite, candSite)) {
                            candSites.push_back(candSite);
                        }
                    }
                }
                SortCandSitesByAlign(group, candSites);
                // search for null place first
                for (unsigned s = 0; s < candSites.size(); s++) {
                    candSite = candSites[s];
                    if (candSite->pack == NULL) {  // NOTE: empty before the whole process (routing friendly..)
                                                   // if (dpData.clbMap[candSite->x][candSite->y]->IsEmpty()){
                        moved = MoveGroupToSite(candSite, group, sourceCLB, sourceGroup);
                        if (moved) {
                            nToOptNullMove++;
                            break;
                        }
                    }
                }
                if (!moved) {
                    // try merge to sites in optimal region
                    for (unsigned s = 0; s < candSites.size(); s++) {
                        candSite = candSites[s];
                        if (candSite->pack != NULL) {
                            moved = MoveGroupToSite(candSite, group, sourceCLB, sourceGroup);
                            if (moved) {
                                nToOptMergeMove++;
                                break;
                            }
                        }
                    }
                }

                if (!moved) {
                    candSite = MoveTowardOptrgn(group, optrgn, sourceCLB, sourceGroup);
                    if (candSite) {
                        moved = true;
                        nTowardOptMove++;
                    }
                }

                if (moved) {
                    PartialUpdate(group, candSite);
                } else {
                    nChainMove += OptrgnChainMove(group);
                }
            }
            if (sourceCLB != NULL) delete sourceCLB;
        }
    }

    int nOptNullMove = 0;
    int nOptMergeMove = 0;

    for (auto& group : groups) {
        if (group.GetSiteType() == SiteType::SLICE) {
            Site* curSite = database.getSite(group.x, group.y);

            CLBBase* sourceCLB = dpData.NewCLB();
            vector<int> sourceGroup;

            Box<double> optrgn = GetOptimalRegion({group.x, group.y}, groupToNets[group.id], netBox);
            bool inOptrgn = ((int)optrgn.dist(curSite->cx(), curSite->cy()) == 0);
            if (inOptrgn) {
                nInOptiRgn++;
                IsGroupMovable(group, sourceCLB, sourceGroup);

                bool moved = false;
                Site* candSite = NULL;
                vector<Site*> candSites;
                for (int x = optrgn.lx(); x <= (int)optrgn.ux(); x++) {
                    for (int y = optrgn.ly(); y <= (int)optrgn.uy(); y++) {
                        auto candSite = database.sites[x][y];
                        if (candSite->type == curSite->type && dpData.IsClkMoveLeg(group, candSite) &&
                            !dpData.WorsenCong(curSite, candSite)) {
                            candSites.push_back(candSite);
                        }
                    }
                }
                SortCandSitesByAlign(group, candSites);
                // search for null place first
                for (unsigned s = 0; s < candSites.size(); s++) {
                    candSite = candSites[s];
                    if (candSite->pack == NULL) {  // NOTE: empty before the whole process (routing friendly..)
                                                   // if (dpData.clbMap[candSite->x][candSite->y]->IsEmpty()){
                        moved = MoveGroupToSite(candSite, group, sourceCLB, sourceGroup);
                        if (moved) {
                            nToOptNullMove += (!inOptrgn);
                            nOptNullMove += inOptrgn;
                            break;
                        }
                    }
                }
                if (!moved) {
                    // try merge to sites in optimal region
                    for (unsigned s = 0; s < candSites.size(); s++) {
                        candSite = candSites[s];
                        if (candSite->pack != NULL) {
                            moved = MoveGroupToSite(candSite, group, sourceCLB, sourceGroup);
                            if (moved) {
                                nToOptMergeMove += (!inOptrgn);
                                nOptMergeMove += inOptrgn;
                                break;
                            }
                        }
                    }
                }

                if (moved) {
                    nOptMergeMove -= (candSite == curSite);
                    PartialUpdate(group, candSite);
                }
            }
            if (sourceCLB != NULL) delete sourceCLB;
        }
    }

    DumpToDB();
    database.ClearEmptyPack();
    double aftHpwl = GetHpwl();
    double improveRatio = (befHpwl - aftHpwl) * 100.0 / befHpwl;
    int nNullMove = nOptNullMove + nToOptNullMove;
    int nMergeMove = nOptMergeMove + nToOptMergeMove + nTowardOptMove;
    printlog(LOG_INFO,
             "#null_move=%d, #merge_move=%d, #cells=%d, hpwl_impr_rate=%.3lf%%",
             nNullMove,
             nMergeMove,
             nSliceGroup,
             improveRatio);
    printlog(LOG_INFO,
             " | out_optrgn: ToOptNullMove=%d, ToOptMergeMove=%d, TowardOptMove=%d, ChainMove=%d, #OutOptRgn=%d",
             nToOptNullMove,
             nToOptMergeMove,
             nTowardOptMove,
             nChainMove,
             nNotInOptiRgn);
    printlog(LOG_INFO,
             " | in_optrgn: OptNullMove=%d, OptMergeMove=%d, #OutOptRgn=%d",
             nOptNullMove,
             nOptMergeMove,
             nSliceGroup - nInOptiRgn);
}

Site* GetHashedSite(int val) { return database.getSite(val / database.sitemap_ny, val % database.sitemap_ny); }

int HashSite(Site* site) { return site->cx() * database.sitemap_ny + site->cy(); }

void DPBle::GlobalEleSwap() {
    double begHpwl = GetHpwl();
    double begCost = 0, aftCost = 0;
    int nMove = 0, nCells = 0;

    vector<int> ffGroups;

    for (auto& group : groups) {
        if (group.IsBLE()) {
            bool hasFFOnly = true;
            for (auto inst : group.instances) {
                if (inst->IsLUT()) {
                    hasFFOnly = false;
                    break;
                }
            }
            if (hasFFOnly) {
                Box<double> optrgn = GetOptimalRegion({group.x, group.y}, groupToNets[group.id], netBox);
                if (optrgn.dist(group.x, group.y) > 0) {
                    ffGroups.push_back(group.id);
                }
            }
        }
    }

    map<tuple<Net*, Net*, Net*>, vector<int>> ffType;

    for (auto gid : ffGroups) {
        Instance* inst = groups[gid].instances[0];
        ffType[tuple<Net*, Net*, Net*>(inst->pins[2]->net, inst->pins[3]->net, inst->pins[4]->net)].push_back(gid);
    }

    for (auto& type2Gids : ffType) {
        if (type2Gids.second.size() < 20) continue;

        nCells += type2Gids.second.size();

        vector<int> gids = type2Gids.second;
        map<Site*, vector<int>> site2Gids;
        vector<Box<double>> optrgns(gids.size());

        for (unsigned i = 0; i < gids.size(); i++) {
            int gid = gids[i];
            double gx = groups[gid].x, gy = groups[gid].y;

            site2Gids[database.getSite(gx, gy)].push_back(gid);

            optrgns[i] = GetOptimalRegion({gx, gy}, groupToNets[gid], netBox);
            begCost += optrgns[i].udist(gx, gy) * 10;
        }
        vector<Site*> sites;
        for (const auto& pair : site2Gids) sites.push_back(pair.first);

        vector<vector<pair<int, int>>> supplyToDemand(gids.size());
        vector<int> demandCap(sites.size(), 0);
        vector<pair<int, int>> res;
        int cost;

        for (unsigned c = 0; c < gids.size(); c++) {
            for (unsigned s = 0; s < sites.size(); s++) {
                double dist = optrgns[c].udist(sites[s]->cx(), sites[s]->cy());
                if (dist < 3 || sites[s] == database.getSite(groups[gids[c]].x, groups[gids[c]].y))
                    supplyToDemand[c].push_back({s, dist * 10});
            }
        }

        for (unsigned s = 0; s < sites.size(); s++) {
            demandCap[s] = site2Gids[sites[s]].size();
        }

        LiteMinCostMaxFlow(supplyToDemand, demandCap, supplyToDemand.size(), demandCap.size(), res, cost);
        aftCost += cost;

        map<Site*, bool> needUpdate;
        for (auto site : sites) needUpdate[site] = false;
        for (unsigned i = 0; i < gids.size(); i++) {
            int gid = gids[i];
            Site* site = sites[res[i].first];
            if (site != database.getSite(groups[gid].x, groups[gid].y)) {
                nMove++;
                needUpdate[site] = true;
                needUpdate[database.getSite(groups[gid].x, groups[gid].y)] = true;
            }
        }

        // update groupmap and clbmap
        for (const auto& pair : site2Gids) {
            Site* site = pair.first;
            int x = site->cx(), y = site->cy();

            if (needUpdate[site]) {
                delete dpData.clbMap[x][y];
                dpData.clbMap[x][y] = dpData.NewCLB();
                vector<int> placedGids;
                for (auto gid : dpData.groupMap[x][y]) {
                    if (find(pair.second.begin(), pair.second.end(), gid) == pair.second.end()) {
                        placedGids.push_back(gid);
                        dpData.clbMap[x][y]->AddInsts(groups[gid]);
                    }
                }
                dpData.groupMap[x][y] = move(placedGids);
            }
        }
        for (unsigned i = 0; i < gids.size(); i++) {
            int gid = gids[i];
            Site* site = sites[res[i].first];
            int x = site->cx(), y = site->cy();

            if (needUpdate[site]) {
                dpData.clbMap[x][y]->AddInsts(groups[gid]);
                dpData.groupMap[x][y].push_back(gid);
            }
        }

        // update netbox
        for (unsigned i = 0; i < gids.size(); i++) {
            int gid = gids[i];
            Site* site = sites[res[i].first];
            if (site != database.getSite(groups[gid].x, groups[gid].y)) PartialUpdate(groups[gid], site);
        }
    }

    DumpToDB();
    database.ClearEmptyPack();
    double aftHpwl = GetHpwl();

    printlog(LOG_INFO,
             "#move=%d, #cells=%d, cost_impr_rate=%.3lf%%, hpwl_impr_rate=%.3lf%%",
             nMove,
             nCells,
             (begCost - aftCost) * 100.0 / begCost,
             (begHpwl - aftHpwl) * 100.0 / begHpwl);
}

DPBle::DPBle(vector<Group>& _groups, DPData& _dpData) : groups(_groups), dpData(_dpData) {
    // mapping between groups and nets
    groupToNets.resize(groups.size());
    netToGids.resize(database.nets.size());
    for (unsigned g = 0; g < groups.size(); g++) {
        for (auto inst : groups[g].instances) {
            if (inst != NULL) {
                for (auto pin : inst->pins) {
                    if (pin->net != NULL) {
                        groupToNets[g].push_back(pin->net);
                    }
                }
            }
        }
        sort(groupToNets[g].begin(), groupToNets[g].end());
        groupToNets[g].resize(distance(groupToNets[g].begin(), unique(groupToNets[g].begin(), groupToNets[g].end())));

        for (auto net : groupToNets[g]) {
            netToGids[net->id].push_back(g);
        }
    }
}

double DPBle::GetHpwl() {
    double r = 0;
    for (auto& b : netBox) r += b.uhp();
    return r;
}

void DPBle::PartialUpdate(Group& group, Site* targetSite, bool updateClk) {
    for (auto net : groupToNets[group.id]) {
        netBox[net->id].erase(group.x, group.y);
        netBox[net->id].insert(targetSite->cx(), targetSite->cy());
    }

    if (updateClk) {
        vector<int> gid2Move = {group.id};
        vector<Site*> sites = {targetSite};
        dpData.colrgnData.Update(gid2Move, sites, groups);
    }

    group.x = targetSite->cx();
    group.y = targetSite->cy();
}

void DPBle::UpdateGroupInfo() {
    netBox.assign(database.nets.size(), {});

    // update nets bounding box info
    for (int n = 0; n < (int)database.nets.size(); n++) {
        for (auto gid : netToGids[n]) {
            Group& group = groups[gid];
            netBox[n].insert(group.x, group.y);
        }
    }
}

void DPBle::Run() {
    UpdateGroupInfo();
    GlobalEleMove();
    // GlobalEleSwap();
}