#include "./lg_main.h"
#include "./lg_dp_chain.h"
#include "./lg_dp_utils.h"

ChainFinder::ChainFinder(vector<Group>& _groups) : groups(_groups) { checkColrgn = (database.crmap_nx != 0); }

void ChainFinder::Backtrack(int gid2Move) {
    inChain[gid2Move] = false;
    delete (clbChain.back());
    gidChain.pop_back();
    clbChain.pop_back();
    orderChain.pop_back();
    posChain.pop_back();
    if (checkColrgn) colrgnChain.pop_back();
}

bool ChainFinder::DFS(LGData& lgData, ChainmoveTarget optTarget) {
    int gid2Move = gidChain.back();

    if (gidChain.size() > depth) {
        Backtrack(gid2Move);
        return false;
    }

    // get the next candidate
    int winWidth = 0;
    while (true) {
        bool doBt;
        if (optTarget == DISP_OPT) {
            double orgDisp = 0.5 * abs((int)groups[gid2Move].x - (int)lgData.groupsX[gid2Move]) +
                             abs((int)groups[gid2Move].y - (int)lgData.groupsY[gid2Move]);
            doBt = gidChain.size() == 1 ? (winWidth >= 2 * orgDisp) : (winWidth > 2 * orgDisp);
        } else {
            doBt = (winWidth >= 2 * maxDisp || winWidth > 2 * maxTotDisp);
        }

        if (doBt) {
            maxTotDisp -= 0.5 * abs((int)groups[gid2Move].x - (int)lgData.groupsX[gid2Move]) +
                          abs((int)groups[gid2Move].y - (int)lgData.groupsY[gid2Move]);
            Backtrack(gid2Move);
            return false;
        }

        vector<Site*> sites;
        GetWinElem(sites, database.sites, {groups[gid2Move].x, groups[gid2Move].y}, winWidth);
        SqueezeCandSites(sites, groups[gid2Move]);

        for (auto site : sites) {
            int x = site->x, y = site->y;
            int hasChange = -1;
            for (int i = posChain.size() - 1; i >= 0; i--) {
                if (posChain[i] == x * database.sitemap_ny + y) {
                    hasChange = i;
                    break;
                }
            }
            vector<int> placedGroups;
            CLBBase* tarClb = NULL;

            if (hasChange == -1) {
                placedGroups = lgData.placedGroupMap[x][y];
                if (clbMap[x][y] == NULL) {
                    clbMap[x][y] = new CLB2;
                    for (auto gid : placedGroups) clbMap[x][y]->AddInsts(groups[gid]);
                }
                tarClb = clbMap[x][y];
            } else {
                placedGroups = orderChain[hasChange];
                tarClb = clbChain[hasChange];
            }

            if (tarClb->AddInsts(groups[gid2Move])) {
                vector<int> lgorder = placedGroups;
                lgorder.push_back(gid2Move);

                gidChain.push_back(x * database.sitemap_ny + y);  // get the final dest of path
                clbChain.push_back(tarClb);
                orderChain.push_back(lgorder);
                posChain.push_back(x * database.sitemap_ny + y);
                return true;
            }
        }

        unsigned count = 0;
        for (auto site : sites) {
            if (site == database.getSite(lgData.groupsX[gid2Move], lgData.groupsY[gid2Move])) continue;
            int x = site->x, y = site->y;
            int hasChange = -1;
            for (int i = posChain.size() - 1; i >= 0; i--) {
                if (posChain[i] == x * database.sitemap_ny + y) {
                    hasChange = i;
                    break;
                }
            }
            vector<int> placedGroups = (hasChange == -1) ? lgData.placedGroupMap[x][y] : orderChain[hasChange];

            while (rand() % 100 > 15) {
                int gid2Sub = placedGroups[rand() % placedGroups.size()];
                if (inChain[gid2Sub]) continue;

                if (count++ > breadth) {
                    Backtrack(gid2Move);
                    return false;
                }

                CLBBase* subClb = new CLB2;
                vector<int> lgorder;
                bool canSub = true;

                for (auto gid : placedGroups) {
                    if (gid != gid2Sub) {
                        if (!subClb->AddInsts(groups[gid])) {
                            canSub = false;
                            break;
                        }
                        lgorder.push_back(gid);
                    }
                }
                if (canSub) {
                    canSub = subClb->AddInsts(groups[gid2Move]);
                    lgorder.push_back(gid2Move);
                }

                if (canSub) {
                    inChain[gid2Sub] = true;
                    gidChain.push_back(gid2Sub);
                    clbChain.push_back(subClb);
                    orderChain.push_back(lgorder);
                    posChain.push_back(x * database.sitemap_ny + y);
                    maxTotDisp += (0.5 * abs((int)groups[gid2Sub].x - (int)lgData.groupsX[gid2Sub]) +
                                   abs((int)groups[gid2Sub].y - (int)lgData.groupsY[gid2Sub])) -
                                  (0.5 * abs((int)groups[gid2Move].x - x) + abs((int)groups[gid2Move].y - y));

                    if (DFS(lgData, optTarget)) {
                        return true;
                    }
                } else {
                    delete subClb;
                }
            }
        }

        winWidth++;
    }

    return false;
}

