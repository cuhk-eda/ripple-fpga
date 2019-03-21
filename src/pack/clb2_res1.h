#pragma once

#include "clb2.h"
#include "clb2_res_base.h"

class CLB2Result1 : public CLB2ResultBase {
    using CLB2ResultBase::CLB2ResultBase;

public:
    void Run();

private:
    // Major steps
    void Prepare();
    void Commit2FF(LUTPair& l2f, FFLoc& ff1, FFLoc& ff2);
    void Commit1FF(LUTPair& l2f, FFLoc& ff);

    // Small gadgets
    bool CanMoveOrSwapIntoGroup(unsigned g, Instance* ff);
    bool CanMoveOrSwapIntoHalf(unsigned h, Instance* ff);
    void MoveFF(FFLoc& ff, unsigned tarG);
    bool SwapFF(FFLoc& ff, unsigned tarG);
    bool MoveOrSwapIntoGroup(FFLoc& ff, unsigned tarG);
    bool SwapGroups(unsigned srcG, unsigned tarG);
    void CommitEasyFF(Instance* ff, unsigned slot);  // for easy mode only
};