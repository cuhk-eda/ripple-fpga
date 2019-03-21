#include "cong_est_base.h"

using namespace db;

void CongEstBase::Run(bool stat) {
    InitDemand();
    if (stat) StatDemand();
}

bool CongEstBase::IsLUT2FF(Net* net) {
    if (net->pins.size() != 2) return false;
    unsigned iLUT = 0;
    unsigned iFF = 1;
    if (net->pins[iLUT]->instance->IsFF() && net->pins[iFF]->instance->IsLUT()) swap(iLUT, iFF);
    if (net->pins[iLUT]->instance->IsLUT() && net->pins[iFF]->instance->IsFF() && net->pins[iLUT]->type->name == "O" &&
        net->pins[iFF]->type->name == "D" && net->pins[0]->instance->pack == net->pins[1]->instance->pack) {
        ++noNeedRoute;
        return true;
    } else
        return false;
}

void CongEstBase::StatDemand() const {
    const unsigned step = 50;
    double sum = 0, sumNonEmpty = 0;
    unsigned num = 0, numNonEmpty = 0;
    vector<unsigned> stat;
    vector<unsigned> statNonEmpty;
    for (int x = 0; x < database.sitemap_nx; ++x)
        for (int y = 0; y < database.sitemap_ny; ++y) {
            double d = siteDemand[x][y];
            if (d > minDem) {
                sum += d;
                ++num;
                unsigned i = d / step;
                if (i >= stat.size()) {
                    stat.resize(i + 1, 0);
                    statNonEmpty.resize(i + 1, 0);
                }
                ++stat[i];
                if (database.sites[x][y]->pack != NULL) {
                    sumNonEmpty += d;
                    ++numNonEmpty;
                    ++statNonEmpty[i];
                }
            }
        }
    printlog(LOG_INFO, "%u nets (LUT-FF) don't need routing", noNeedRoute);
    printlog(LOG_INFO,
             "all sites: avg_demand = %lf, #site = %u ; nonempty sites: avg_demand = %lf, #site = %u",
             sum / num,
             num,
             sumNonEmpty / numNonEmpty,
             numNonEmpty);
    for (unsigned i = 0; i < stat.size(); ++i)
        printlog(LOG_INFO, "#site in [%4d, %4d): %5d (%5d)", i * step, (i + 1) * step, stat[i], statNonEmpty[i]);
}