#include "clb2.h"
#include "clb2_res1.h"
#include "clb2_res2.h"

extern vector<bool> with2FF;

// #define USE_LUTPG

bool CLB2::AddInst(Instance* inst) {
    if (inst->IsFF()) {  // FF
        for (unsigned i = 0; i < 4; i += 2) {
            if (FFs[i].size() == 0) {
                FFs[i].push_back(inst);
                ++numFF;
                return true;
            } else if (sameClk(FFs[i][0], inst) && sameSr(FFs[i][0], inst)) {  // clk, sr
                for (unsigned j = 0; j < 2; ++j) {
                    unsigned k = i + j;
                    if (FFs[k].size() == 0 || (sameCe(FFs[k][0], inst) && FFs[k].size() < 4)) {  // ce
                        FFs[k].push_back(inst);
                        ++numFF;
                        return true;
                    }
                }
            }
        }
    } else if (inst->IsLUT()) {  // LUT
#ifdef USE_LUTPG
        if (LUTs2.AddLUT(inst, true)) {
            ++numLUT;
            return true;
        }
#else
        if (!with2FF[inst->id]) {
            for (int i = LUTs.size() - 1; i >= 0; --i)  // reverse order (prefer the same inst group!)
                if (LUTs[i][1] == nullptr && IsLUTCompatible(*LUTs[i][0], *inst)) {
                    LUTs[i][1] = inst;
                    ++numLUT;
                    return true;
                }
        }
        if (LUTs.size() < 8) {
            array<Instance*, 2> lutPair = {inst, nullptr};
            LUTs.push_back(lutPair);
            ++numLUT;
            return true;
        }
#endif
    } else {  // others (invalid)
        printlog(LOG_ERROR, "invalid instance type");
    }
    return false;
}

bool CLB2::AddInsts(const vector<Instance*>& insts) {
    CLB2 clbBackup(*this);
    for (auto inst : insts)
        if (!AddInst(inst)) {
            *this = clbBackup;
            return false;
        }
    return true;
}

void CLB2::BreakLUTPairs() {
    // sort by FF number
    vector<pair<int, int>> luts;  // (ff num, idx)
    for (unsigned l = 0; l < LUTs.size(); ++l) luts.emplace_back(0, l);
    for (auto& ffGroup : FFs)
        for (auto ff : ffGroup) {
            Net* ffINet = ff->pins[database.ffIPinIdx]->net;
            for (unsigned l = 0; l < LUTs.size(); ++l) {
                if (LUTs[l][0]->pins[database.lutOPinIdx]->net == ffINet ||
                    (LUTs[l][1] != nullptr && LUTs[l][1]->pins[database.lutOPinIdx]->net == ffINet)) {
                    ++luts[l].first;
                    break;
                }
            }
        }
    sort(luts.begin(), luts.end(), [](const pair<int, int>& a, const pair<int, int>& b) { return a.first > b.first; });
    auto LUTsCopy = move(LUTs);
    LUTs.resize(LUTsCopy.size());
    for (unsigned l = 0; l < luts.size(); ++l) LUTs[l] = LUTsCopy[luts[l].second];
    // seperate
    for (unsigned i = 0; i < LUTs.size() && LUTs.size() < 8; ++i) {
        if (LUTs[i][1] && NumDupInputs(*LUTs[i][0], *LUTs[i][1]) == 0) {
            LUTs.push_back({LUTs[i][1], nullptr});
            LUTs[i][1] = nullptr;
        }
    }
}

int groupShifts[4] = {16, 17, 24, 25};

void CLB2::GetResult(Group& group) {
    auto& res = group.instances;
    res.assign(33, nullptr);
    if (IsEmpty()) return;
#ifdef USE_LUTPG
    LUTs2.CommitFast();
    LUTs2.Export(LUTs);
#endif
    for (unsigned l = 0; l < LUTs.size(); ++l) {
        res[2 * l] = LUTs[l][0];
        res[2 * l + 1] = LUTs[l][1];
    }
    for (unsigned g = 0; g < 4; ++g) {
        int shift = groupShifts[g];
        for (auto ff : FFs[g]) {
            res[shift] = ff;
            shift += 2;
        }
    }
}

int numShare(vector<array<Instance*, 2>>& LUTs) {
    int num = 0;
    for (auto& lutp : LUTs)
        if (lutp[1] != nullptr) num += NumDupInputs(*lutp[0], *lutp[1]);
    return num;
}

void CLB2::GetFinalResult(Group& group) {
    if (IsEmpty()) return;

#ifndef USE_LUTPG
    for (auto& lutp : LUTs)
        for (auto lut : lutp)
            if (lut != nullptr) LUTs2.AddLUT(lut);
#endif
    LUTs2.CommitBalance(FFs);
    LUTs2.Export(LUTs);

    //     BreakLUTPairs();

    clb2stat.numShare += numShare(LUTs);

    CLB2Result1 res(*this, group.instances);
    res.Run();
    ++clb2stat.numTotClb;
    if (res.stat.numBroken > res.stat.numImposs) {
        group.instances.clear();
        CLB2Result2 res2(*this, group.instances);
        res2.Run();
        clb2stat.tot += res2.stat;
    } else {
        ++clb2stat.numEasyClb;
        clb2stat.tot += res.stat;
    }
}

void CLB2::Print() const {
    printf("clb#%d\n", id);
    printf("%ld LUT pairs (%d LUTs): ", LUTs.size(), numLUT);
    for (const auto& lutPair : LUTs) {
        printf("(%d:%d", lutPair[0]->id, lutPair[0]->master->name);
        if (lutPair[1] != nullptr) printf(", %d:%d", lutPair[1]->id, lutPair[1]->master->name);
        printf(")");
    }
    printf("\n");
    for (unsigned i = 0; i < 4; ++i) {
        printf("%dth ffgroup with %ld FFs ", i, FFs[i].size());
        if (FFs[i].size() != 0) {
            printf("(clk%p, sr%p, ce%p): ",
                   FFs[i][0]->pins[database.clkPinIdx]->net,
                   FFs[i][0]->pins[database.srPinIdx]->net,
                   FFs[i][0]->pins[database.cePinIdx]->net);
            for (auto ff : FFs[i]) printf("%d, ", ff->id);
        }
        printf("\n");
    }
}

void CLB2Stat::Init() {
    numConn = 0;
    numImposs = 0;
    numBroken = 0;
}
void CLB2Stat::Print() {
    cout << "totConn = " << numConn << " brokenConn = " << numBroken << " impossConn = " << numImposs << endl;
}
const CLB2Stat& CLB2Stat::operator+=(const CLB2Stat& rhs) {
    numConn += rhs.numConn;
    numImposs += rhs.numImposs;
    numBroken += rhs.numBroken;
    return *this;
}

void CLB2TotStat::Init() {
    numEasyClb = 0;
    numTotClb = 0;
    numShare = 0;
    r1.Init();
    r2.Init();
    tot.Init();
}
void CLB2TotStat::Print() {
    log() << "easyClb = " << numEasyClb << " totClb = " << numTotClb << " share = " << numShare << endl;
    log() << "r1  : ";
    r1.Print();
    log() << "r2  : ";
    r2.Print();
    log() << "tot : ";
    tot.Print();
}

CLB2TotStat clb2stat;