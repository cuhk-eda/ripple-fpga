#include "gp_setting.h"
#include "../db/db.h"
#include "../global.h"

using namespace db;

GPSetting gpSetting;

void GPSetting::init() {
    unsigned ffPerSlice = 9, lutPerSlice = 12;
    unsigned numFF = 0, numLUT6 = 0, numLUT15 = 0;
    for (auto inst : database.instances) {
        if (inst->IsFF())
            ++numFF;
        else if (inst->IsLUT()) {
            if (inst->master->name == Master::LUT6)
                ++numLUT6;
            else
                ++numLUT15;
        }
    }
    unsigned numLUT = 2 * numLUT6 + numLUT15;
    lutArea = double(max(numFF / ffPerSlice, numLUT / lutPerSlice)) / (numFF + numLUT);
    ffArea = lutArea;
    printlog(LOG_INFO, "LUTs require %d slices, FFs require %d slices", numLUT / lutPerSlice, numFF / ffPerSlice);
    printlog(LOG_INFO,
             "LUT area is %lf ( x %d = %lf), FF area is %lf ( x %d = %lf)",
             lutArea,
             lutPerSlice,
             lutPerSlice * lutArea,
             ffArea,
             ffPerSlice,
             ffPerSlice * ffArea);
    dspArea = 2.5;
    ramArea = 5.0;

    if (database.instances.size() < 10000)
        areaScale = (database.crmap_nx == 0) ? 1.2 : 1.1;
    else
        areaScale = (database.crmap_nx == 0) ? 1.4 : 1.1;

    setDefault();
}

void GPSetting::setDefault() {
#ifdef SINGLE_THREAD
    nThreads = 1;
#else
    nThreads = 2;
#endif
    initIter = 0;
    mainWLIter = 0;
    mainCongIter = 0;
    finalIter = 3;
    lbIter = 4;
    ubIter = 3;
    pseudoNetWeightBegin = 0.1;
    pseudoNetWeightEnd = 2.0;
    degreeNetWeight = true;
    ioNetWeight = 1.0;
    expandBoxRatio = 0.0;
    expandBoxLimit = 0.2;
    pseudoNetWeightRatioX = 1.2;
    pseudoNetWeightRatioY = 0.6;
    enableFence = false;
    packed = false;
    useLastXY = false;
}

void GPSetting::set0() {
    setDefault();
    initIter = 6;
    mainWLIter = 2;
    pseudoNetWeightBegin = 0.1;
    pseudoNetWeightEnd = 0.5;
}
void GPSetting::set1() {
    setDefault();
    mainWLIter = 6;
    pseudoNetWeightBegin = 0.1;
    pseudoNetWeightEnd = 2.0;
}
void GPSetting::set2() {
    setDefault();
    mainWLIter = 10;
    mainWLIter = (database.crmap_nx == 0) ? 10 : 30;
    pseudoNetWeightBegin = 0.2;
    pseudoNetWeightEnd = 3.0;
}
void GPSetting::set2c() {
    setDefault();
    mainWLIter = 3;
    pseudoNetWeightBegin = 2.0;
    pseudoNetWeightEnd = 4.0;
    useLastXY = true;
}
void GPSetting::set3() {
    setDefault();
    mainWLIter = (database.crmap_nx == 0) ? 10 : 20;
    pseudoNetWeightBegin = 2.0;
    pseudoNetWeightEnd = 6.0;
    useLastXY = true;
}
void GPSetting::set3c() {
    setDefault();
    mainWLIter = 3;
    pseudoNetWeightBegin = 3.0;
    pseudoNetWeightEnd = 5.0;
    useLastXY = true;
}

inline string bool2str(bool b) { return b ? "true" : "false"; }

void GPSetting::print() {
    printlog(LOG_INFO, "nThreads    : %d", nThreads);
    printlog(LOG_INFO, "initIter    : %d", initIter);
    printlog(LOG_INFO, "mainWLIter  : %d", mainWLIter);
    printlog(LOG_INFO, "mainCongIter: %d", mainCongIter);
    printlog(LOG_INFO, "finalIter   : %d", finalIter);
    printlog(LOG_INFO, "areaScale   : %lf", areaScale);
    printlog(LOG_INFO, "pseudoNetWeightBegin : %lf", pseudoNetWeightBegin);
    printlog(LOG_INFO, "pseudoNetWeightEnd   : %lf", pseudoNetWeightEnd);
    printlog(LOG_INFO, "degreeNetWeight      : %lf", degreeNetWeight);
    printlog(LOG_INFO, "ioNetWeight          : %lf", ioNetWeight);
    printlog(LOG_INFO, "enableFence : %s", bool2str(enableFence).c_str());
    printlog(LOG_INFO, "packed      : %s", bool2str(packed).c_str());
    printlog(LOG_INFO, "useLastXY   : %s", bool2str(useLastXY).c_str());
}
