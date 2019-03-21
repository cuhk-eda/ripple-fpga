#include "clb2_lutpg.h"

bool LUTPairGraph::CanMerge(int i, int j) {
    if (i < j) swap(i, j);
    return graph[i].find(j) != graph[i].end();
}

int LUTPairGraph::NumPossibleFFs(const vector<Instance*>& ffs) {
    if (ffs.size() < 2) return ffs.size();
    for (auto iff = ffs.begin(); iff != ffs.end(); ++iff)
        for (auto jff = next(iff); jff != ffs.end(); ++jff)
            if (sameClk(*iff, *jff) && sameSr(*iff, *jff)) {
                return 2;
            }
    return 1;
}

Instance* LUTPairGraph::GetInst(int i) {
    if (i == -1)
        return nullptr;
    else
        return insts[i];
}

bool LUTPairGraph::AddLUT(Instance* lut, bool check) {
    // Add
    int id = insts.size();
    graph.emplace_back();
    for (size_t i = 0; i < insts.size(); ++i) {
        if (IsLUTCompatible(*insts[i], *lut)) {
            graph[id][i] = NumDupInputs(*insts[i], *lut);  // single record
        }
    }
    insts.push_back(lut);

    // Check
    if (!check) return true;
    // check num of lut
    if (insts.size() <= 8) return true;
    // use greedy method
    CommitGreedy();
    if (pairs.size() <= 8) return true;
    // use matching
    CommitMinSlots();
    if (pairs.size() <= 8) return true;

    // Fail & Recover
    graph.pop_back();
    insts.pop_back();
    pairs.clear();
    tried = 0;
    return false;
}

void LUTPairGraph::CommitGreedy() {
    if (tried == -1) {
        pairs.clear();
        tried = 0;
    }
    for (; tried < (int)insts.size(); ++tried) {
        bool matched = false;
        for (int l = pairs.size() - 1; l >= 0; --l)  // reverse order (prefer the same inst group!)
            if (pairs[l][1] == -1 && CanMerge(pairs[l][0], tried)) {
                pairs[l][1] = tried;
                matched = true;
                break;
            }
        if (!matched) pairs.push_back({tried, -1});
    }
}

void LUTPairGraph::CommitMinSlots() {
    GenGraph g(insts.size());
    for (unsigned i = 0; i < graph.size(); ++i)
        for (auto& e : graph[i]) g.AddEdge(i + 1, e.first + 1, 1);
    g.CalcMaxWeightMatch();
    vector<Match> matches = g.GetMate();
    vector<bool> got(insts.size(), false);
    pairs.clear();
    for (auto& m : matches) {
        pairs.push_back({m.u - 1, m.v - 1});
        got[m.u - 1] = true;
        got[m.v - 1] = true;
    }
    for (size_t i = 0; i < insts.size(); ++i)
        if (!got[i]) pairs.push_back({(int)i, -1});
    tried = insts.size();
}

void LUTPairGraph::CommitFast() {
    // check num of lut
    if (insts.size() <= 8) {
        pairs.clear();
        for (int i = 0; i < (int)insts.size(); ++i) pairs.push_back({i, -1});
        tried = -1;
        return;
    }
    // use greedy method
    CommitGreedy();
    if (pairs.size() <= 8) return;
    // use matching
    CommitMinSlots();
    if (pairs.size() <= 8) return;
}

