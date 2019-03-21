#ifndef _DP_DP_DATA_H_
#define _DP_DP_DATA_H_

#include "db/db.h"
#include "pack/clb.h"
#include "lgclk/lgclk_data.h"
#include "cong/cong.h"

using namespace db;

class DPData {
private:
    CongEstBB congEst;
    bool useCLB1;
    vector<Group>& groups;

public:
    ColrgnData colrgnData;
    vector<vector<CLBBase*>> clbMap;
    vector<vector<vector<int>>> groupMap;

    inline CLBBase* NewCLB() {
        if (useCLB1)
            return new CLB1;
        else
            return new CLB2;
    }

    bool IsClkMoveLeg(Group& group, Site* site);
    bool IsClkMoveLeg(Pack* pack, Site* site);
    bool IsClkMoveLeg(const vector<pair<Pack*, Site*>>& pairs);

    bool WorsenCong(Site* src, Site* dst);

    DPData(vector<Group>& groups);
    ~DPData();
};

#endif