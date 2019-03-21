#pragma once

#include "clb_base.h"
#include "alg/matching.h"

class LUTPairGraph {
private:
    int id;
    vector<Instance*> insts;
    vector<unordered_map<int, int>> graph;  // id -> num of shared inputs
    vector<array<int, 2>> pairs;            // -1 for empty
    int tried = 0;                          // insts [0, tried] have tried

    bool CanMerge(int i, int j);
    int NumPossibleFFs(const vector<Instance*>& ffs);
    Instance* GetInst(int i);

public:
    LUTPairGraph(int i) { id = i; }
    bool AddLUT(Instance* lut, bool check = true);
    void CommitGreedy();
    void CommitMinSlots();
    void CommitFast();
    void CommitBalance(const array<vector<Instance*>, 4>& FFs);
    void Export(vector<array<Instance*, 2>>& LUTs);
    void Print();
};