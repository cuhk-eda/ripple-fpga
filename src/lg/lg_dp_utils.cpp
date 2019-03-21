#include "lg_dp_utils.h"

void SqueezeCandSites(vector<Site*>& candSites, const Group& group, bool rmDup) {
    if (!rmDup) {
        // squeeze out the invalid cand sites
        unsigned num = 0;
        for (unsigned i = 0; i < candSites.size(); ++i) {
            if (group.IsTypeMatch(candSites[i]) && group.InClkBox(candSites[i])) candSites[num++] = candSites[i];
        }
        candSites.resize(num);
    } else {
        // squeeze out the duplicate and invalid cand sites
        unordered_set<Site*> sites;

        for (auto site : candSites) {
            if (group.IsTypeMatch(site) && group.InClkBox(site)) sites.insert(site);
        }
        candSites.clear();
        for (auto site : sites) candSites.push_back(site);
        sort(candSites.begin(), candSites.end(), [](Site* a, Site* b) {
            return a->x < b->x || (a->x == b->x && a->y < b->y);
        });  // avoid machine diff
    }
}

// squeeze out the duplicate and wrong type cand sites
void SqueezeCandSites(vector<Site*>& candSites, SiteType::Name type) {
    unordered_set<Site*> sites;
    for (auto site : candSites) {
        if (site->type->name == type) sites.insert(site);
    }
    candSites.clear();
    for (auto site : sites) candSites.push_back(site);
    sort(candSites.begin(), candSites.end(), [](Site* a, Site* b) {
        return a->x < b->x || (a->x == b->x && a->y < b->y);
    });  // avoid machine diff
}

// squeeze out the invalid cand sites and those father from the optrgn
void SqueezeCandSites(vector<Site*>& candSites, const Group& group, Site* orgSite, Box<double>& optrgn) {
    double orgDist2Optrgn = optrgn.udist(orgSite->cx(), orgSite->cy());

    unsigned num = 0;
    for (unsigned i = 0; i < candSites.size(); ++i) {
        if (group.IsTypeMatch(candSites[i]) && group.InClkBox(candSites[i])) {
            double tarDist2Optrgn = optrgn.udist(candSites[i]->cx(), candSites[i]->cy());

            if (orgDist2Optrgn >= tarDist2Optrgn) {
                candSites[num++] = candSites[i];
            }
        }
    }
    candSites.resize(num);
}

Box<double> GetOptimalRegion(const pair<double, double> pinLoc,
                             const vector<Net*>& nets,
                             const vector<DynamicBox<double>>& netBox) {
    double lx, hx, ly, hy;
    vector<double> boxX, boxY;

    bool selfConn = true;

    for (auto net : nets) {
        if (net->isClk) continue;
        const auto& b = netBox[net->id];
        if (b.size() == 1) continue;
        boxX.push_back(b.x.ol(pinLoc.first));
        boxX.push_back(b.x.ou(pinLoc.first));
        boxY.push_back(b.y.ol(pinLoc.second));
        boxY.push_back(b.y.ou(pinLoc.second));

        selfConn = false;
    }

    if (!selfConn) {
        GetMedianTwo(boxX, lx, hx);
        GetMedianTwo(boxY, ly, hy);
    } else {
        // smaller region?
        lx = ly = 0;
        hx = database.sitemap_nx - 1;
        hy = database.sitemap_ny - 1;
    }

    return Box<double>(hx, hy, lx, ly);
}