#include "db/db.h"
#include "dp_main.h"
#include "dp_data.h"
#include "pack/pack.h"
#include "global.h"

using namespace db;

void dplace(vector<Group> &groups, const int numIter) {
    srand(0);  // fix the seed for consistence

    printlog(LOG_INFO, "");
    printlog(LOG_INFO, " = = = = begin detailed placement = = = = ");

    DPData dpData(groups);
    DPPack dpPack(groups, dpData);
    DPBle dpBle(groups, dpData);

    double originHpwl = database.getHPWL(), befHpwl, aftHpwl = 0, finalHpwl;
    for (int i = 0; i < numIter; i++) {
        befHpwl = (i == 0) ? originHpwl : database.getHPWL();

        dpBle.Run();
        dpPack.Run();

        aftHpwl = database.getHPWL();
        double improveRatio = (befHpwl - aftHpwl) * 100.0 / befHpwl;
        printlog(LOG_INFO,
                 "dp iter %d:\tbefHpwl=%.0lf, aftHpwl=%.0lf, impr_rate=%.3lf%%",
                 i,
                 befHpwl,
                 aftHpwl,
                 improveRatio);
        printlog(LOG_INFO, "***********");

        if (abs(improveRatio) < 0.1) {
            break;
        }
    }

    finalHpwl = aftHpwl;
    printlog(LOG_INFO,
             "WL Before DP: %.0lf, WL After DP: %.0lf, improveRatio: %.3lf%%",
             originHpwl,
             finalHpwl,
             (originHpwl - finalHpwl) * 100.0 / originHpwl);

    if (!database.isPlacementValid()) {
        printlog(LOG_ERROR, "dp not legal");
    }

    maxLutFfCon(dpData);
}