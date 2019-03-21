#include "dp_data.h"

using namespace db;

extern bool legCLB1Succ;  // true;

DPData::DPData(vector<Group>& _groups) : groups(_groups) {
    useCLB1 = legCLB1Succ;

    // build clb map and group map
    clbMap.assign(database.sitemap_nx, vector<CLBBase*>(database.sitemap_ny, NULL));
    for (int x = 0; x < database.sitemap_nx; x++) {
        for (int y = 0; y < database.sitemap_ny; y++) {
            if (database.getSite(x, y)->type->name == SiteType::SLICE) {
                clbMap[x][y] = NewCLB();
            }
        }
    }
    groupMap.resize(database.sitemap_nx, vector<vector<int>>(database.sitemap_ny));
    for (unsigned g = 0; g < groups.size(); g++) {
        int gx = groups[g].x;
        int gy = groups[g].y;
        groupMap[gx][gy].push_back(groups[g].id);
        if (groups[g].IsBLE()) {
            if (!clbMap[gx][gy]->AddInsts(groups[g])) assert(false);
        }
    }

    vector<int> inst2Gid(database.instances.size(), -1);
    for (const auto& group : groups) {
        for (unsigned i = 0; i < group.instances.size(); i++) {
            if (group.instances[i] != NULL) {
                inst2Gid[group.instances[i]->id] = group.id;
            }
        }
    }
    colrgnData.Init(groups, inst2Gid);

    congEst.Run();
}

DPData::~DPData() {
    for (int x = 0; x < database.sitemap_nx; x++) {
        for (int y = 0; y < database.sitemap_ny; y++) {
            if (database.getSite(x, y)->type->name == SiteType::SLICE) {
                delete clbMap[x][y];
            }
        }
    }

    vector<Group> resultGroups;
    for (int x = 0; x < database.sitemap_nx; x++) {
        for (int y = 0; y < database.sitemap_ny; y++) {
            for (auto gid : groupMap[x][y]) {
                resultGroups.push_back(groups[gid]);
                resultGroups[resultGroups.size() - 1].id = resultGroups.size() - 1;
            }
        }
    }
    groups = move(resultGroups);
}

bool DPData::IsClkMoveLeg(Group& group, Site* site) {
    vector<int> gid2Move = {group.id};

    return colrgnData.IsClkMoveLeg(gid2Move, site, groups);
}

bool DPData::IsClkMoveLeg(Pack* pack, Site* site) {
    vector<int> gid2Move = groupMap[pack->site->cx()][pack->site->cy()];

    return colrgnData.IsClkMoveLeg(gid2Move, site, groups);
}

bool DPData::IsClkMoveLeg(const vector<pair<Pack*, Site*>>& pairs) {
    vector<vector<int>> gid2Move(pairs.size());
    vector<Site*> sites(pairs.size());

    for (unsigned i = 0; i < pairs.size(); i++) {
        gid2Move[i] = groupMap[pairs[i].first->site->cx()][pairs[i].first->site->cy()];
        sites[i] = pairs[i].second;
    }

    return colrgnData.IsClkMoveLeg(gid2Move, sites, groups);
}

bool DPData::WorsenCong(Site* src, Site* dst) {
    // auto d = congEst.siteDemand[dst->x][dst->y];
    // return 	d > 500 && d > congEst.siteDemand[src->x][src->y];
    return false;
}