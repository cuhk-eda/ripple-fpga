#include "cong_est_bb.h"

using namespace db;

void CongEstBB::InitDemand() {
    noNeedRoute = 0;
    vector<vector<double>> swDemand(database.switchbox_nx, vector<double>(database.switchbox_ny, 0));
    database.setSwitchBoxPins();
    vector<vector<SwitchBox*>> net2sw(database.nets.size());
    for (const auto& sws : database.switchboxes)
        for (auto sw : sws)
            for (auto pin : sw->pins)
                if (pin != NULL && pin->net != NULL) net2sw[pin->net->id].push_back(sw);

    for (auto net : database.nets) {
        if (IsLUT2FF(net)) continue;
        Box<int> bbox;
        for (auto sw : net2sw[net->id]) bbox.update(sw->x, sw->y);
        double totW = bbox.hp() + 1;
        unsigned nPins = net->pins.size();
        if (nPins < 10)
            totW *= 1.06;
        else if (nPins < 20)
            totW *= 1.2;
        else if (nPins < 30)
            totW *= 1.4;
        else if (nPins < 50)
            totW *= 1.6;
        else if (nPins < 100)
            totW *= 1.8;
        else if (nPins < 200)
            totW *= 2.1;
        else
            totW *= 3.0;
        int numGCell = (bbox.x() + 1) * (bbox.y() + 1);
        double indW = totW / numGCell;
        for (int x = bbox.lx(); x <= bbox.ux(); ++x)
            for (int y = bbox.ly(); y <= bbox.uy(); ++y) swDemand[x][y] += indW;
    }
    SwMapToSiteMap(swDemand, siteDemand);
}