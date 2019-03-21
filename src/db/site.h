#pragma once

#include "../global.h"

namespace db {

class Master;
class Instance;
class SiteType;
class Pack;

class Resource {
public:
    enum Name { LUT, FF, CARRY8, DSP48E2, RAMB36E2, IO };
    static Name NameString2Enum(const string &name);
    static string NameEnum2String(const Resource::Name &name);

    Name name;
    SiteType *siteType;
    vector<Master *> masters;

    Resource(Name n);
    Resource(const Resource &resource);

    void addMaster(Master *master);
};

class SiteType {
public:
    enum Name { SLICE, DSP, BRAM, IO };
    static Name NameString2Enum(const string &name);
    static string NameEnum2String(const SiteType::Name &name);

    int id;
    Name name;
    vector<Resource *> resources;

    SiteType(Name n);
    SiteType(const SiteType &sitetype);

    void addResource(Resource *resource);
    void addResource(Resource *resource, int count);

    bool matchInstance(Instance *instance);
};

class Site {
public:
    SiteType *type;
    Pack *pack;
    int x, y;
    int w, h;
    Site();
    Site(int x, int y, SiteType *sitetype);
    // center
    inline double cx() { return x + 0.5 * w; }
    inline double cy() { return y + 0.5 * h; }
};

class Pack {
public:
    SiteType *type;
    Site *site;
    vector<Instance *> instances;
    int id;
    void print();
    bool IsEmpty();
    Pack();
    Pack(SiteType *type);
};

}  // namespace db