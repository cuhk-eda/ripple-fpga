#pragma once

#include "../global.h"
#include "../db/db.h"
#include "lgclk_data.h"

using namespace db;

class ClkrgnPlan {
private:
    vector<vector<LGClkrgn *>> lgClkrgns;

    vector<vector<unordered_map<Net *, unordered_set<int>>>> clkMap;

    vector<int> inst2Gid;
    vector<int> groupIds;
    vector<unordered_set<Net *>> group2Clk;

    void InitGroupInfo(vector<Group> &groups);
    void InitClkInfo(vector<Group> &groups);

    double GetMove(ClkBox &clkbox,
                   LGClkrgn *clkrgn,
                   vector<Group> &groups,
                   vector<int> &gid2Move,
                   double &pos,
                   double &disp,
                   int &dir);
    bool IsMoveLeg(vector<int> &gid2Move, double pos, int dir, vector<Group> &groups);
    void UpdatePl(LGClkrgn *clkrgn, vector<int> &gid2Move, double &tarPos, vector<Group> &groups);

    void PrintStat(vector<Group> &groups);

    LGClkrgn *GetClkrgn(int x, int y);

public:
    unordered_map<Net *, ClkBox> clkboxs;

    void Init(vector<Group> &groups);
    void Shrink(vector<Group> &groups);
    void Expand(vector<Group> &groups);
    void Apply(vector<Group> &groups);
};