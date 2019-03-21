#include "clb2_res1.h"

void CLB2Result1::Run() {
    Prepare();

    if (CanMoveOrSwapIntoGroup(1, ffgs[0].type) && CanMoveOrSwapIntoGroup(2, ffgs[0].type) &&
        CanMoveOrSwapIntoGroup(3, ffgs[0].type)) {  // Easy: all ffs are of the same type (TODO: remove)
        for (unsigned i = 0; i < lutps.size(); ++i) {
            unsigned shift = 2 * i;
            // lut
            CommitLUTPair(lutps[i], shift);
            // ff
            unsigned num = min(2u, (unsigned)lutps[i].ffs.size());
            for (unsigned j = 0; j < num; ++j) CommitFF(lutps[i].ffs[j], shift + 16 + j, true);
        }
        // left ff
        unsigned shift = 16;
        for (auto& ffg : ffgs)
            for (auto& ff : ffg.insts)
                if (ff != nullptr) {
                    while (res[shift] != nullptr) ++shift;
                    CommitEasyFF(ff, shift);
                }
    } else {  // Difficult: 3 steps
        // Step 1: commit lut pair (with >=2ff) to 2ff
        auto first2ff = lutps.begin();  // first 2-ff ble
        while (first2ff != lutps.end() && first2ff->ffs.size() < 2) ++first2ff;
        for (auto it = first2ff; it != lutps.end(); ++it) {
            for (auto iff = it->ffs.begin(); iff != it->ffs.end() && !it->assigned; ++iff)
                for (auto jff = next(iff); jff != it->ffs.end() && !it->assigned; ++jff) Commit2FF(*it, *iff, *jff);
        }
        // Step 2: commit lut pair (with >=1ff) to 1 ff
        auto first1ff = lutps.begin();
        while (first1ff != lutps.end() && first1ff->ffs.size() < 1) ++first1ff;
        for (auto it = first1ff; it != lutps.end(); ++it) {
            for (auto ff = it->ffs.begin(); ff != it->ffs.end() && !it->assigned; ++ff) Commit1FF(*it, *ff);
        }
        // Step 3: commit left lut pair
        for (auto& lutp : lutps)
            if (!lutp.assigned) {
                CommitLUTPair(lutp, GetShift(lutSlots[0] / 8));
            }
        // Step 4: commit left ff
        for (unsigned g = 0; g < 4; ++g)
            for (unsigned s = 0; s < ffgs[g].size(); ++s)
                if (ffgs[g][s] != nullptr) {
                    unsigned half = g / 2;
                    unsigned initSlot = 16 + half * 8 + g % 2, slot = initSlot;
                    while (res[slot] != nullptr) slot += 2;
                    assert(slot - initSlot <= 6);
                    CommitEasyFF(ffgs[g][s], slot);
                }
    }

    // move single LUT to odd postion
    for (unsigned i = 0; i < 16; i += 2)
        if (res[i] != nullptr && res[i + 1] == nullptr) {
            res[i + 1] = res[i];
            res[i] = nullptr;
        }

    // update clb2stat
    clb2stat.r1 += stat;
}

//*******************************************************************************

void CLB2Result1::Prepare() {
    // init ffgs
    ffgs.resize(4);
    for (unsigned i = 0; i < 4; ++i) {
        if (clb.FFs[i].size() > 0) {
            ffgs[i].insts = clb.FFs[i];
            ffgs[i].type = clb.FFs[i][0];
        }
    }
    // evenly distribute for same-type group
    for (unsigned g1 = 0; g1 < 4; ++g1)
        for (unsigned g2 = g1 + 1; g2 < 4; ++g2)
            if (ffgs[g1].size() != 0 && ffgs[g2].size() != 0 && sameCSC(ffgs[g1][0], ffgs[g2][0]))
                while (ffgs[g1].size() - ffgs[g2].size() >= 2) {
                    ffgs[g2].insts.push_back(ffgs[g1].insts.back());
                    ffgs[g1].insts.pop_back();
                }
    // init lutps
    InitLUTPairs();
    sort(lutps.begin(), lutps.end(), [](const LUTPair& lhs, const LUTPair& rhs) {
        return lhs.ffs.size() < rhs.ffs.size();
    });
}

unsigned compGroup[4] = {1, 0, 3, 2};

