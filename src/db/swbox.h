#pragma once

#include "../global.h"

namespace db {

class Site;
class Pin;

class SwitchBox {
public:
    int x, y;
    std::vector<Site *> sites;
    std::vector<SwitchBox *> connections;
    std::vector<Pin *> pins;
    // for global routing
    std::vector<int> supply;
    std::vector<int> demand;

    SwitchBox();
    SwitchBox(int x, int y);
    SwitchBox(const SwitchBox &switchbox);

    static bool getSwitchBoxOffset(int i, int &x, int &y);
};

}  // namespace db