#pragma once

#include "../global.h"
#include "site.h"

namespace db {

class Instance;
class SiteType;

class Group {
public:
    int id;
    vector<Instance*> instances;
    double x, y;
    double lastX, lastY;
    double areaScale;
    Box<int> lgBox;

    Group()
        : id(-1), x(0.0), y(0.0), lastX(0.0), lastY(0.0), areaScale(1.0), lgBox(INT_MAX, INT_MAX, INT_MIN, INT_MIN) {}
    void Print();
    bool IsBLE();
    bool IsTypeMatch(Site* site) const;
    bool InClkBox(Site* site) const;
    SiteType::Name GetSiteType() const;

    static void WriteGroups(const vector<Group>& groups, const string& fileNamePreFix);
    static void ReadGroups(vector<Group>& groups, const string& fileNamePreFix);
};
}  // namespace db