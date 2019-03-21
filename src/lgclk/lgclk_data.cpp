#include "lgclk_data.h"

LGHfcol::LGHfcol(HfCol *_col, vector<vector<unordered_map<Net *, unordered_set<int>>>> &clkMap) {
    col = _col;

    colrgn = NULL;
    clkrgn = NULL;

    for (int y = col->ly; y < col->hy; y++) {
        for (auto &pair : clkMap[col->x][y]) {
            nets.insert(pair.first);
        }
    }
}

void LGHfcol::Update(vector<vector<unordered_map<Net *, unordered_set<int>>>> &clkMap) {
    nets.clear();

    for (int y = col->ly; y < col->hy; y++) {
        for (auto &pair : clkMap[col->x][y]) {
            nets.insert(pair.first);
        }
    }
}

LGColrgn::LGColrgn(int _lx, int _ly, int _hx, int _hy, const vector<vector<LGHfcol *>> &lgHfcols) {
    lx = _lx;
    hx = _hx;
    ly = _ly;
    hy = _hy;

    cols.push_back(lgHfcols[lx][ly / 30]);
    cols.push_back(lgHfcols[lx + 1][ly / 30]);

    for (auto col : cols) {
        col->colrgn = this;
        for (auto &net : col->nets) {
            nets.insert(net);
        }
    }
}

void LGColrgn::Update() {
    nets.clear();

    for (auto col : cols) {
        for (auto &net : col->nets) {
            nets.insert(net);
        }
    }
}

LGClkrgn::LGClkrgn(ClkRgn *_clkrgn) { clkrgn = _clkrgn; }

vector<LGClkrgn *> ClkBox::GetClkrgns(vector<vector<LGClkrgn *>> &lgClkrgns) {
    vector<LGClkrgn *> clkrgns;

    for (int x = 0; x < database.crmap_nx; x++) {
        for (int y = 0; y < database.crmap_ny; y++) {
            if (box.x.intersect(lgClkrgns[x][y]->clkrgn->lx, lgClkrgns[x][y]->clkrgn->hx) &&
                box.y.intersect(lgClkrgns[x][y]->clkrgn->ly, lgClkrgns[x][y]->clkrgn->hy)) {
                if (!(box.x.l() == lgClkrgns[x][y]->clkrgn->hx || box.y.l() == lgClkrgns[x][y]->clkrgn->hy)) {
                    clkrgns.push_back(lgClkrgns[x][y]);
                }
            }
        }
    }

    return clkrgns;
}

void ColrgnData::Init(vector<Group> &groups, const vector<int> &inst2Gid) {
    if (database.crmap_nx == 0) return;

    int nClknet = 0;
    colrgn_nx = 0, colrgn_ny = database.crmap_ny * 2;

    for (int x = 0; x < database.crmap_nx; x++) {
        colrgn_nx += (database.clkrgns[x][0]->hx - database.clkrgns[x][0]->lx) / 2;
    }

    lgHfcols.assign(database.sitemap_nx, vector<LGHfcol *>(2 * database.crmap_ny, NULL));
    lgColrgns.assign(colrgn_nx, vector<LGColrgn *>(colrgn_ny, NULL));

    clkMap.assign(database.sitemap_nx, vector<unordered_map<Net *, unordered_set<int>>>(database.sitemap_ny));
    group2Clk.assign(groups.size(), unordered_set<Net *>());

    for (auto net : database.nets) {
        if (net->isClk) {
            nClknet += net->isClk;
            for (auto pin : net->pins) {
                if (pin->type->type != 'o' && pin->instance != NULL) {
                    Group &group = groups[inst2Gid[pin->instance->id]];
                    if (group2Clk[group.id].find(net) == group2Clk[group.id].end()) {
                        clkMap[group.x][group.y][net].insert(group.id);
                        group2Clk[group.id].insert(net);
                    }
                }
            }
        }
    }

    // printlog(LOG_INFO,"#clk=%d", nClknet);

    for (int y = 0; y < colrgn_ny; y++) {
        for (int x = 0; x < database.sitemap_nx; x++) {
            if (database.hfcols[x][y] != NULL) {
                lgHfcols[x][y] = new LGHfcol(database.hfcols[x][y], clkMap);
            }
        }
    }

    for (int y = 0; y < database.crmap_ny; y++) {
        for (int x = 0, n = 0; x < database.crmap_nx; x++) {
            int lx = database.clkrgns[x][y]->lx;
            int hx = database.clkrgns[x][y]->hx;
            int offset = (hx - lx) & 1;

            for (int colx = lx + offset; colx < hx; colx += 2) {
                for (int coly = 0; coly < 2; coly++) {
                    lgColrgns[n][coly + y * 2] =
                        new LGColrgn(colx, y * 60 + coly * 30, colx + 2, y * 60 + (coly + 1) * 30, lgHfcols);
                }
                n++;
            }
        }
    }
}

void ColrgnData::Clear() {
    for (int y = 0; y < colrgn_ny; y++) {
        for (int x = 0; x < database.sitemap_nx; x++) {
            if (database.hfcols[x][y] != NULL) {
                delete lgHfcols[x][y];
            }
        }
    }

    for (int x = 0; x < colrgn_nx; x++) {
        for (int y = 0; y < colrgn_ny; y++) {
            delete lgColrgns[x][y];
        }
    }
}