void LGData::ChainUpdate(vector<int>& path, vector<CLBBase*>& clbChain, vector<vector<int>>& lgorderChain) {
    int destX = path.back() / database.sitemap_ny, destY = path.back() % database.sitemap_ny;

    double orgDisp = 0, newDisp = 0, orgMaxDisp = DBL_MIN, newMaxDisp = DBL_MIN;
    Site* destSite = database.getSite(destX, destY);
    for (unsigned i = 0; i < path.size() - 1; i++) {
        int tarX = (i == path.size() - 2) ? destSite->cx() : groupsX[path[i + 1]],
            tarY = (i == path.size() - 2) ? destSite->cy() : groupsY[path[i + 1]];
        int orgX = groups[path[i]].x, orgY = groups[path[i]].y;
        int curX = groupsX[path[i]], curY = groupsY[path[i]];
        orgDisp += 0.5 * abs(curX - orgX) + abs(curY - orgY);
        newDisp += 0.5 * abs(tarX - orgX) + abs(tarY - orgY);
        orgMaxDisp = max(0.5 * abs(curX - orgX) + abs(curY - orgY), orgMaxDisp);
        newMaxDisp = max(0.5 * abs(tarX - orgX) + abs(tarY - orgY), newMaxDisp);
    }
    // printf("disp: %.1f->%.1f, maxDisp: %.1f->%.1f, #groups: %lu\n", orgDisp, newDisp, orgMaxDisp, newMaxDisp,
    // path.size()-1);

    // printf("chain: ");
    // for (unsigned i=0; i<path.size()-1; i++){
    //     double tarX=(i==path.size()-2)?destSite->cx():groupsX[path[i+1]];
    //     double tarY=(i==path.size()-2)?destSite->cy():groupsY[path[i+1]];
    //     printf("%d(%.1f,%.1f)->(%.1f,%.1f),", path[i], groups[path[i]].x, groups[path[i]].y, tarX, tarY);
    // }
    // printf("\n");
    // fflush(stdout);

    // update clbmap and placedGroupMap
    for (unsigned i = 0; i < path.size(); i++) {
        int gid = path[i];
        int tarX = (i == path.size() - 1) ? destX : groupsX[gid];
        int tarY = (i == path.size() - 1) ? destY : groupsY[gid];
        placedGroupMap[tarX][tarY] = lgorderChain[i];
        clbMap[tarX][tarY] = clbChain[i];
    }

    for (unsigned i = 0; i < path.size() - 1; i++) {
        int gid = path[i];
        int tarX = (i == path.size() - 2) ? destX : groupsX[path[i + 1]];
        int tarY = (i == path.size() - 2) ? destY : groupsY[path[i + 1]];
        Site* targetSite = database.sites[tarX][tarY];

        // update bounding box
        for (auto net : group2Nets[gid]) {
            netBox[net->id].erase(groupsX[gid], groupsY[gid]);
            netBox[net->id].insert(targetSite->cx(), targetSite->cy());
        }

        // update pins distribution
        nPinPerSite[groupsX[gid]][groupsY[gid]] -= nPinPerGroup[gid];
        nPinPerSite[tarX][tarY] += nPinPerGroup[gid];

        // update group location info
        groupsX[gid] = targetSite->cx();
        groupsY[gid] = targetSite->cy();
    }
}

