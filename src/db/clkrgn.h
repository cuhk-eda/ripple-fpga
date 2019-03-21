#pragma once

#include "../global.h"
#include "net.h"

namespace db {

class Site;
class ClkRgn;
class HfCol;

class ClkRgn {
public:
    string name;
    int lx, ly, hx, hy;
    int cr_x, cr_y;

    std::vector<Site*> sites;

    unordered_set<Net*> clknets;
    vector<vector<HfCol*>> hfcols;

    ClkRgn(string _name, int _lx, int _ly, int _hx, int _hy, int _cr_x, int _cr_y);
    void Init();
};

class HfCol {
public:
    int x, ly, hy;
    vector<Site*> sites;
    unordered_set<Net*> clknets;

    ClkRgn* clkrgn;

    HfCol(int _x, int _lx, int _hy, ClkRgn* _clkrgn);
    void Init();
};
}  // namespace db
