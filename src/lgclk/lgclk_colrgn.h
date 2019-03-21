#pragma once

#include "lgclk_data.h"
#include "../global.h"
#include "../db/db.h"
#include "../pack/clb.h"

using namespace db;

class MapUpdater {
public:
    vector<CLBBase *> updatedClbs;
    vector<vector<int>> updatedGroups;
    vector<Site *> sites;

    void Update(vector<vector<CLBBase *>> &clbMap, vector<vector<vector<int>>> &groupMap);
};

class ColrgnLG {
private:
    vector<vector<CLBBase *>> clbMap;
    vector<vector<vector<int>>> groupMap;

    vector<int> inst2Gid;
    vector<int> groupIds;

    ColrgnData colrgnData;

    void InitGroupInfo(vector<Group> &groups);
    void InitPlInfo(vector<Group> &groups);

    void GetResult(vector<Group> &groups);

    bool IsClkMovable(
        Net *net, LGColrgn *srcColrgn, vector<Group> &groups, vector<int> &gid2Move, MapUpdater &srcUpdater);
    double GetMove(vector<int> &gid2Move,
                   LGColrgn *srcColrgn,
                   vector<Group> &groups,
                   MapUpdater &destUpdater,
                   double &disp,
                   const double curMin);
    void Update(vector<int> &gid2Move, MapUpdater &srcUpdater, MapUpdater &destUpdater, vector<Group> &groups);

public:
    void Init(vector<Group> &groups);
    void Clear();

    void RunLG(vector<Group> &groups);
};