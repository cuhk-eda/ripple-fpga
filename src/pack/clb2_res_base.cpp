#include "clb2_res_base.h"

void FFLoc::Print() { printf("g%ds%d ", g, s); }

void FFGroup::Print() {
    printf("FF group with %ld FFs: ", insts.size());
    if (insts.size() != 0) {
        printf("(clk%p, sr%p, ce%p): ",
               type->pins[database.clkPinIdx]->net,
               type->pins[database.srPinIdx]->net,
               type->pins[database.cePinIdx]->net);
        for (auto ff : insts)
            if (ff != nullptr)
                printf("%d, ", ff->id);
            else
                printf("(moved out), ");
    }
    printf("\n");
}

void LUTPair::Print() {
    printf("LUT pair (%d(%d)", insts[0]->id, insts[0]->master->name);
    if (insts[1] != nullptr) printf(", %d(%d)", insts[1]->id, insts[1]->master->name);
    printf("): ");
    for (auto ff : ffs) ff.Print();
    printf("\n");
}

CLB2ResultBase::CLB2ResultBase(const CLB2& c, vector<Instance*>& r) : clb(c), res(r) {
    assert(c.LUTs.size() <= 8);
    assert(c.FFs[0].size() <= 4);
    assert(res.size() == 0);
    res.resize(33, nullptr);
}

void CLB2ResultBase::Print() {
    printf("clb#%d\n", clb.id);
    for (auto& lutp : lutps) lutp.Print();
    for (auto& ffg : ffgs) ffg.Print();
    printf("res:\n");
    for (unsigned i = 0; i < res.size(); ++i) {
        if (res[i])
            printf("%d(%d), ", res[i]->id, res[i]->master->name);
        else
            printf("xxxxx(x), ");
        if (i % 8 == 7) printf("\n");
    }
    printf("\n");
    stat.Print();
}

unsigned CLB2ResultBase::GetShift(unsigned half) {
    assert(lutSlots[half] <= 6);
    unsigned shift = half * 8 + lutSlots[half];
    lutSlots[half] += 2;
    return shift;
}

void CLB2ResultBase::CommitFF(const FFLoc& ff, unsigned slot, bool conn) {
    res[slot] = ffgs[ff.g][ff.s];
    ffgs[ff.g][ff.s] = nullptr;
    ffgs[ff.g].assigned = true;
    if (conn) --stat.numBroken;
}

void CLB2ResultBase::CommitLUTPair(LUTPair& lutp, unsigned slot) {
    res[slot] = lutp.insts[0];
    res[slot + 1] = lutp.insts[1];
    lutp.assigned = true;
}

void CLB2ResultBase::InitStat() {
    stat.numConn = 0;
    stat.numImposs = 0;
    for (const auto& lutp : lutps) {
        stat.numConn += lutp.ffs.size();
        if (lutp.ffs.size() >= 2) {
            unsigned nff = 1;
            for (auto iff = lutp.ffs.begin(); iff != lutp.ffs.end() && nff == 1; ++iff)
                for (auto jff = next(iff); jff != lutp.ffs.end(); ++jff)
                    if (sameClk(ffgs[iff->g][iff->s], ffgs[jff->g][jff->s]) &&
                        sameSr(ffgs[iff->g][iff->s], ffgs[jff->g][jff->s])) {
                        nff = 2;
                        break;
                    }
            stat.numImposs += lutp.ffs.size() - nff;
        }
    }
    stat.numBroken = stat.numConn;
}

void CLB2ResultBase::InitLUTPairs() {
    assert(ffgs.size() != 0);
    lutSlots = {0, 0};
    lutps.resize(clb.LUTs.size());
    for (unsigned i = 0; i < clb.LUTs.size(); ++i) lutps[i].insts = clb.LUTs[i];
    for (unsigned g = 0; g < ffgs.size(); ++g)
        for (unsigned s = 0; s < ffgs[g].size(); ++s) {
            Net* ffINet = ffgs[g][s]->pins[database.ffIPinIdx]->net;
            for (auto& lutp : lutps) {
                if (lutp.insts[0]->pins[database.lutOPinIdx]->net == ffINet ||
                    (lutp.insts[1] != nullptr && lutp.insts[1]->pins[database.lutOPinIdx]->net == ffINet)) {
                    lutp.ffs.emplace_back(g, s);
                    break;
                }
            }
        }
    InitStat();
}