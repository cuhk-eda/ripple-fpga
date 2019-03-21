#ifndef _LG_H_
#define _LG_H_

#include "db/db.h"

using namespace db;

enum lgSiteOrder { SITE_HPWL, SITE_HPWL_SMALL_WIN, SITE_ALIGN };

enum lgGroupOrder { DEFAULT, GROUP_LGBOX };

enum lgRetrunGroup { GET_SLICE, UPDATE_XY, UPDATE_XY_ORDER, NO_UPDATE };

enum lgPackMethod { USE_CLB1, USE_CLB2 };

void planning(vector<Group> &groups);
void legalize(vector<Group> &groups, lgSiteOrder siteOrder, lgRetrunGroup retGroup);
void legalize_partial(vector<Group> &groups);

#endif
