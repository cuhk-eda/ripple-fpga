#ifndef _LG_MAIN_H_
#define _LG_MAIN_H_

#include "db/db.h"
#include "./lg_data.h"
using namespace db;

enum ChainmoveTarget { DISP_OPT, MAX_DISP_OPT };

class Legalizer {
private:
    lgSiteOrder siteOrder;
    lgGroupOrder groupOrder;

    vector<Group> &groups;

    bool MergeGroupToSite(Site *site, Group &group, bool fixDspRam = true);
    bool AssignPackToSite(Site *site, Group &group);

    void SortCandSitesByHpwl(vector<Site *> &candSites, const Group &group);
    void SortCandSitesByPins(vector<Site *> &candSites, const Group &group);
    void SortCandSitesByAlign(vector<Site *> &candSites, const Group &group);
    void SortGroupsByGroupsize();
    void SortGroupsByPins();
    void SortGroupsByLGBox();
    void SortGroupsByOptRgnDist();

    void BipartiteLeg(SiteType::Name type, int minNumSites);
    int ChainMove(Group &group, ChainmoveTarget optTarget);

public:
    LGData lgData;

    Legalizer(vector<Group> &_groups);
    void Init(lgPackMethod packMethod);
    void GetResult(lgRetrunGroup retGroup);

    bool RunAll(lgSiteOrder siteOrder, lgGroupOrder groupOrder);
    void RunPartial();

    double GetHpwl();
};

#endif
