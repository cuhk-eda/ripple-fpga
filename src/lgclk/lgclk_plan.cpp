#include "lgclk_plan.h"

void ClkrgnPlan::InitGroupInfo(vector<Group> &groups) {
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

void ClkrgnPlan::InitClkInfo(vector<Group> &groups) {
    int nClknet = 0;

    lgClkrgns.assign(database.crmap_nx, vector<LGClkrgn *>(database.crmap_ny, NULL));
    clkMap.assign(database.sitemap_nx, vector<unordered_map<Net *, unordered_set<int>>>(database.sitemap_ny));
    group2Clk.assign(groups.size(), unordered_set<Net *>());

    for (auto net : database.nets) {
        if (net->isClk) {
            nClknet += net->isClk;
            clkboxs[net].net = net;
            for (auto pin : net->pins) {
                if (pin->type->type != 'o' && pin->instance != NULL) {
                    Group &group = groups[inst2Gid[pin->instance->id]];
                    if (group2Clk[group.id].find(net) == group2Clk[group.id].end()) {
                        clkboxs[net].box.x.insert(group.x);
                        clkboxs[net].box.y.insert(group.y);
                        clkMap[group.x][group.y][net].insert(group.id);
                        group2Clk[group.id].insert(net);
                    }
                }
            }
        }
    }

    printlog(LOG_INFO, "#clk=%d", nClknet);

    for (int x = 0; x < database.crmap_nx; x++) {
        for (int y = 0; y < database.crmap_ny; y++) {
            lgClkrgns[x][y] = new LGClkrgn(database.clkrgns[x][y]);
        }
    }

    for (auto &pair : clkboxs) {
        vector<LGClkrgn *> clkrgns = pair.second.GetClkrgns(lgClkrgns);

        for (auto clkrgn : clkrgns) {
            clkrgn->nets.insert(pair.second.net);
        }
    }
}

void ClkrgnPlan::PrintStat(vector<Group> &groups) {
    // info of input
    for (int cr_x = 0; cr_x < database.crmap_nx; cr_x++) {
        for (int cr_y = 0; cr_y < database.crmap_ny; cr_y++) {
            LGClkrgn *clkrgn = lgClkrgns[cr_x][cr_y];
            if (clkrgn->nets.size() > database.CLKRGN_NCLK) {
                int lx = clkrgn->clkrgn->lx;
                int ly = clkrgn->clkrgn->ly;
                int hx = clkrgn->clkrgn->hx;
                int hy = clkrgn->clkrgn->hy;
                printlog(LOG_INFO,
                         "clkrgn_X%d_Y%d (%d, %d, %d, %d) OF = %lu(%lu)",
                         cr_x,
                         cr_y,
                         lx,
                         ly,
                         hx,
                         hy,
                         clkrgn->nets.size(),
                         database.CLKRGN_NCLK);
                for (auto net : clkrgn->nets) {
                    if (clkboxs[net].box.x.l() < lx && clkboxs[net].box.y.l() < ly && clkboxs[net].box.x.u() > hx &&
                        clkboxs[net].box.y.u() > hy) {
                        printlog(LOG_INFO,
                                 "%s, #move= N/A, box=(%.1f, %.1f, %.1f, %.1f)",
                                 net->name.c_str(),
                                 clkboxs[net].box.x.l(),
                                 clkboxs[net].box.y.l(),
                                 clkboxs[net].box.x.u(),
                                 clkboxs[net].box.y.u());
                        continue;
                    }

                    vector<int> gid2Move;
                    double disp = -1;
                    double pos = -1;
                    int dir = -1;

                    // dsp/ram are only allowed to move vertically and with high penalty
                    GetMove(clkboxs[net], clkrgn, groups, gid2Move, pos, disp, dir);

                    if (dir != -1) {
                        printlog(LOG_INFO,
                                 "%s, #move= %lu, disp= %.1f, dir= %d, pos=%.2f, box=(%.1f, %.1f, %.1f, %.1f)",
                                 net->name.c_str(),
                                 gid2Move.size(),
                                 disp,
                                 dir,
                                 abs(pos),
                                 clkboxs[net].box.x.l(),
                                 clkboxs[net].box.y.l(),
                                 clkboxs[net].box.x.u(),
                                 clkboxs[net].box.y.u());
                    } else {
                        printlog(LOG_INFO,
                                 "%s, #move= N/A, box=(%.1f, %.1f, %.1f, %.1f)",
                                 net->name.c_str(),
                                 clkboxs[net].box.x.l(),
                                 clkboxs[net].box.y.l(),
                                 clkboxs[net].box.x.u(),
                                 clkboxs[net].box.y.u());
                    }
                }
                printf("\n");
            }
        }
    }
}

void ClkrgnPlan::Init(vector<Group> &groups) {
    InitGroupInfo(groups);
    InitClkInfo(groups);
    // PrintStat(groups);
}

void ClkrgnPlan::UpdatePl(LGClkrgn *clkrgn, vector<int> &gid2Move, double &tarPos, vector<Group> &groups) {
    vector<double> orgGroupX(gid2Move.size());
    vector<double> orgGroupY(gid2Move.size());

    for (unsigned i = 0; i < gid2Move.size(); i++) {
        int gid = gid2Move[i];
        orgGroupX[i] = groups[gid].x;
        orgGroupY[i] = groups[gid].y;

        if (tarPos > 0) {
            groups[gid].x = tarPos;
        } else {
            groups[gid].y = -tarPos;
        }
    }

    unordered_set<Net *> clk2Update;
    for (unsigned i = 0; i < gid2Move.size(); i++) {
        int gid = gid2Move[i];

        for (auto net : group2Clk[gid]) {
            clkMap[orgGroupX[i]][orgGroupY[i]][net].erase(clkMap[orgGroupX[i]][orgGroupY[i]][net].find(gid));
            if (clkMap[orgGroupX[i]][orgGroupY[i]][net].empty())
                clkMap[orgGroupX[i]][orgGroupY[i]].erase(clkMap[orgGroupX[i]][orgGroupY[i]].find(net));
        }

        for (auto net : group2Clk[gid]) {
            clkMap[groups[gid].x][groups[gid].y][net].insert(gid);
        }

        for (auto net : group2Clk[gid]) {
            clk2Update.insert(net);
            clkboxs[net].box.x.erase(orgGroupX[i]);
            clkboxs[net].box.y.erase(orgGroupY[i]);
            clkboxs[net].box.x.insert(groups[gid].x);
            clkboxs[net].box.y.insert(groups[gid].y);
        }
    }

    for (auto net : clk2Update) {
        clkrgn->nets.erase(clkrgn->nets.find(net));
        vector<LGClkrgn *> clkrgns = clkboxs[net].GetClkrgns(lgClkrgns);

        for (auto clkrgn : clkrgns) {
            clkrgn->nets.insert(clkboxs[net].net);
        }
    }
}

bool ClkrgnPlan::IsMoveLeg(vector<int> &gid2Move, double pos, int dir, vector<Group> &groups) {
    unordered_map<LGClkrgn *, unordered_set<Net *>> clkrgn2Nets;

    for (auto gid : gid2Move) {
        double x = (dir < 2) ? pos : groups[gid].x;
        double y = (dir < 2) ? groups[gid].y : pos;

        LGClkrgn *clkrgn = GetClkrgn(x, y);
        if (clkrgn2Nets.find(clkrgn) == clkrgn2Nets.end()) clkrgn2Nets[clkrgn] = clkrgn->nets;

        unsigned nClk_before = clkrgn2Nets[clkrgn].size();
        for (auto net : group2Clk[gid]) {
            clkrgn2Nets[clkrgn].insert(net);
        }

        if (clkrgn2Nets[clkrgn].size() > nClk_before) return false;
    }

    return true;
}

double ClkrgnPlan::GetMove(ClkBox &clkbox,
                           LGClkrgn *clkrgn,
                           vector<Group> &groups,
                           vector<int> &gid2Move,
                           double &pos,
                           double &disp,
                           int &dir) {
    double cost = DBL_MAX;

    dir = -1;
    pos = -1;

    const double nonBlePen = 10000;

    double box_lx = clkbox.box.x.l();
    double box_ly = clkbox.box.y.l();
    double box_hx = clkbox.box.x.u();
    double box_hy = clkbox.box.y.u();

    int cr_lx = clkrgn->clkrgn->lx;
    int cr_ly = clkrgn->clkrgn->ly;
    int cr_hx = clkrgn->clkrgn->hx;
    int cr_hy = clkrgn->clkrgn->hy;

    Net *net = clkbox.net;

    if (box_hx > cr_hx && box_lx < cr_hx && box_lx > cr_lx) {
        // RIGHT;
        double tmpDisp = 0;
        double tmpCost = 0;
        vector<int> gid2Move_tmp;

        for (int x = clkboxs[net].box.x.l(); x < cr_hx; x++) {
            for (int y = clkboxs[net].box.y.l(); y < (int)clkboxs[net].box.y.u() + 1; y++) {
                auto it = clkMap[x][y].find(net);
                if (it == clkMap[x][y].end()) continue;
                for (auto gid : it->second) {
                    gid2Move_tmp.push_back(gid);
                    tmpDisp += 0.5 * abs(groups[gid].x - (cr_hx + 0.01));
                    tmpCost += 0.5 * abs(groups[gid].x - (cr_hx + 0.01)) * (groups[gid].IsBLE() ? 1 : nonBlePen);
                }
            }
        }

        if (tmpCost < cost && IsMoveLeg(gid2Move_tmp, cr_hx + 0.01, 0, groups)) {
            disp = tmpDisp;
            cost = tmpCost;
            pos = cr_hx + 0.01;
            dir = 0;
            gid2Move = gid2Move_tmp;
        }
    }

    if (box_lx < cr_lx && box_hx < cr_hx && box_hx > cr_lx) {
        // LEFT
        double tmpDisp = 0;
        double tmpCost = 0;
        vector<int> gid2Move_tmp;

        for (int x = cr_lx; x < (int)clkboxs[net].box.x.u() + 1; x++) {
            for (int y = clkboxs[net].box.y.l(); y < (int)clkboxs[net].box.y.u() + 1; y++) {
                auto it = clkMap[x][y].find(net);
                if (it == clkMap[x][y].end()) continue;
                for (auto gid : it->second) {
                    gid2Move_tmp.push_back(gid);
                    tmpDisp += 0.5 * abs(groups[gid].x - (cr_lx - 0.01));
                    tmpCost += 0.5 * abs(groups[gid].x - (cr_lx - 0.01)) * (groups[gid].IsBLE() ? 1 : nonBlePen);
                }
            }
        }

        if (tmpCost < cost && IsMoveLeg(gid2Move_tmp, cr_lx - 0.01, 1, groups)) {
            disp = tmpDisp;
            cost = tmpCost;
            pos = cr_lx - 0.01;
            dir = 1;
            gid2Move = gid2Move_tmp;
        }
    }

    if (box_hy > cr_hy && box_ly < cr_hy && box_ly > cr_ly) {
        // UP
        double tmpDisp = 0;
        double tmpCost = 0;
        vector<int> gid2Move_tmp;

        for (int x = clkboxs[net].box.x.l(); x < (int)clkboxs[net].box.x.u() + 1; x++) {
            for (int y = clkboxs[net].box.y.l(); y < cr_hy; y++) {
                auto it = clkMap[x][y].find(net);
                if (it == clkMap[x][y].end()) continue;
                for (auto gid : it->second) {
                    gid2Move_tmp.push_back(gid);
                    tmpDisp += abs(groups[gid].y - (cr_hy + 0.01));
                    tmpCost += abs(groups[gid].y - (cr_hy + 0.01)) * (groups[gid].IsBLE() ? 1 : nonBlePen);
                }
            }
        }

        if (tmpCost < cost && IsMoveLeg(gid2Move_tmp, cr_hy + 0.01, 2, groups)) {
            disp = tmpDisp;
            cost = tmpCost;
            pos = (-1) * (cr_hy + 0.01);
            dir = 2;
            gid2Move = gid2Move_tmp;
        }
    }

    if (box_ly < cr_ly && box_hy < cr_hy && box_hy > cr_ly) {
        // DOWN;
        double tmpDisp = 0;
        double tmpCost = 0;
        vector<int> gid2Move_tmp;

        for (int x = clkboxs[net].box.x.l(); x < (int)clkboxs[net].box.x.u() + 1; x++) {
            for (int y = cr_ly; y < (int)clkboxs[net].box.y.u() + 1; y++) {
                auto it = clkMap[x][y].find(net);
                if (it == clkMap[x][y].end()) continue;
                for (auto gid : it->second) {
                    gid2Move_tmp.push_back(gid);
                    tmpDisp += abs(groups[gid].y - (cr_ly - 0.01));
                    tmpCost += abs(groups[gid].y - (cr_ly - 0.01)) * (groups[gid].IsBLE() ? 1 : nonBlePen);
                }
            }
        }

        if (tmpCost < cost && IsMoveLeg(gid2Move_tmp, cr_ly - 0.01, 3, groups)) {
            disp = tmpDisp;
            cost = tmpCost;
            pos = (-1) * (cr_ly - 0.01);
            dir = 3;
            gid2Move = gid2Move_tmp;
        }
    }

    return cost;
}

void ClkrgnPlan::Shrink(vector<Group> &groups) {
    double totalDisp = 0;
    int nMove = 0;

    // statistic
    int nOF = 0;
    int overflow = 0;
    for (int x = 0; x < database.crmap_nx; x++) {
        for (int y = 0; y < database.crmap_ny; y++) {
            if (lgClkrgns[x][y]->nets.size() > database.CLKRGN_NCLK) {
                overflow += lgClkrgns[x][y]->nets.size();
                nOF++;
            }
        }
    }

    // printf("clock region info before planning\n");
    // for (int cr_x=0; cr_x<database.crmap_nx; cr_x++){
    //     for (int cr_y=0; cr_y<database.crmap_ny; cr_y++){
    //         printf("%2.lu ", lgClkrgns[cr_x][cr_y]->nets.size());
    //     }
    //     printf("\n");
    // }

    while (true) {
        bool hasChange = false;

        vector<pair<int, LGClkrgn *>> OFClkRgns;
        for (int x = 0; x < database.crmap_nx; x++) {
            for (int y = 0; y < database.crmap_ny; y++) {
                if (lgClkrgns[x][y]->nets.size() > database.CLKRGN_NCLK) {
                    OFClkRgns.push_back(pair<int, LGClkrgn *>(lgClkrgns[x][y]->nets.size(), lgClkrgns[x][y]));
                }
            }
        }
        sort(OFClkRgns.begin(), OFClkRgns.end(), [](const pair<int, LGClkrgn *> &a, const pair<int, LGClkrgn *> &b) {
            return a.first > b.first;
        });

        for (auto pair : OFClkRgns) {
            LGClkrgn *clkrgn = pair.second;

            // printlog(LOG_INFO,"clkrgn_X%d_Y%d (%d, %d, %d, %d) OF = %lu(%lu)",
            //                 clkrgn->clkrgn->cr_x, clkrgn->clkrgn->cr_y, clkrgn->clkrgn->lx, clkrgn->clkrgn->ly,
            //                 clkrgn->clkrgn->hx, clkrgn->clkrgn->hy, clkrgn->nets.size(), database.CLKRGN_NCLK);

            vector<int> gid2Move;
            Net *net2Move = NULL;
            double tarPos = -1;
            double cost = DBL_MAX;
            double disp = 0;

            for (auto net : clkrgn->nets) {
                vector<int> gid2Move_tmp;
                double disp_tmp = 0;
                double cost_tmp;
                double pos_tmp = -1;
                int dir = -1;

                cost_tmp = GetMove(clkboxs[net], clkrgn, groups, gid2Move_tmp, pos_tmp, disp_tmp, dir);

                if (dir != -1 && cost > cost_tmp) {  // TODO: check multiple clock legality
                    cost = cost_tmp;
                    disp = disp_tmp;
                    gid2Move = gid2Move_tmp;
                    net2Move = net;
                    tarPos = pos_tmp;
                }
            }

            if (net2Move != NULL) {
                // printlog(LOG_INFO,"%s, #move= %lu, disp= %.1f, pos=%.2f, box=(%.1f, %.1f, %.1f, %.1f)",
                //                 net2Move->name.c_str(), gid2Move.size(), disp, abs(tarPos),
                //                 clkboxs[net2Move].box.x.l(),  clkboxs[net2Move].box.y.l(),
                //                 clkboxs[net2Move].box.x.u(), clkboxs[net2Move].box.y.u());

                UpdatePl(clkrgn, gid2Move, tarPos, groups);
                totalDisp += disp;
                nMove += gid2Move.size();
                hasChange = true;
                break;
            }
        }

        if (!hasChange) {
            break;
        }
    }

    // check if there is any of clkrgn
    for (int cr_x = 0; cr_x < database.crmap_nx; cr_x++) {
        for (int cr_y = 0; cr_y < database.crmap_ny; cr_y++) {
            if (lgClkrgns[cr_x][cr_y]->nets.size() > database.CLKRGN_NCLK) {
                int lx = lgClkrgns[cr_x][cr_y]->clkrgn->lx;
                int ly = lgClkrgns[cr_x][cr_y]->clkrgn->ly;
                int hx = lgClkrgns[cr_x][cr_y]->clkrgn->hx;
                int hy = lgClkrgns[cr_x][cr_y]->clkrgn->hy;

                printlog(LOG_ERROR,
                         "clkrgn_X%d_Y%d (%d, %d, %d, %d) OF = %lu(%lu), cannot be resolve",
                         cr_x,
                         cr_y,
                         lx,
                         ly,
                         hx,
                         hy,
                         lgClkrgns[cr_x][cr_y]->nets.size(),
                         database.CLKRGN_NCLK);
            }
        }
    }

    // printf("clock region info after shrink\n");
    // for (int cr_x=0; cr_x<database.crmap_nx; cr_x++){
    //     for (int cr_y=0; cr_y<database.crmap_ny; cr_y++){
    //         printf("%2.lu ", lgClkrgns[cr_x][cr_y]->nets.size());
    //     }
    //     printf("\n");
    // }

    printlog(LOG_INFO,
             "Finish clkrgn shrinking, disp=%.1f, #move=%d, #OF=%d(%d), avgOF=%.2f(%lu)",
             totalDisp,
             nMove,
             nOF,
             overflow * 1.0 / nOF,
             database.crmap_nx * database.crmap_ny,
             database.CLKRGN_NCLK);
}

void ClkrgnPlan::Expand(vector<Group> &groups) {
    // expand to the margin of the covering clkrgn
    for (auto &pair : clkboxs) {
        ClkBox &box = pair.second;
        box.lrgn = GetClkrgn(box.box.x.l(), box.box.y.l());
        box.hrgn = GetClkrgn(box.box.x.u(), box.box.y.u());
        box.lx = box.lrgn->clkrgn->lx;
        box.ly = box.lrgn->clkrgn->ly;
        box.hx = box.hrgn->clkrgn->hx - 1;
        box.hy = box.hrgn->clkrgn->hy - 1;
    }

    // expand region
    unordered_map<Net *, vector<int>> clk2Gid;
    for (auto &group : groups) {
        for (auto net : group2Clk[group.id]) {
            clk2Gid[net].push_back(group.id);
        }
    }

    vector<pair<double, Net *>> clkDensity;
    for (auto &pair : clk2Gid) {
        ClkBox &box = clkboxs[pair.first];
        double density = (-1.0) * pair.second.size() / ((box.hx - box.lx + 1) * (box.hy - box.ly + 1));
        clkDensity.push_back(std::pair<double, Net *>(density, pair.first));
    }

    // statistic
    int nRound = 0;
    vector<pair<double, Net *>> clkDensity_beg = clkDensity;

    bool hasChange = true;
    while (hasChange) {
        sort(clkDensity.begin(), clkDensity.end());

        hasChange = false;

        for (auto &pair : clkDensity) {
            Net *net = pair.second;
            ClkBox &box = clkboxs[net];

            vector<std::pair<int, std::pair<int, int>>> dirCost;
            for (int dir = 0; dir < 4; dir++) {
                int gain = (dir < 2) ? (box.hy - box.ly + 1) : (box.hx - box.lx + 1);
                int cost = 0;

                bool isDirLeg = true;
                if (dir == 0) {
                    // right
                    if (box.hrgn->clkrgn->cr_x + 1 < database.crmap_nx) {
                        int x = box.hrgn->clkrgn->cr_x + 1;
                        gain *= (lgClkrgns[x][box.hrgn->clkrgn->cr_y]->clkrgn->hx -
                                 lgClkrgns[x][box.hrgn->clkrgn->cr_y]->clkrgn->lx);
                        for (int y = box.lrgn->clkrgn->cr_y; y <= box.hrgn->clkrgn->cr_y && isDirLeg; y++) {
                            auto it = lgClkrgns[x][y]->nets.find(net);
                            if (it == lgClkrgns[x][y]->nets.end()) {
                                if (lgClkrgns[x][y]->nets.size() == database.CLKRGN_NCLK) {
                                    isDirLeg = false;
                                } else {
                                    cost += (lgClkrgns[x][y]->nets.find(net) == lgClkrgns[x][y]->nets.end());
                                }
                            }
                        }
                    } else {
                        gain *= 0;
                    }
                } else if (dir == 1) {
                    // left
                    if (box.lrgn->clkrgn->cr_x > 0) {
                        int x = box.lrgn->clkrgn->cr_x - 1;
                        gain *= (lgClkrgns[x][box.lrgn->clkrgn->cr_y]->clkrgn->hx -
                                 lgClkrgns[x][box.lrgn->clkrgn->cr_y]->clkrgn->lx);
                        for (int y = box.lrgn->clkrgn->cr_y; y <= box.hrgn->clkrgn->cr_y && isDirLeg; y++) {
                            auto it = lgClkrgns[x][y]->nets.find(net);
                            if (it == lgClkrgns[x][y]->nets.end()) {
                                if (lgClkrgns[x][y]->nets.size() == database.CLKRGN_NCLK) {
                                    isDirLeg = false;
                                } else {
                                    cost += (lgClkrgns[x][y]->nets.find(net) == lgClkrgns[x][y]->nets.end());
                                }
                            }
                        }
                    } else {
                        gain *= 0;
                    }
                } else if (dir == 2) {
                    // up
                    if (box.hrgn->clkrgn->cr_y + 1 < database.crmap_ny) {
                        int y = box.hrgn->clkrgn->cr_y + 1;
                        gain *= (lgClkrgns[box.hrgn->clkrgn->cr_x][y]->clkrgn->hy -
                                 lgClkrgns[box.hrgn->clkrgn->cr_x][y]->clkrgn->ly);
                        for (int x = box.lrgn->clkrgn->cr_x; x <= box.hrgn->clkrgn->cr_x && isDirLeg; x++) {
                            auto it = lgClkrgns[x][y]->nets.find(net);
                            if (it == lgClkrgns[x][y]->nets.end()) {
                                if (lgClkrgns[x][y]->nets.size() == database.CLKRGN_NCLK) {
                                    isDirLeg = false;
                                } else {
                                    cost += (lgClkrgns[x][y]->nets.find(net) == lgClkrgns[x][y]->nets.end());
                                }
                            }
                        }
                    } else {
                        gain *= 0;
                    }
                } else if (dir == 3) {
                    // down
                    if (box.lrgn->clkrgn->cr_y > 0) {
                        int y = box.lrgn->clkrgn->cr_y - 1;
                        gain *= (lgClkrgns[box.lrgn->clkrgn->cr_x][y]->clkrgn->hy -
                                 lgClkrgns[box.lrgn->clkrgn->cr_x][y]->clkrgn->ly);
                        for (int x = box.lrgn->clkrgn->cr_x; x <= box.hrgn->clkrgn->cr_x && isDirLeg; x++) {
                            auto it = lgClkrgns[x][y]->nets.find(net);
                            if (it == lgClkrgns[x][y]->nets.end()) {
                                if (lgClkrgns[x][y]->nets.size() == database.CLKRGN_NCLK) {
                                    isDirLeg = false;
                                } else {
                                    cost += (lgClkrgns[x][y]->nets.find(net) == lgClkrgns[x][y]->nets.end());
                                }
                            }
                        }
                    } else {
                        gain *= 0;
                    }
                }

                if (gain != 0 && isDirLeg) {
                    gain *= -1;
                    dirCost.push_back(std::pair<int, std::pair<int, int>>(cost, std::pair<int, int>(gain, dir)));
                }
            }

            sort(dirCost.begin(), dirCost.end());
            if (!dirCost.empty()) {
                int dir = dirCost[0].second.second;

                // update colrgn, box, density vector
                if (dir == 0) {
                    // right
                    for (int y = box.lrgn->clkrgn->cr_y; y <= box.hrgn->clkrgn->cr_y; y++) {
                        lgClkrgns[box.hrgn->clkrgn->cr_x + 1][y]->nets.insert(net);
                    }
                    box.hrgn = lgClkrgns[box.hrgn->clkrgn->cr_x + 1][box.hrgn->clkrgn->cr_y];
                    box.hx = box.hrgn->clkrgn->hx - 1;
                } else if (dir == 1) {
                    // left
                    for (int y = box.lrgn->clkrgn->cr_y; y <= box.hrgn->clkrgn->cr_y; y++) {
                        lgClkrgns[box.lrgn->clkrgn->cr_x - 1][y]->nets.insert(net);
                    }
                    box.lrgn = lgClkrgns[box.lrgn->clkrgn->cr_x - 1][box.lrgn->clkrgn->cr_y];
                    box.lx = box.lrgn->clkrgn->lx;
                } else if (dir == 2) {
                    // up
                    for (int x = box.lrgn->clkrgn->cr_x; x <= box.hrgn->clkrgn->cr_x; x++) {
                        lgClkrgns[x][box.hrgn->clkrgn->cr_y + 1]->nets.insert(net);
                    }
                    box.hrgn = lgClkrgns[box.hrgn->clkrgn->cr_x][box.hrgn->clkrgn->cr_y + 1];
                    box.hy = box.hrgn->clkrgn->hy - 1;
                } else if (dir == 3) {
                    // down
                    for (int x = box.lrgn->clkrgn->cr_x; x <= box.hrgn->clkrgn->cr_x; x++) {
                        lgClkrgns[x][box.lrgn->clkrgn->cr_y - 1]->nets.insert(net);
                    }
                    box.lrgn = lgClkrgns[box.lrgn->clkrgn->cr_x][box.lrgn->clkrgn->cr_y - 1];
                    box.ly = box.lrgn->clkrgn->ly;
                }

                pair.first = (-1.0) * clk2Gid[net].size() / ((box.hx - box.lx + 1) * (box.hy - box.ly + 1));

                nRound++;
                hasChange = true;
                break;
            }
        }
    }

    // statistic
    int indx = 0;
    int nExpand = 0;
    double totExpandRate = 0;
    for (auto &pair : clk2Gid) {
        ClkBox &box = clkboxs[pair.first];
        double density = pair.second.size() * 1.0 / ((box.hx - box.lx + 1) * (box.hy - box.ly + 1));
        double ratio = abs(clkDensity_beg[indx].first) / density;
        if (ratio != 1) {
            nExpand++;
            totExpandRate += ratio;
            // cout<<pair.first->name<<" "<<abs(clkDensity_beg[indx].first)<< " "<< density<<endl;
        }
        indx++;
    }

    // printf("clock region info after expand\n");
    // for (int cr_x=0; cr_x<database.crmap_nx; cr_x++){
    //     for (int cr_y=0; cr_y<database.crmap_ny; cr_y++){
    //         printf("%2.lu ", lgClkrgns[cr_x][cr_y]->nets.size());
    //     }
    //     printf("\n");
    // }

    printlog(LOG_INFO,
             "Finish clkrgn expanding, #round=%d, #expand=%d(%lu), avgExpandRate=%.2f%%",
             nRound,
             nExpand,
             clk2Gid.size(),
             totExpandRate / nExpand * 100.0);
}

void ClkrgnPlan::Apply(vector<Group> &groups) {
    // set the lg box of groups
    for (auto &group : groups) {
        if (!group2Clk[group.id].empty()) {
            bool isSet = false;
            for (auto net : group2Clk[group.id]) {
                if (!isSet) {
                    isSet = true;
                    group.lgBox.set(clkboxs[net].hx, clkboxs[net].hy, clkboxs[net].lx, clkboxs[net].ly);
                } else {
                    int lx = max(clkboxs[net].lx, group.lgBox.lx());
                    int ly = max(clkboxs[net].ly, group.lgBox.ly());
                    int hx = min(clkboxs[net].hx, group.lgBox.ux());
                    int hy = min(clkboxs[net].hy, group.lgBox.uy());
                    group.lgBox.set(hx, hy, lx, ly);
                }
            }
        }
    }
}

LGClkrgn *ClkrgnPlan::GetClkrgn(int x, int y) {
    for (int cr_x = 0; cr_x < database.crmap_nx; cr_x++) {
        for (int cr_y = 0; cr_y < database.crmap_ny; cr_y++) {
            if (x < lgClkrgns[cr_x][cr_y]->clkrgn->lx || x >= lgClkrgns[cr_x][cr_y]->clkrgn->hx) break;
            if (!(y < lgClkrgns[cr_x][cr_y]->clkrgn->ly || y >= lgClkrgns[cr_x][cr_y]->clkrgn->hy))
                return lgClkrgns[cr_x][cr_y];
        }
    }

    return NULL;
}