void CLB2Result1::Commit2FF(LUTPair& lutp, FFLoc& ff1, FFLoc& ff2) {
    if (!(sameClk(ffgs[ff1.g].type, ffgs[ff2.g].type) && sameSr(ffgs[ff1.g].type, ffgs[ff2.g].type))) return;
    if (ff1.g > ff2.g) swap(ff1, ff2);  // make sure that ff1.g <= ff2.g
    bool get = false;
    // Put the two ffgs together by moving or swapping
    if ((ff1.g == 0 && ff2.g == 1) || (ff1.g == 2 && ff2.g == 3)) {  // case 1: in paired FF groups (done directly)
        get = true;
    } else if (ff1.g == ff2.g) {    // case 2: in the same FF group
        if (ffgs[2].size() == 0) {  // case 2.1: move into the empty half (FF groups 2&3)
            MoveFF(ff1, 2);
            MoveFF(ff2, 3);
            get = true;
        } else {  // case 2.2: try moving/swapping FF within the same half
            get = MoveOrSwapIntoGroup(ff1, compGroup[ff1.g]);
        }
    } else if (ff1.g / 2 == 0 && ff2.g / 2 == 1)  // note: all clk, sr are the same
    {                                             // case 3: in unpaired FF groups
        // case 3.1: try moving/swapping one FF into the other half
        FFLoc* ffs[2] = {&ff1, &ff2};
        for (int i = 0; i < 2; ++i) {
            auto& ff = *ffs[i];
            unsigned tarG = compGroup[ffs[1 - i]->g];
            if ((get = MoveOrSwapIntoGroup(ff, tarG))) break;
        }
        if (!get) {  // case 3.2: try swapping one FF group with a group in the other half
            unsigned srcGroups[2] = {ff2.g, ff1.g};  // ff2 first
            for (int i = 0; i < 2; ++i) {
                if ((get = SwapGroups(srcGroups[i], compGroup[srcGroups[1 - i]]))) break;
            }
        }
    }
    // Commit the result
    if (get) {
        if (ff1.g > ff2.g) swap(ff1, ff2);
        unsigned shift = GetShift(ff1.g / 2);
        CommitLUTPair(lutp, shift);
        CommitFF(ff1, 16 + shift, true);
        CommitFF(ff2, 17 + shift, true);
    }
}

void CLB2Result1::Commit1FF(LUTPair& lutp, FFLoc& ff) {
    bool get = false;
    unsigned half = ff.g / 2;
    if (lutSlots[half] <= 6) {  // case 1: no full (done directly)
        get = true;
    } else {  // case 2: full
        half = 1 - half;
        assert(lutSlots[half] <= 6);
        // case 2.1: try moving/swapping the FFLoc into the other half
        for (unsigned tarG = half * 2; tarG < half * 2 + 2; ++tarG) {
            if ((get = MoveOrSwapIntoGroup(ff, tarG))) break;
        }
        CLB2Result1 clb2copy(*this);
        if (!get && CanMoveOrSwapIntoHalf(
                        half, ffgs[ff.g].type)) {  // case 2.2: try swapping the FF group with a group in the other half
            for (unsigned tarG = half * 2; tarG < half * 2 + 2; ++tarG) {
                if ((get = SwapGroups(ff.g, tarG))) break;
            }
        }
    }
    // Commit the result
    if (get) {
        unsigned shift = GetShift(half);
        CommitLUTPair(lutp, shift);
        CommitFF(ff, 16 + shift + ff.g % 2, true);
    }
}

//*******************************************************************************

inline bool CLB2Result1::CanMoveOrSwapIntoGroup(unsigned g, Instance* ff) {
    return ffgs[g].size() == 0 || sameCSC(ffgs[g].type, ff);
}

inline bool CLB2Result1::CanMoveOrSwapIntoHalf(unsigned h, Instance* ff) {
    unsigned g = 2 * h;
    return ffgs[g].size() == 0 || (sameClk(ffgs[g].type, ff) && sameSr(ffgs[g].type, ff));
}

void CLB2Result1::MoveFF(FFLoc& ff, unsigned tarG) {
    // update ffgs
    ffgs[tarG].type = ffgs[ff.g].type;
    ffgs[tarG].insts.push_back(ffgs[ff.g][ff.s]);
    ffgs[ff.g][ff.s] = nullptr;
    // update ff
    ff.g = tarG;
    ff.s = ffgs[tarG].size() - 1;
}

bool CLB2Result1::SwapFF(FFLoc& ff, unsigned tarG) {
    unsigned tarI = 0;
    while (tarI < ffgs[tarG].size() && ffgs[tarG][tarI] == nullptr) tarI++;
    if (tarI < ffgs[tarG].size()) {
        // update ffgs
        swap(ffgs[tarG][tarI], ffgs[ff.g][ff.s]);
        // update lutps
        for (auto& lutp : lutps)
            for (auto& f : lutp.ffs)
                if (f.g == tarG && f.s == tarI) {
                    f = ff;  // may not exist
                    break;
                }
        // update ff
        ff.g = tarG;
        ff.s = tarI;
        return true;
    }
    return false;
}

bool CLB2Result1::MoveOrSwapIntoGroup(FFLoc& ff, unsigned tarG) {
    if (!CanMoveOrSwapIntoHalf(tarG / 2, ffgs[ff.g].type)) return false;
    if (!CanMoveOrSwapIntoGroup(tarG, ffgs[ff.g].type)) return false;
    if (ffgs[tarG].size() < 4) {
        MoveFF(ff, tarG);
        return true;
    } else
        return SwapFF(ff, tarG);
}

bool CLB2Result1::SwapGroups(unsigned srcG, unsigned tarG) {
    if (ffgs[srcG].assigned || ffgs[tarG].assigned) return false;
    swap(ffgs[srcG], ffgs[tarG]);
    for (auto& lutp : lutps)
        for (auto& ff : lutp.ffs) {
            if (ff.g == srcG)
                ff.g = tarG;
            else if (ff.g == tarG)
                ff.g = srcG;
        }
    return true;
}

void CLB2Result1::CommitEasyFF(Instance* ff, unsigned slot) { res[slot] = ff; }