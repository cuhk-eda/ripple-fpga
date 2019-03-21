#pragma once

#include "db/db.h"
using namespace db;

// squeeze out the invalid cand sites
void SqueezeCandSites(vector<Site *> &candSites, const Group &group, bool rmDup = false);
// squeeze out the duplicate and wrong type cand sites
void SqueezeCandSites(vector<Site *> &candSites, SiteType::Name type);
// squeeze out the invalid cand sites and those father from the optrgn
void SqueezeCandSites(vector<Site *> &candSites, const Group &group, Site *orgSite, Box<double> &optrgn);

Box<double> GetOptimalRegion(const pair<double, double> pinLoc,
                             const vector<Net *> &nets,
                             const vector<DynamicBox<double>> &netBox);