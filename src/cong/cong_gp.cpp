#include "cong_gp.h"
#include "lg/lg_main.h"
#include "gp/gp.h"
#include "cong_est_bb.h"

Legalizer* legalizer;

void gp_cong(vector<Group>& groups, int iteration) {
    printlog(LOG_INFO, "");
    printlog(LOG_INFO, " = = = = begin congestion-driven GP = = = = ");
    printlog(LOG_INFO, "");
    double prevHpwl, curHpwl, origHpwl;

    // initial legalization
    legalizer = new Legalizer(groups);
    legalizer->Init(USE_CLB2);
    legalizer->RunAll(SITE_HPWL_SMALL_WIN, DEFAULT);
    legalizer->GetResult(NO_UPDATE);
    origHpwl = curHpwl = database.getHPWL();
    printlog(LOG_INFO, "**************");

    for (int iter = 0; iter < iteration; iter++) {
        // inflate
        AdjAreaByCong(groups);

        // gp again
        gplace(groups);

        // legalize
        legalizer->Init(USE_CLB2);
        legalizer->RunAll(SITE_HPWL_SMALL_WIN, DEFAULT);
        legalizer->GetResult(NO_UPDATE);
        prevHpwl = curHpwl;
        curHpwl = database.getHPWL();

        printlog(LOG_INFO, "= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =");
        printlog(LOG_INFO,
                 "iter %d: prev_WL=%0f, cur_WL=%0f, delta=%.2lf%%",
                 iter,
                 prevHpwl,
                 curHpwl,
                 (curHpwl / prevHpwl - 1) * 100.0);
        printlog(LOG_INFO, "= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =");
    }
    printlog(LOG_INFO, "");
    printlog(LOG_INFO, " = = = = finish congestion-driven GP = = = = ");
    printlog(LOG_INFO,
             "Totally, orig_WL=%0f, cur_WL=%0f, delta=%.2lf%%",
             origHpwl,
             curHpwl,
             (curHpwl / origHpwl - 1) * 100.0);
    CongEstBB cong;
    cong.Run();
    WriteMap("cong_final", cong.siteDemand);

    delete legalizer;
}

void GetBleDispMap(vector<Group>& groups, vector<vector<double>>& bleDispMap) {
    bleDispMap.resize(database.sitemap_nx, vector<double>(database.sitemap_ny, 0.0));
    for (int x = 0; x < database.sitemap_nx; x++)
        for (int y = 0; y < database.sitemap_ny; y++)
            if (database.sites[x][y]->pack != NULL && database.sites[x][y]->type->name == SiteType::SLICE) {
                unsigned num = 0;
                for (auto gid : legalizer->lgData.groupMap[x][y]) {
                    if (groups[gid].instances[0]->IsLUTFF()) {
                        ++num;
                        bleDispMap[x][y] += legalizer->lgData.dispPerGroup[gid];
                    }
                }
                if (num != 0) bleDispMap[x][y] /= num;
            }
}

void AdjAreaByCong(vector<Group>& groups) {
    CongEstBB cong;
    cong.Run();
    static int cnt = 0;
    WriteMap("cong_iter" + to_string(cnt++), cong.siteDemand);
    int nScaleUp = 0, nScaleDown = 0, nLargeDisp = 0;
    double maxAreaScale = 1.0, minAreaScale = 1.0;

    // only consider sites with bles
    vector<Point2<int>> sites;
    for (int x = 0; x < database.sitemap_nx; x++)
        for (int y = 0; y < database.sitemap_ny; y++)
            if (database.sites[x][y]->pack != NULL && cong.siteDemand[x][y] > cong.minDem) {
                bool ble = false;
                for (auto gid : legalizer->lgData.groupMap[x][y]) {
                    if (groups[gid].instances[0]->IsLUTFF()) {
                        ble = true;
                        break;
                    }
                }
                if (ble) sites.emplace_back(x, y);
            }
    function<double(Point2<int>)> get_demand = [&](const Point2<int> s) { return cong.siteDemand[s.x()][s.y()]; };
    ComputeAndSort(sites, get_demand, less<double>());

    // inflate (also handle large disp?)
    for (unsigned i = sites.size() - 1; i > 0.9 * sites.size(); --i) {
        int x = sites[i].x();
        int y = sites[i].y();
        if (cong.siteDemand[x][y] < 360) break;
        nScaleUp++;
        for (auto gid : legalizer->lgData.groupMap[x][y]) {
            if (!groups[gid].instances[0]->IsLUTFF()) continue;
            groups[gid].areaScale *= 1.2;
            if (groups[gid].areaScale > maxAreaScale) maxAreaScale = groups[gid].areaScale;
        }
    }

    // shrink
    // vector<vector<double>> bleDispMap;
    // GetBleDispMap(groups, bleDispMap);
    for (unsigned i = 0; i < 0.3 * sites.size(); ++i) {
        int x = sites[i].x();
        int y = sites[i].y();
        if (cong.siteDemand[x][y] > 180) break;
        // if (bleDispMap[x][y] > 0.5){
        if (legalizer->lgData.dispPerSite[x][y] > 2) {
            ++nLargeDisp;
            continue;
        }
        nScaleDown++;
        for (auto gid : legalizer->lgData.groupMap[x][y]) {
            if (!groups[gid].instances[0]->IsLUTFF()) continue;
            groups[gid].areaScale *= 0.8;
            if (groups[gid].areaScale < minAreaScale) minAreaScale = groups[gid].areaScale;
        }
    }

    printlog(LOG_INFO,
             "#site: %d, #up: %d, #down: %d, #large disp: %d, max as: %.2lf, min as: %.2lf",
             sites.size(),
             nScaleUp,
             nScaleDown,
             nLargeDisp,
             maxAreaScale,
             minAreaScale);
}