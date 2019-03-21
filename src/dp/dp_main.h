#ifndef _DP_DP_MAIN_H_
#define _DP_DP_MAIN_H_

#include "db/db.h"
#include "pack/clb.h"
#include "./dp_data.h"

using namespace db;

class DPPack {
private:
    vector<vector<Pack*>> netToPacks;
    vector<vector<Net*>> packToNets;
    vector<DynamicBox<double>> netBox;

    vector<Group>& groups;
    DPData& dpData;

    void UpdatePackInfo();
    void PartialUpdate(Pack* curPack, Site* targetSite, bool updateClk = true);
    void UpdateGroupXY(Site* site);
    void UpdateDpData(const vector<pair<Pack*, Site*>>& pairs, SiteType::Name type);

    double GetHpwl();

    void SwapPack(Site* curSite, Site* candSite);
    double TrySwap(Site* curSite, Site* candSite);
    void ClkAndCongFilter(Pack* pack, vector<Site*>& candSites);

    void GlobalPackSwap(SiteType::Name type);
    void BipartiteDP(SiteType::Name type, int minNumSites);

public:
    DPPack(vector<Group>& _groups, DPData& _dpData);
    void Run();
};

class DPBle {
private:
    vector<vector<int>> netToGids;
    vector<Group>& groups;

    void PartialUpdate(Group& group, Site* targetSite, bool updateClk = true);
    void ChainUpdate(vector<int>& path, vector<CLBBase*>& clbChain, vector<vector<int>>& lgorderChain);
    void UpdateGroupInfo();
    void DumpToDB();
    bool IsGroupMovable(Group& group, CLBBase*& sourceCLB, vector<int>& sourceGroup);
    bool MoveGroupToSite(Site* site, Group& group, CLBBase*& sourceCLB, vector<int>& sourceGroup);

    double GetHpwl();

    void SortCandSitesByAlign(Group& group, vector<Site*>& candSites);
    void SortCandSitesByDisp(Group& group, vector<Site*>& candSites);

    Site* MoveTowardOptrgn(Group& group, Box<double>& optrgn, CLBBase*& sourceCLB, vector<int>& sourceGroup);
    bool OptrgnChainMove(Group& group);
    void GlobalEleSwap();
    void GlobalEleMove();

public:
    vector<vector<Net*>> groupToNets;
    vector<DynamicBox<double>> netBox;
    DPData& dpData;

    DPBle(vector<Group>& _groups, DPData& _dpData);
    void Run();
};

#endif