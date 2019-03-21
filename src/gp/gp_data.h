#ifndef _GP_DATA_H_
#define _GP_DATA_H_

#include "db/db.h"
#include "gp_spread.h"

using namespace db;

// BASICS
extern int numNodes;
extern int numCells;
extern int numMacros;
extern int numPads;

extern int numNets;
extern int numPins;
extern int numFences;

extern double coreLX, coreLY;
extern double coreHX, coreHY;

extern vector<vector<int> > cellNet;
extern vector<vector<int> > netCell;

extern vector<double> netWeight;

extern vector<double> cellX;
extern vector<double> cellY;
extern vector<double> cellW;
extern vector<double> cellH;

// LOWER BOUND
// last positions
extern vector<double> lastVarX;
extern vector<double> lastVarY;
// nearest fence region
extern vector<double> cellFenceX;
extern vector<double> cellFenceY;
extern vector<double> cellFenceDist;
extern vector<int> cellFenceRect;
extern vector<int> cellFence;

// UPPER BOUND
// local target density
extern vector<vector<double> > binTD;
extern int numDBinX;
extern int numDBinY;
extern double DBinW;
extern double DBinH;
// legalization bin
extern vector<vector<vector<LGBin> > > LGGrid;  // information of each bin    [fence][binx][biny]
extern vector<vector<double> > LGTD;            // target density of each bin [binx][biny]
extern vector<list<OFBin> > OFBins;             // [fence][bin]
extern vector<vector<vector<char> > > OFMap;    // mark if a bin is handled   [fence][binx][biny]
extern int LGNumX;
extern int LGNumY;
extern double LGBinW;
extern double LGBinH;
extern int LGBinSize;  // in number of rows
// area
extern vector<double> totalFArea;
extern vector<double> totalUArea;
extern vector<double> totalCArea;
// region
class FRect {
public:
    double lx, ly;
    double hx, hy;
    FRect();
    FRect(double lx, double ly, double hx, double hy);
    ~FRect();
};
class FRegion {
public:
    SiteType *type;
    vector<FRect> rects;
    FRegion();
    FRegion(SiteType *type);

    void addRect(FRect rect);
};
extern vector<FRegion> regions;

void gp_copy_in(vector<Group> &group, bool layout);
void gp_copy_out();

double hpwl(double *wx = NULL, double *wy = NULL);

#endif
