#include "global.h"
#include "pack.h"
#include "clb2.h"

extern bool legCLB1Succ;

void packble(vector<Group>& groups) {
    printlog(LOG_INFO, "*******************************************************************");
    PackBLE packble(groups);
    printlog(LOG_INFO, "*******************************************************************");
    packble.PairLUTFF();
    printlog(LOG_INFO, "*******************************************************************");
    packble.PairLUTs();
    printlog(LOG_INFO, "*******************************************************************");
    packble.PackSingFF();
    printlog(LOG_INFO, "*******************************************************************");
    packble.GetResult(groups);
}

void maxLutFfCon(const DPData& dpData) {
    if (legCLB1Succ) return;

    clb2stat.Init();

    log() << endl;
    log() << "Reallocate LUTs/FFs in each CLB to maximize LUT-FF direct connection" << endl;
    for (int x = 0; x < database.sitemap_nx; ++x) {
        for (int y = 0; y < database.sitemap_ny; ++y) {
            auto site = database.getSite(x, y);
            if (site->type->name != SiteType::SLICE || site->pack == NULL)
                continue;
            else {
                for (auto inst : site->pack->instances)
                    if (inst) database.unplace(inst);
            }
            Group res;
            dpData.clbMap[x][y]->GetFinalResult(res);
            auto& insts = res.instances;
            for (unsigned i = 0; i < insts.size(); ++i)
                if (insts[i] != NULL) database.place(insts[i], site, i);
        }
    }

    clb2stat.Print();

    if (!database.isPlacementValid()) printlog(LOG_ERROR, "max lut ff failed");
}