#ifndef _GP_SETTING_H_
#define _GP_SETTING_H_

#include "global.h"

enum LBMode { LBModeSimple = 1, LBModeFenceBBox = 2, LBModeFenceRect = 3 };

class GPSetting {
public:
    int nThreads;

    int initIter;
    int mainWLIter;
    int mainCongIter;
    int finalIter;
    int lbIter;
    int ubIter;
    double pseudoNetWeightBegin;
    double pseudoNetWeightEnd;
    bool degreeNetWeight;
    double ioNetWeight;
    double expandBoxRatio;
    double expandBoxLimit;

    double pseudoNetWeightRatioX;
    double pseudoNetWeightRatioY;

    double lutArea, ffArea, dspArea, ramArea;  // standard area for lut and ff
    double areaScale;

    bool enableFence;
    bool packed;
    bool useLastXY;

    void setDefault();
    void init();
    void set0();
    void set1();
    void set2();
    void set2c();
    void set3();
    void set3c();
    void print();
};

extern GPSetting gpSetting;

#endif
