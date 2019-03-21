#include "clkrgn.h"

#include "db.h"
using namespace db;

ClkRgn::ClkRgn(string _name, int _lx, int _ly, int _hx, int _hy, int _cr_x, int _cr_y) {
    name = _name;
    lx = _lx;
    ly = _ly;
    hx = _hx + 1;
    hy = _hy + 1;
    cr_x = _cr_x;
    cr_y = _cr_y;

    hfcols.assign(hx - lx, vector<HfCol*>(2, NULL));
    for (int x = lx; x < hx; x++) {
        for (int y = 0; y < 2; y++) {
            database.hfcols[x][ly / 30 + y] = new HfCol(x, ly + y * 30, ly + (y + 1) * 30, this);
            hfcols[x - lx][y] = database.hfcols[x][ly / 30 + y];
        }
    }
}

void ClkRgn::Init() {
    for (int x = lx; x < hx; x++) {
        for (int y = ly; y < hy; y++) {
            sites.push_back(database.getSite(x, y));
        }
    }
    for (int x = lx; x < hx; x++) {
        for (int y = 0; y < 2; y++) {
            hfcols[x - lx][y]->Init();
        }
    }
}

HfCol::HfCol(int _x, int _ly, int _hy, ClkRgn* _clkrgn) {
    x = _x;
    ly = _ly;
    hy = _hy;
    clkrgn = _clkrgn;
}

void HfCol::Init() {
    for (int y = ly; y < hy; y++) {
        sites.push_back(database.getSite(x, y));
    }
}