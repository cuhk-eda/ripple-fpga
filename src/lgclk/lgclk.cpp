#include "lgclk.h"
#include "lgclk_colrgn.h"

void ClkLegalize(vector<Group> &groups) {
    if (database.crmap_nx == 0) return;
    printlog(LOG_INFO, "");
    printlog(LOG_INFO, " = = = = column region legalization = = = = ");

    double begHpwl = database.getHPWL();

    ColrgnLG colrgnLG;

    colrgnLG.Init(groups);
    colrgnLG.RunLG(groups);
    colrgnLG.Clear();

    double aftHpwl = database.getHPWL();
    printlog(LOG_INFO,
             "WL Before LG: %.0f, WL After LG:%.0f, Gap: %.2f%%",
             begHpwl,
             aftHpwl,
             (aftHpwl - begHpwl) / begHpwl * 100);

    return;
}