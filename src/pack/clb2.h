#pragma once

#include "clb_base.h"
#include "clb2_lutpg.h"

class CLB2ResultBase;
class CLB2Result1;
class CLB2Result2;

class CLB2 : public CLBBase {
    friend CLB2ResultBase;
    friend CLB2Result1;
    friend CLB2Result2;

private:
    vector<array<Instance*, 2>> LUTs;  // max 8
    LUTPairGraph LUTs2;
    array<vector<Instance*>, 4> FFs;  // max 4 for each
    unsigned numLUT = 0, numFF = 0;
    bool AddInsts(const vector<Instance*>& insts);
    void BreakLUTPairs();

public:
    CLB2(int i) : CLBBase(i), LUTs2(i) {}
    CLB2() : CLBBase(), LUTs2(id) {}
    ~CLB2(){};
    bool AddInst(Instance* inst);
    bool AddInsts(const Group& group) { return AddInsts(group.instances); }
    inline bool IsEmpty() { return numLUT == 0 && numFF == 0; }
    void GetResult(Group& group);  // const; TODO: make it const
    void GetFinalResult(Group& group);
    void Print() const;
};

class CLB2Stat {
public:
    unsigned numConn, numImposs, numBroken;
    void Init();
    void Print();
    const CLB2Stat& operator+=(const CLB2Stat& rhs);
};

class CLB2TotStat {
public:
    unsigned numEasyClb, numTotClb, numShare;
    CLB2Stat r1, r2, tot;
    void Init();
    void Print();
};

extern CLB2TotStat clb2stat;