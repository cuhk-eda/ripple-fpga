#include "group.h"
#include "db.h"

#include "../alg/matching.h"

using namespace db;

void Group::WriteGroups(const vector<Group>& groups, const string& fileNamePreFix) {
#ifdef WRITE_GROUP
    ofstream os(fileNamePreFix + "_" + database.bmName + ".group");
    if (os.fail()) {
        cerr << "Group::WriteGroups: cannot open file" << endl;
        return;
    }
    os.precision(numeric_limits<double>::digits10 + 2);

    os << groups.size() << endl;
    for (auto g : groups) {
        os << g.id << endl;
        os << g.instances.size() << endl;
        for (auto inst : g.instances) {
            if (inst != NULL) {
                os << inst->id << ' ';
                // 2 conditions: 1. placed, 2. fixed.
                if (inst->slot != -1 && inst->fixed)
                    os << inst->slot << ' ';
                else
                    os << -1 << ' ';
            } else
                os << -1 << ' ';
        }
        os << endl;
        os << g.y << endl;
        os << g.x << endl;
        os << g.lastY << endl;
        os << g.lastX << endl;
    }

    os.close();
#else
    printlog(LOG_ERROR, "Group::WriteGroups: cannot be used in release mode");
#endif
}

void Group::ReadGroups(vector<Group>& groups, const string& fileNamePreFix) {
#ifdef WRITE_GROUP
    groups.clear();
    ifstream is(fileNamePreFix + "_" + database.bmName + ".group");
    if (is.fail()) {
        cerr << "Group::ReadGroups: cannot open file" << endl;
        return;
    }

    unsigned int gsize;
    is >> gsize;
    groups.resize(gsize);
    for (auto& g : groups) {
        is >> g.id;
        unsigned int isize;
        is >> isize;
        g.instances.resize(isize, NULL);
        for (unsigned int i = 0; i < isize; ++i) {
            int instId;
            is >> instId;
            if (instId != -1) {
                g.instances[i] = database.instances[instId];
                is >> g.instances[i]->slot;
            }
        }
        is >> g.y;
        is >> g.x;
        is >> g.lastY;
        is >> g.lastX;
        for (auto inst : g.instances) {
            if (inst != NULL && inst->slot != -1 && !inst->inputFixed) {
                database.place(inst, g.x, g.y, inst->slot);
                inst->fixed = true;
            }
        }
    }

    is.close();
#else
    printlog(LOG_ERROR, "Group::ReadGroups: cannot be used in release mode");
#endif
}

void Group::Print() {
    string instList = "";
    for (auto inst : instances)
        if (inst != NULL) instList = instList + " " + to_string(inst->id);
    printlog(LOG_INFO,
             "group %d:(x,y)=(%.3lf,%.3lf),(lastX,lastY)=(%.3lf,%.3lf),instances=[%s]",
             id,
             x,
             y,
             lastX,
             lastY,
             instList.c_str());
}

bool Group::IsBLE() { return instances[0]->IsLUTFF(); }

SiteType::Name Group::GetSiteType() const {
    for (auto inst : instances)
        if (inst != NULL) return inst->master->resource->siteType->name;
    printlog(LOG_ERROR, "unknow type");
    return SiteType::SLICE;
}

bool Group::IsTypeMatch(Site* site) const { return site->type->name == GetSiteType(); }

bool Group::InClkBox(Site* site) const { return lgBox.dist(site->x, site->y) == 0; }