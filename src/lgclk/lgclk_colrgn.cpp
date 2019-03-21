#include "lgclk_colrgn.h"

void ColrgnLG::InitGroupInfo(vector<Group> &groups) {
    groupIds.assign(groups.size(), -1);
    inst2Gid.assign(database.instances.size(), -1);

    for (unsigned g = 0; g < groups.size(); g++) {
        // instance to group mapping
        for (unsigned i = 0; i < groups[g].instances.size(); i++) {
            if (groups[g].instances[i] != NULL) {
                inst2Gid[groups[g].instances[i]->id] = groups[g].id;
            }
        }

        // get group id
        groupIds[g] = groups[g].id;
    }
}

void ColrgnLG::InitPlInfo(vector<Group> &groups) {
    clbMap.assign(database.sitemap_nx, vector<CLBBase *>(database.sitemap_ny, NULL));
    groupMap.assign(database.sitemap_nx, vector<vector<int>>(database.sitemap_ny));

    for (int x = 0; x < database.sitemap_nx; x++) {
        for (int y = 0; y < database.sitemap_ny; y++) {
            if (database.getSite(x, y)->type->name == SiteType::SLICE) {
                clbMap[x][y] = new CLB2;
            }
        }
    }

    for (unsigned g = 0; g < groups.size(); g++) {
        int gx = groups[g].x;
        int gy = groups[g].y;
        groupMap[gx][gy].push_back(groups[g].id);
        if (groups[g].IsBLE()) {
            if (!clbMap[gx][gy]->AddInsts(groups[g])) {
                printlog(LOG_ERROR, "group %d cannot be placed into CLB", groups[g].id);
            }
        }
    }
}

void ColrgnLG::Init(vector<Group> &groups) {
    InitGroupInfo(groups);
    InitPlInfo(groups);
    colrgnData.Init(groups, inst2Gid);
}

void ColrgnLG::GetResult(vector<Group> &groups) {
    // update the placement result
    database.unplaceAll();
    for (int x = 0; x < database.sitemap_nx; x++) {
        for (int y = 0; y < database.sitemap_ny; y++) {
            if (clbMap[x][y] != NULL && !clbMap[x][y]->IsEmpty()) {
                Site *site = database.getSite(x, y);
                if (site->pack == NULL) {
                    database.place(database.addPack(site->type), site);
                }
                Group tmpGroup;
                clbMap[x][y]->GetResult(tmpGroup);
                for (int i = 0; i < 32; i++) {
                    if (tmpGroup.instances[i] != NULL) {
                        database.place(tmpGroup.instances[i], database.getSite(x, y), i);
                    }
                }
            }
        }
    }
    database.ClearEmptyPack();

    vector<Group> tmpGroups;
    for (int x = 0; x < database.sitemap_nx; x++) {
        for (int y = 0; y < database.sitemap_ny; y++) {
            for (auto gid : groupMap[x][y]) {
                tmpGroups.push_back(groups[gid]);
                tmpGroups[tmpGroups.size() - 1].id = tmpGroups.size() - 1;
            }
        }
    }
    groups = tmpGroups;
}

void ColrgnLG::Clear() {
    for (int x = 0; x < database.sitemap_nx; x++) {
        for (int y = 0; y < database.sitemap_ny; y++) {
            if (clbMap[x][y] != NULL) delete clbMap[x][y];
        }
    }

    colrgnData.Clear();
}

bool ColrgnLG::IsClkMovable(
    Net *net, LGColrgn *srcColrgn, vector<Group> &groups, vector<int> &gid2Move, MapUpdater &srcUpdater) {
    int lx = srcColrgn->lx, ly = srcColrgn->ly;
    for (int x = 0; x < 2; x++) {
        for (int y = 0; y < 30; y++) {
            auto it = colrgnData.clkMap[lx + x][ly + y].find(net);
            if (it != colrgnData.clkMap[lx + x][ly + y].end()) {
                srcUpdater.sites.push_back(database.getSite(x + lx, y + ly));
                srcUpdater.updatedGroups.push_back(groupMap[x + lx][y + ly]);
                if (database.getSite(x + lx, y + ly)->type->name != SiteType::SLICE) {
                    srcUpdater.updatedClbs.push_back(NULL);
                } else {
                    srcUpdater.updatedClbs.push_back(new CLB2);
                }
                for (auto gid : it->second) {
                    gid2Move.push_back(gid);
                    vector<int> &gids = srcUpdater.updatedGroups.back();
                    gids.erase(find(gids.begin(), gids.end(), gid));
                }
            }
        }
    }

    for (unsigned i = 0; i < srcUpdater.updatedGroups.size(); i++) {
        if (srcUpdater.updatedClbs[i] != NULL) {
            for (auto gid : srcUpdater.updatedGroups[i]) {
                if (!srcUpdater.updatedClbs[i]->AddInsts(groups[gid])) return false;
            }
        }
    }

    return true;
}

