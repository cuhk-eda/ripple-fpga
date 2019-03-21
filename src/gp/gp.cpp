#include "../global.h"

#include "gp_data.h"
#include "gp_main.h"
#include "gp.h"

void gplace(vector<Group> &groups) {
    printlog(LOG_INFO, "");
    gp_copy_in(groups, true);
    GP_Main();
    gp_copy_out();
}