bool ChainFinder::Init(Group& group, LGData& lgData) {
    CLBBase* srcClb = new CLB2;
    vector<int> lgorder;

    for (auto gid : lgData.placedGroupMap[lgData.groupsX[group.id]][lgData.groupsY[group.id]]) {
        if (gid != group.id) {
            if (!srcClb->AddInsts(groups[gid])) {
                return false;
                break;
            }
            lgorder.push_back(gid);
        }
    }

    clbMap.assign(database.sitemap_nx, vector<CLBBase*>(database.sitemap_ny, NULL));
    inChain.assign(groups.size(), false);

    inChain[group.id] = true;
    clbChain = {srcClb};
    orderChain = {lgorder};
    posChain = {(int)lgData.groupsX[group.id] * database.sitemap_ny + (int)lgData.groupsY[group.id]};
    gidChain = {group.id};
    maxTotDisp = maxDisp =
        0.5 * abs((int)group.x - (int)lgData.groupsX[group.id]) + abs((int)group.y - (int)lgData.groupsY[group.id]);

    checkColrgn = false;

    return true;
}

int Legalizer::ChainMove(Group& group, ChainmoveTarget optTarget) {
    ChainFinder chainFinder(groups);
    if (chainFinder.Init(group, lgData)) {
        if (chainFinder.DFS(lgData, optTarget)) {
            lgData.ChainUpdate(chainFinder.gidChain, chainFinder.clbChain, chainFinder.orderChain);
            return chainFinder.gidChain.size();
        }
    }

    return -1;
}

