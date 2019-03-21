#pragma once

#include "../global.h"
#include "../db/db.h"

using namespace db;

class LGHfcol;
class LGColrgn;
class LGClkrgn;

class LGHfcol {
public:
    HfCol *col;
    LGColrgn *colrgn;
    LGClkrgn *clkrgn;
    unordered_set<Net *> nets;

    LGHfcol(HfCol *_col, vector<vector<unordered_map<Net *, unordered_set<int>>>> &clkMap);

    void Update(vector<vector<unordered_map<Net *, unordered_set<int>>>> &clkMap);
};

class LGColrgn {
public:
    int lx, hx, ly, hy;
    vector<LGHfcol *> cols;
    unordered_set<Net *> nets;

    LGColrgn(int _lx, int _ly, int _hx, int _hy, const vector<vector<LGHfcol *>> &lgHfcols);

    void Update();
};

class LGClkrgn {
public:
    ClkRgn *clkrgn;
    unordered_set<Net *> nets;

    LGClkrgn(ClkRgn *_clkrgn);
};

class ClkBox {
public:
    Net *net;
    int lx, ly, hx, hy;
    LGClkrgn *lrgn, *hrgn;

    DynamicBox<double> box;

    vector<LGClkrgn *> GetClkrgns(vector<vector<LGClkrgn *>> &lgClkrgns);
};

class ColrgnData {
public:
    vector<vector<LGHfcol *>> lgHfcols;
    vector<vector<LGColrgn *>> lgColrgns;
    int colrgn_nx, colrgn_ny;

    vector<unordered_set<Net *>> group2Clk;

    vector<vector<unordered_map<Net *, unordered_set<int>>>> clkMap;
    void Init(vector<Group> &groups, const vector<int> &inst2Gid);
    void Clear();
    void Update(vector<int> &gid2Move, vector<Site *> &sites, vector<Group> &groups);

    LGColrgn *GetColrgn(int x, int y);
    unordered_map<Net *, int> GetColrgnData(int x, int y);

    bool IsClkMoveLeg(const vector<int> &gid2Move, Site *site, vector<Group> &groups, bool strict = true);
    bool IsClkMoveLeg(const vector<vector<int>> &gid2Move, const vector<Site *> &sites, vector<Group> &groups);
};