LGColrgn *ColrgnData::GetColrgn(int x, int y) { return lgHfcols[x][y / 30]->colrgn; }

unordered_map<Net *, int> ColrgnData::GetColrgnData(int x, int y) {
    unordered_map<Net *, int> data;

    LGColrgn *colrgn = GetColrgn(x, y);

    for (int x = colrgn->lx; x < colrgn->hx; x++) {
        for (int y = colrgn->ly; y < colrgn->hy; y++) {
            for (auto &pair : clkMap[x][y]) data[pair.first] += pair.second.size();
        }
    }

    return data;
}

bool ColrgnData::IsClkMoveLeg(const vector<int> &gid2Move, Site *site, vector<Group> &groups, bool strict) {
    if (database.crmap_nx == 0) return true;

    for (auto gid : gid2Move) {
        Group &group = groups[gid];
        if (!group.InClkBox(site)) return false;
    }

    LGColrgn *colrgn = GetColrgn(site->cx(), site->cy());

    unordered_set<Net *> nets = colrgn->nets;

    unsigned nClk_beg = nets.size();

    for (auto gid : gid2Move) {
        for (auto net : group2Clk[gid]) {
            nets.insert(net);
        }
    }

    unsigned nClk_aft = nets.size();

    if (nClk_aft <= database.COLRGN_NCLK || (!strict && nClk_aft == nClk_beg))
        return true;
    else
        return false;
}

bool ColrgnData::IsClkMoveLeg(const vector<vector<int>> &gid2Move, const vector<Site *> &sites, vector<Group> &groups) {
    if (database.crmap_nx == 0) return true;

    for (unsigned i = 0; i < gid2Move.size(); i++) {
        for (auto gid : gid2Move[i]) {
            Group &group = groups[gid];
            if (!group.InClkBox(sites[i])) return false;
        }
    }

    unordered_map<LGColrgn *, unordered_map<Net *, int>> clkCount;

    for (unsigned i = 0; i < gid2Move.size(); i++) {
        LGColrgn *tarColrgn = GetColrgn(sites[i]->cx(), sites[i]->cy());
        if (tarColrgn != NULL) {
            if (clkCount.find(tarColrgn) == clkCount.end()) {
                for (int x = tarColrgn->lx; x < tarColrgn->hx; x++) {
                    for (int y = tarColrgn->ly; y < tarColrgn->hy; y++) {
                        for (const auto &pair : clkMap[x][y]) {
                            clkCount[tarColrgn][pair.first] += pair.second.size();
                        }
                    }
                }
            }
        }

        for (auto gid : gid2Move[i]) {
            LGColrgn *srcColrgn = GetColrgn(groups[gid].x, groups[gid].y);

            if (srcColrgn != NULL) {
                if (clkCount.find(srcColrgn) == clkCount.end()) {
                    for (int x = srcColrgn->lx; x < srcColrgn->hx; x++) {
                        for (int y = srcColrgn->ly; y < srcColrgn->hy; y++) {
                            for (const auto &pair : clkMap[x][y]) {
                                clkCount[srcColrgn][pair.first] += pair.second.size();
                            }
                        }
                    }
                }

                for (auto net : group2Clk[gid]) clkCount[srcColrgn][net]--;
            }

            if (tarColrgn != NULL) {
                for (auto net : group2Clk[gid]) clkCount[tarColrgn][net]++;
            }
        }
    }

    for (auto &pairs : clkCount) {
        unsigned count = 0;

        for (auto &pair : pairs.second) {
            if (pair.second != 0) count++;
        }

        if (count > database.COLRGN_NCLK) return false;
    }

    return true;
}

void ColrgnData::Update(vector<int> &gid2Move, vector<Site *> &sites, vector<Group> &groups) {
    if (database.crmap_nx == 0) return;

    unordered_set<LGColrgn *> colrgn2Update;
    unordered_set<LGHfcol *> hfcol2Update;

    for (unsigned i = 0; i < gid2Move.size(); i++) {
        int gid = gid2Move[i];

        int srcX = groups[gid].x, srcY = groups[gid].y;
        int destX = sites[i]->cx(), destY = sites[i]->cy();

        for (auto net : group2Clk[gid]) {
            clkMap[srcX][srcY][net].erase(clkMap[srcX][srcY][net].find(gid));
            clkMap[destX][destY][net].insert(gid);

            if (clkMap[srcX][srcY][net].empty()) clkMap[srcX][srcY].erase(clkMap[srcX][srcY].find(net));
        }

        hfcol2Update.insert(lgHfcols[srcX][srcY / 30]);
        hfcol2Update.insert(lgHfcols[destX][destY / 30]);
        colrgn2Update.insert(GetColrgn(srcX, srcY));
        colrgn2Update.insert(GetColrgn(destX, destY));
    }

    for (auto col : hfcol2Update) col->Update(clkMap);
    for (auto colrgn : colrgn2Update) colrgn->Update();
}