#pragma once

#include "clb2.h"

class FFLoc {
public:
    unsigned g, s;  // group id, slot id (inside the group)
    FFLoc(unsigned gid, unsigned sid) : g(gid), s(sid) {}
    void Print();
};

class FFGroup {
public:
    vector<Instance*> insts;  // lazy deletion will be used
    Instance* type = nullptr;
    bool assigned = false;  // at least one FF is assigned
    Instance*& operator[](size_t i) { return insts[i]; }
    const Instance* operator[](size_t i) const { return insts[i]; }
    size_t size() { return insts.size(); }
    void Print();
};

class LUTPair {
public:
    array<Instance*, 2> insts;  // LUT pair
    vector<FFLoc> ffs;          // FFs (may more than two)
    bool assigned = false;
    void Print();
};

class CLB2ResultBase {
public:
    CLB2ResultBase(const CLB2& c, vector<Instance*>& r);
    virtual void Run() = 0;
    void Print();
    CLB2Stat stat;

protected:
    const CLB2& clb;
    vector<FFGroup> ffgs;
    vector<LUTPair> lutps;
    // result
    array<unsigned, 2> lutSlots;  // first half: 0-7lut, 16-23ff; second half: 8-15lut, 24-31ff
    vector<Instance*>& res;

    // LUT pair slot    =           half*8 + lutSlots[half]
    // FF slot          =   16 +    half*8 + lutSlots[half] + 0/1
    // define: shift    =           half*8 + lutSlots[half]
    unsigned GetShift(unsigned half);
    void CommitFF(const FFLoc& ff, unsigned slot, bool conn = false);
    void CommitLUTPair(LUTPair& l2f, unsigned slot);

    void InitStat();
    void InitLUTPairs();
    virtual void Prepare() = 0;
};