bool ChainFinder::DFS(DPBle& dpBle) {
    int gid2Move = gidChain.back();

    if (gidChain.size() > depth) {
        Backtrack(gid2Move);
        return false;
    }

    vector<Site*> sites;
    Box<double> optrgn =
        GetOptimalRegion({groups[gid2Move].x, groups[gid2Move].y}, dpBle.groupToNets[gid2Move], dpBle.netBox);
    for (int x = optrgn.lx(); x < optrgn.ux(); x++) {
        for (int y = optrgn.ly(); y < optrgn.uy(); y++) {
            Site* site = database.getSite(x, y);
            if (groups[gid2Move].IsTypeMatch(site) && groups[gid2Move].InClkBox(site)) sites.push_back(site);
        }
    }
    function<double(Site*)> cal_disp = [&](Site* site) {
        return 0.5 * abs(groups[gid2Move].x - site->cx()) + abs(groups[gid2Move].y - site->cy());
    };
    ComputeAndSort(sites, cal_disp, less<double>());

    for (auto site : sites) {
        int x = site->x, y = site->y;
        int siteIndx = -1, colrgnIndx = -1;
        for (int i = posChain.size() - 1; i >= 0; i--) {
            if (posChain[i] == x * database.sitemap_ny + y) {
                siteIndx = colrgnIndx = i;
                break;
            }
        }
        vector<int> placedGroups = (siteIndx == -1) ? dpBle.dpData.groupMap[x][y] : orderChain[siteIndx];

        if (checkColrgn) {
            for (int i = posChain.size() - 1; i >= 0 && colrgnIndx == -1; i--) {
                int posX = posChain[i] / database.sitemap_ny;
                int posY = posChain[i] % database.sitemap_ny;
                if (dpBle.dpData.colrgnData.GetColrgn(x, y) == dpBle.dpData.colrgnData.GetColrgn(posX, posY)) {
                    colrgnIndx = i;
                    break;
                }
            }

            unordered_map<Net*, int> colrgn =
                (colrgnIndx == -1) ? dpBle.dpData.colrgnData.GetColrgnData(x, y) : colrgnChain[colrgnIndx];
            for (auto net : dpBle.dpData.colrgnData.group2Clk[gid2Move]) {
                colrgn[net]++;
            }
            if (colrgn.size() > database.COLRGN_NCLK) continue;
        }

        CLBBase* tarClb = NULL;
        if (siteIndx == -1) {
            if (clbMap[x][y] == NULL) {
                clbMap[x][y] = new CLB2;
                for (auto gid : placedGroups) clbMap[x][y]->AddInsts(groups[gid]);
            }
            tarClb = clbMap[x][y];
        } else {
            tarClb = clbChain[siteIndx];
        }

        if (tarClb->AddInsts(groups[gid2Move])) {
            vector<int> lgorder = placedGroups;
            lgorder.push_back(gid2Move);

            gidChain.push_back(x * database.sitemap_ny + y);  // get the final dest of path
            clbChain.push_back(tarClb);
            orderChain.push_back(lgorder);
            posChain.push_back(x * database.sitemap_ny + y);
            return true;
        }
    }

    unsigned count = 0;
    for (auto site : sites) {
        if (site == database.getSite(groups[gid2Move].x, groups[gid2Move].y)) continue;
        int x = site->x, y = site->y;
        int siteIndx = -1, colrgnIndx = -1;
        for (int i = posChain.size() - 1; i >= 0; i--) {
            if (posChain[i] == x * database.sitemap_ny + y) {
                siteIndx = colrgnIndx = i;
                break;
            }
        }
        vector<int> placedGroups = (siteIndx == -1) ? dpBle.dpData.groupMap[x][y] : orderChain[siteIndx];
        if (placedGroups.empty()) continue;

        unordered_map<Net*, int> colrgn;
        if (checkColrgn) {
            for (int i = posChain.size() - 1; i >= 0 && colrgnIndx == -1; i--) {
                int posX = posChain[i] / database.sitemap_ny;
                int posY = posChain[i] % database.sitemap_ny;
                if (dpBle.dpData.colrgnData.GetColrgn(x, y) == dpBle.dpData.colrgnData.GetColrgn(posX, posY)) {
                    colrgnIndx = i;
                    break;
                }
            }
            colrgn = (colrgnIndx == -1) ? dpBle.dpData.colrgnData.GetColrgnData(x, y) : colrgnChain[colrgnIndx];
            for (auto net : dpBle.dpData.colrgnData.group2Clk[gid2Move]) {
                colrgn[net]++;
            }
        }

        while (rand() % 100 > 15) {
            int gid2Sub = placedGroups[rand() % placedGroups.size()];
            if (inChain[gid2Sub]) continue;

            if (count++ > breadth) {
                Backtrack(gid2Move);
                return false;
            }

            if (checkColrgn) {
                for (auto net : dpBle.dpData.colrgnData.group2Clk[gid2Sub]) {
                    colrgn[net]--;
                    if (colrgn[net] == 0) colrgn.erase(colrgn.find(net));
                }
                if (colrgn.size() > database.COLRGN_NCLK) {
                    for (auto net : dpBle.dpData.colrgnData.group2Clk[gid2Sub]) colrgn[net]++;
                    continue;
                }
            }

            CLBBase* subClb = new CLB2;
            vector<int> lgorder;
            bool canSub = true;

            for (auto gid : placedGroups) {
                if (gid != gid2Sub) {
                    if (!subClb->AddInsts(groups[gid])) {
                        canSub = false;
                        break;
                    }
                    lgorder.push_back(gid);
                }
            }
            if (canSub) {
                canSub = subClb->AddInsts(groups[gid2Move]);
                lgorder.push_back(gid2Move);
            }

            if (canSub) {
                inChain[gid2Sub] = true;
                gidChain.push_back(gid2Sub);
                clbChain.push_back(subClb);
                orderChain.push_back(lgorder);
                posChain.push_back(x * database.sitemap_ny + y);
                if (checkColrgn) colrgnChain.push_back(colrgn);

                if (DFS(dpBle)) {
                    return true;
                }
            } else {
                delete subClb;
            }

            if (checkColrgn) {
                for (auto net : dpBle.dpData.colrgnData.group2Clk[gid2Sub]) {
                    colrgn[net]++;
                }
            }
        }
    }

    Backtrack(gid2Move);
    return false;
}

