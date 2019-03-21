#ifndef _LG_CHAIN_H_
#define _LG_CHAIN_H_

#include "db/db.h"
#include "./lg_data.h"
#include "./lg_main.h"
#include "dp/dp_main.h"

using namespace db;

class ChainFinder {
private:
    vector<Group> &groups;

    const unsigned depth = 5;
    const unsigned breadth = 35;
    double maxTotDisp;
    double maxDisp;

    vector<vector<CLBBase *>> clbMap;
    vector<bool> inChain;

    bool checkColrgn;

    void Backtrack(int gid2Move);

public:
    vector<int> gidChain;
    vector<CLBBase *> clbChain;
    vector<vector<int>> orderChain;
    vector<int> posChain;
    vector<unordered_map<Net *, int>> colrgnChain;

    bool DFS(LGData &lgData, ChainmoveTarget optTarget);
    bool Init(Group &group, LGData &lgData);

    bool DFS(DPBle &dpBle);
    void Init(Group &group, DPBle &dpBle);

    ChainFinder(vector<Group> &_groups);
};

#endif