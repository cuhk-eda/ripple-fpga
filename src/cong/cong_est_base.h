#pragma once

#include "db/db.h"

class CongEstBase {
public:
    vector<vector<double>> siteDemand;
    int noNeedRoute = 0;
    const double minDem = 0.00001;  // ignore sites with demand smaller than this
    // supply under the most optimistic assumption
    const int horiSup = 208;   // (24*1+16*2+8*6) * 2
    const int vertiSup = 256;  // (21*1+14*2+2*3+8*4+7*5+1*6) + (20*1+18*2+8*4+8*5)

    void Run(bool stat = true);

protected:
    bool IsLUT2FF(db::Net* net);  // ignore nets from lut-o to ff-d
    virtual void InitDemand() = 0;
    void StatDemand() const;
};

template <typename T>
void SwMapToSiteMap(vector<vector<T>>& swMap, vector<vector<T>>& siteMap) {
    assert((int)swMap.size() == db::database.switchbox_nx && (int)swMap[0].size() == db::database.switchbox_ny);
    siteMap = vector<vector<double>>(db::database.sitemap_nx, vector<double>(db::database.sitemap_ny, 0));
    for (int x = 0; x < db::database.sitemap_nx; ++x)
        for (int y = 0; y < db::database.sitemap_ny; ++y) siteMap[x][y] = swMap[db::SWCol(x)][y];
}