#ifndef _SPREAD_H_
#define _SPREAD_H_

class LGBin {
public:
    double lx, ly, hx, hy;
    vector<int> cells;
    double uArea;  // usable area
    double fArea;  // free area
    double cArea;  // cell area
    LGBin() {
        lx = ly = hx = hy = -1;
        uArea = fArea = cArea = 0;
    }
    LGBin(double lx, double ly, double hx, double hy) {
        this->lx = lx;
        this->ly = ly;
        this->hx = hx;
        this->hy = hy;
        uArea = fArea = cArea = 0;
    }
};

class OFBin {
public:
    int x, y;  // overfill center
    int region;
    double ofArea;
    double ofRatio;
    double aofArea;
    double aofRatio;
    int lx, ly;  // expanded region
    int hx, hy;  // expanded region
    double exCArea;
    double exUArea;
    double exFArea;
    OFBin(int _r, int _x, int _y) {
        lx = hx = x = _x;
        ly = hy = y = _y;
        region = _r;
        ofArea = 0.0;
        ofRatio = 0.0;
        aofArea = 0.0;
        aofRatio = 0.0;
        exCArea = 0.0;
        exFArea = 0.0;
        exUArea = 0.0;
    }
    OFBin() {
        lx = hx = x = -1;
        ly = hy = y = -1;
        region = -1;
        ofArea = 0.0;
        ofRatio = 0.0;
        aofArea = 0.0;
        aofRatio = 0.0;
        exCArea = 0.0;
        exFArea = 0.0;
        exUArea = 0.0;
    }

    bool operator<(const OFBin &b) const { return ofRatio > b.ofRatio; }
};

void spreadCells(int binSize, int times);

#endif