void LUTPairGraph::CommitBalance(const array<vector<Instance*>, 4>& FFs) {
    unsigned bestNum = max(int(insts.size()) - 8, 0);

    // Calculate edge weights
    int scale = 2;
    int constant = 100;
    vector<vector<Instance*>> ffs(insts.size());
    for (auto& ffGroup : FFs) {
        for (auto ff : ffGroup) {
            Net* ffINet = ff->pins[database.ffIPinIdx]->net;
            for (unsigned i = 0; i < insts.size(); ++i) {
                if (insts[i]->pins[database.lutOPinIdx]->net == ffINet) {
                    ffs[i].push_back(ff);
                    break;
                }
            }
        }
    }
    vector<int> num(insts.size());
    for (unsigned i = 0; i < insts.size(); ++i) num[i] = NumPossibleFFs(ffs[i]);  // 0, 1 or 2
    GenGraph g2(insts.size());
    for (unsigned i = 0; i < graph.size(); ++i) {
        for (auto& e : graph[i]) {
            // # possible ffs: now - ori1 - ori2 (min: -2)
            // increased difficulty, ie (1,1) -> 2: -0.5
            int j = e.first;
            double loss;
            if (num[i] == 0 || num[j] == 0)
                loss = 0;
            else {
                int tot;
                if (num[i] == 2 || num[j] == 2)
                    tot = 2;
                else {
                    vector<Instance*> tmp;
                    for (auto ff : ffs[i]) tmp.push_back(ff);
                    for (auto ff : ffs[j]) tmp.push_back(ff);
                    tot = NumPossibleFFs(tmp);  // 1 or 2
                }
                loss = tot - num[i] - num[j];  // -1 (2-2-1 or 1-1-1) or -2 (2-2-2)
                if (num[i] == 1 && num[j] == 1 && tot == 2) loss -= 0.5;
            }
            // in summary, loss can be 0, -0.5, ..., -2.5
            e.second = scale * (e.second + loss - 3);  // - 3 for fewer pairs
        }
    }

    // Consider positive edges only
    GenGraph g(insts.size());
    for (unsigned i = 0; i < graph.size(); ++i)
        for (auto& e : graph[i])
            if (e.second > 0) g.AddEdge(i + 1, e.first + 1, e.second);
    g.CalcMaxWeightMatch();
    vector<Match> matches = g.GetMate();

    function<double(Match)> weight = [&](const Match& m) {
        int i = m.u - 1;
        int j = m.v - 1;
        if (i < j) swap(i, j);
        return graph[i][j];
    };

    // Shift by a large constant to prefer min cardinalty
    if (matches.size() < bestNum) {
        GenGraph g2(insts.size());
        for (unsigned i = 0; i < graph.size(); ++i)
            for (auto& e : graph[i]) {
                g2.AddEdge(i + 1, e.first + 1, e.second + scale * constant);
            }
        g2.CalcMaxWeightMatch();
        matches = g2.GetMate();
        if (matches.size() > bestNum) {
            ComputeAndSort(matches, weight, greater<int>());
            while (matches.size() > bestNum && weight(matches.back()) <= 0) matches.pop_back();
        }
    }

    // Commit
    vector<bool> got(insts.size(), false);
    pairs.clear();
    for (auto& m : matches) {
        pairs.push_back({m.u - 1, m.v - 1});
        got[m.u - 1] = true;
        got[m.v - 1] = true;
    }
    for (size_t i = 0; i < insts.size(); ++i)
        if (!got[i]) pairs.push_back({(int)i, -1});
}

void LUTPairGraph::Export(vector<array<Instance*, 2>>& LUTs) {
    assert(pairs.size() <= 8);
    LUTs.resize(pairs.size());
    for (unsigned l = 0; l < pairs.size(); ++l) {
        LUTs[l][0] = GetInst(pairs[l][0]);
        LUTs[l][1] = GetInst(pairs[l][1]);
    }
}

void LUTPairGraph::Print() {
    cout << "clb" << id << endl;
    cout << "graph" << endl;
    for (size_t i = 0; i < insts.size(); ++i) {
        printf("%d(%d): ", (int)i, (int)insts[i]->id);
        for (auto& e : graph[i]) printf("%d-%d, ", e.first, e.second);
        cout << endl;
    }
    cout << "pairs" << endl;
    for (auto& p : pairs) cout << p[0] << " " << p[1] << endl;
}