void DPBle::ChainUpdate(vector<int>& path, vector<CLBBase*>& clbChain, vector<vector<int>>& lgorderChain) {
    int destX = path.back() / database.sitemap_ny, destY = path.back() % database.sitemap_ny;

    // string s;
    // for (unsigned i=0; i<path.size()-1; i++){
    //     s+=to_string(path[i])+",";
    // }
    // printlog(LOG_INFO,"%s", s.c_str());

    // update clbmap and placedGroupMap
    for (unsigned i = 0; i < path.size(); i++) {
        int gid = path[i];
        int tarX = (i == path.size() - 1) ? destX : groups[gid].x;
        int tarY = (i == path.size() - 1) ? destY : groups[gid].y;
        dpData.groupMap[tarX][tarY] = lgorderChain[i];
        dpData.clbMap[tarX][tarY] = clbChain[i];
    }

    for (unsigned i = 0; i < path.size() - 1; i++) {
        int gid = path[i];
        int tarX = (i == path.size() - 2) ? destX : groups[path[i + 1]].x;
        int tarY = (i == path.size() - 2) ? destY : groups[path[i + 1]].y;
        Site* targetSite = database.sites[tarX][tarY];

        PartialUpdate(groups[gid], targetSite);

        // update group location info
        groups[gid].x = targetSite->cx();
        groups[gid].y = targetSite->cy();
    }
}

void ChainFinder::Init(Group& group, DPBle& dpBle) {
    CLBBase* srcClb = new CLB2;
    vector<int> lgorder;

    for (auto gid : dpBle.dpData.groupMap[groups[group.id].x][groups[group.id].y]) {
        if (gid != group.id) {
            srcClb->AddInsts(groups[gid]);
            lgorder.push_back(gid);
        }
    }

    if (checkColrgn) {
        unordered_map<Net*, int> colrgn = dpBle.dpData.colrgnData.GetColrgnData(group.x, group.y);
        for (auto net : dpBle.dpData.colrgnData.group2Clk[group.id]) {
            colrgn[net]--;
            if (colrgn[net] == 0) colrgn.erase(colrgn.find(net));
        }
        colrgnChain = {colrgn};
    }

    clbMap.assign(database.sitemap_nx, vector<CLBBase*>(database.sitemap_ny, NULL));
    inChain.assign(groups.size(), false);

    inChain[group.id] = true;
    clbChain = {srcClb};
    orderChain = {lgorder};
    posChain = {(int)groups[group.id].x * database.sitemap_ny + (int)groups[group.id].y};
    gidChain = {group.id};
}

bool DPBle::OptrgnChainMove(Group& group) {
    // return false;

    ChainFinder chainFinder(groups);
    chainFinder.Init(group, *this);

    if (chainFinder.DFS(*this)) {
        this->ChainUpdate(chainFinder.gidChain, chainFinder.clbChain, chainFinder.orderChain);
        return true;
    } else {
        return false;
    }
}