double ColrgnLG::GetMove(vector<int> &gid2Move,
                         LGColrgn *srcColrgn,
                         vector<Group> &groups,
                         MapUpdater &destUpdater,
                         double &disp,
                         const double curMin) {
    double cost = 0;
    bool isSucc = true;

    const double nonBlePen = 10;

    disp = 0;

    for (auto gid : gid2Move) {
        int winWidth = 0;
        bool isPlaced = false;
        Group &group = groups[gid];
        pair<int, int> center(group.x, group.y);

        while (!isPlaced && winWidth < 1000) {
            vector<Site *> candSites;
            GetWinElem(candSites, database.sites, center, winWidth);

            for (auto site : candSites) {
                if (colrgnData.GetColrgn(site->cx(), site->cy()) != srcColrgn && group.IsTypeMatch(site) &&
                    colrgnData.IsClkMoveLeg({group.id}, site, groups, false)) {
                    vector<int> tarGroups;
                    tarGroups = groupMap[site->cx()][site->cy()];
                    auto rit = find(destUpdater.sites.rbegin(), destUpdater.sites.rend(), site);
                    if (rit != destUpdater.sites.rend()) {
                        tarGroups = destUpdater.updatedGroups[distance(destUpdater.sites.begin(), rit.base()) - 1];
                    }

                    CLBBase *clb = NULL;

                    if (group.IsBLE()) {
                        clb = new CLB2;
                        for (auto gid : tarGroups) {
                            clb->AddInsts(groups[gid]);
                        }
                        isPlaced = (clb->AddInsts(group));
                    } else {
                        isPlaced = (tarGroups.size() == 0);
                    }

                    if (isPlaced) {
                        cost += abs(site->cx() - group.x) * 0.5 +
                                abs(site->cy() - group.y) * (group.IsBLE() ? 1 : nonBlePen);
                        disp += abs(site->cx() - group.x) * 0.5 + abs(site->cy() - group.y);
                        if (cost > curMin) return -1;

                        destUpdater.updatedClbs.push_back(clb);
                        destUpdater.updatedGroups.push_back(tarGroups);
                        destUpdater.updatedGroups.back().push_back(gid);
                        destUpdater.sites.push_back(site);
                        break;
                    }
                }
            }
            winWidth++;
        }

        if (!isPlaced) {
            isSucc = false;
            break;
        }
    }

    if (isSucc)
        return cost;
    else
        return -1;
}

void ColrgnLG::Update(vector<int> &gid2Move, MapUpdater &srcUpdater, MapUpdater &destUpdater, vector<Group> &groups) {
    srcUpdater.Update(clbMap, groupMap);
    destUpdater.Update(clbMap, groupMap);

    colrgnData.Update(gid2Move, destUpdater.sites, groups);

    for (unsigned i = 0; i < gid2Move.size(); i++) {
        int gid = gid2Move[i];
        groups[gid].x = destUpdater.sites[i]->cx();
        groups[gid].y = destUpdater.sites[i]->cy();
    }
}

void ColrgnLG::RunLG(vector<Group> &groups) {
    int nMove = 0;
    double totalDisp = 0;
    int nOF = 0;

    for (int x = 0; x < colrgnData.colrgn_nx; x++) {
        for (int y = 0; y < colrgnData.colrgn_ny; y++) {
            LGColrgn *srcColrgn = colrgnData.lgColrgns[x][y];

            if (srcColrgn->nets.size() > database.COLRGN_NCLK) {
                // printlog(LOG_INFO,"colrgn(%d,%d) at (%d,%d) has overflow, OF=%lu(%lu)", x, y, srcColrgn->lx,
                // srcColrgn->ly, srcColrgn->nets.size(), database.COLRGN_NCLK);
                nOF++;

                while (srcColrgn->nets.size() > database.COLRGN_NCLK) {
                    Net *net2Move = NULL;
                    MapUpdater srcUpdater;
                    MapUpdater destUpdater;
                    vector<int> gid2Move;
                    double cost = DBL_MAX;
                    double disp = 0;

                    for (auto net : srcColrgn->nets) {
                        MapUpdater srcUpdater_tmp;
                        MapUpdater destUpdater_tmp;
                        vector<int> gid2Move_tmp;
                        double cost_tmp = -1;
                        double disp_tmp;

                        if (IsClkMovable(
                                net, srcColrgn, groups, gid2Move_tmp, srcUpdater_tmp)) {  // only ble is allowed to move
                            cost_tmp = GetMove(gid2Move_tmp, srcColrgn, groups, destUpdater_tmp, disp_tmp, cost);
                            if (cost_tmp != -1 && cost > cost_tmp) {
                                net2Move = net;

                                cost = cost_tmp;
                                disp = disp_tmp;
                                gid2Move = gid2Move_tmp;
                                destUpdater = destUpdater_tmp;
                                srcUpdater = srcUpdater_tmp;
                            }
                        } else {
                            continue;
                        }
                    }

                    if (net2Move != NULL) {
                        // printlog(LOG_INFO, "moving %s, #move= %lu disp= %.1f", net2Move->name.c_str(),
                        // gid2Move.size(), cost);
                        // DSP/RAM
                        for (unsigned i = 0; i < gid2Move.size(); i++) {
                            if (!groups[gid2Move[i]].IsBLE()) {
                                database.place(database.getSite(groups[gid2Move[i]].x, groups[gid2Move[i]].y)->pack,
                                               destUpdater.sites[i]);
                            }
                        }

                        Update(gid2Move, srcUpdater, destUpdater, groups);
                        totalDisp += disp;
                        nMove += gid2Move.size();
                    } else {
                        printlog(LOG_ERROR,
                                 "colrgn(%d,%d) at (%d,%d) cannot be legalized",
                                 x,
                                 y,
                                 srcColrgn->lx,
                                 srcColrgn->ly);
                    }
                }
            }
        }
    }

    GetResult(groups);

    printlog(LOG_INFO,
             "#Move=%d, disp=%.1f, #OF=%d(%d)",
             nMove,
             totalDisp,
             nOF,
             colrgnData.colrgn_nx * colrgnData.colrgn_ny);
}

void MapUpdater::Update(vector<vector<CLBBase *>> &clbMap, vector<vector<vector<int>>> &groupMap) {
    for (unsigned i = 0; i < sites.size(); i++) {
        Site *site = sites[i];

        clbMap[site->cx()][site->cy()] = updatedClbs[i];
        groupMap[site->cx()][site->cy()] = updatedGroups[i];
    }
}