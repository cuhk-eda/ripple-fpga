#include "lg.h"
#include "lg_main.h"
#include "../lgclk/lgclk.h"
#include "../lgclk/lgclk_plan.h"

bool legCLB1Succ = false;  // true;

void legalize(vector<Group> &groups, lgSiteOrder siteOrder, lgRetrunGroup retGroup) {
    printlog(LOG_INFO, " = = = = legalization = = = = ");

    double begHpwl = 0;

    Legalizer legalizer(groups);
    legalizer.Init(USE_CLB2);

    begHpwl = legalizer.GetHpwl();

    if (!legalizer.RunAll(siteOrder, DEFAULT)) {
        // perform lg with SortGroupsByLGBox
        legalizer.Init(USE_CLB2);
        legalizer.RunAll(siteOrder, GROUP_LGBOX);
    }

    legalizer.GetResult(retGroup);
    double aftHpwl = legalizer.GetHpwl();
    printlog(LOG_INFO,
             "WL Before LG: %.0f, WL After LG:%.0f, Gap: %.2f%%",
             begHpwl,
             aftHpwl,
             (aftHpwl - begHpwl) / begHpwl * 100);

    if (retGroup == UPDATE_XY_ORDER) {
        ClkLegalize(groups);
        if (!database.isPlacementValid()) printlog(LOG_ERROR, "legalization failed");
    }
}

void legalize_partial(vector<Group> &groups) {
    printlog(LOG_INFO, "");
    printlog(LOG_INFO, " = = = = legalize DSP and BRAM = = = = ");
    Legalizer legalizer(groups);
    legalizer.Init(USE_CLB2);

    double begHpwl = legalizer.GetHpwl();
    legalizer.RunPartial();
    legalizer.GetResult(UPDATE_XY);
    double aftHpwl = legalizer.GetHpwl();
    printlog(LOG_INFO,
             "WL Before LG: %.0f, WL After LG:%.0f, Gap: %.2f%%",
             begHpwl,
             aftHpwl,
             (aftHpwl - begHpwl) / begHpwl * 100);
}

void planning(vector<Group> &groups) {
    if (database.crmap_nx == 0) return;
    printlog(LOG_INFO, "");
    printlog(LOG_INFO, " = = = = clock region planning = = = = ");

    ClkrgnPlan plan;

    plan.Init(groups);
    plan.Shrink(groups);
    plan.Expand(groups);
    plan.Apply(groups);
}