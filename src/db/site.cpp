#include "site.h"
#include "db.h"

using namespace db;

/***** Resource *****/
Resource::Name Resource::NameString2Enum(const string &name) {
    using db::Resource;

    if (name == "LUT")
        return LUT;
    else if (name == "FF")
        return FF;
    else if (name == "CARRY8")
        return CARRY8;
    else if (name == "DSP48E2")
        return DSP48E2;
    else if (name == "RAMB36E2")
        return RAMB36E2;
    else if (name == "IO")
        return IO;
    else {
        cerr << "unknown master name: " << name << endl;
        exit(1);
    }
}

string resourceNameStrs[] = {"LUT", "FF", "CARRY8", "DSP48E2", "RAMB36E2", "IO"};
string Resource::NameEnum2String(const Resource::Name &name) { return resourceNameStrs[name]; }

Resource::Resource(Name n) { this->name = n; }

Resource::Resource(const Resource &resource) {
    name = resource.name;
    masters = resource.masters;
}

void Resource::addMaster(Master *master) {
    masters.push_back(master);
    master->resource = this;
}

/***** SiteType *****/
SiteType::Name SiteType::NameString2Enum(const string &name) {
    using db::SiteType;
    if (name == "SLICE")
        return SLICE;
    else if (name == "DSP")
        return DSP;
    else if (name == "BRAM")
        return BRAM;
    else if (name == "IO")
        return IO;
    else {
        cerr << "unknown site type name: " << name << endl;
        exit(1);
    }
}

string siteTypeNameStrs[] = {"SLICE", "DSP", "BRAM", "IO"};
string SiteType::NameEnum2String(const SiteType::Name &name) { return siteTypeNameStrs[name]; }

SiteType::SiteType(Name n) { this->name = n; }

SiteType::SiteType(const SiteType &sitetype) {
    name = sitetype.name;
    resources = sitetype.resources;
}

void SiteType::addResource(Resource *resource) {
    resources.push_back(resource);
    resource->siteType = this;
}
void SiteType::addResource(Resource *resource, int count) {
    for (int i = 0; i < count; i++) {
        addResource(resource);
    }
}

bool SiteType::matchInstance(Instance *instance) {
    for (auto r : resources) {
        if (instance->master->resource == r) {
            return true;
        }
    }
    return false;
}

/***** Site *****/
Site::Site() {
    type = NULL;
    pack = NULL;
    x = -1;
    y = -1;
    w = 0;
    h = 0;
}

Site::Site(int x, int y, SiteType *sitetype) {
    this->x = x;
    this->y = y;
    this->w = 1;
    this->h = 1;
    this->type = sitetype;
    this->pack = NULL;
}

/***** Pack *****/

Pack::Pack() {
    type = NULL;
    site = NULL;
}

Pack::Pack(SiteType *sitetype) {
    this->instances.resize(sitetype->resources.size(), NULL);
    this->type = sitetype;
    this->site = NULL;
}

void Pack::print() {
    string instList = "";
    int nInst = 0;
    for (auto inst : instances)
        if (inst != NULL) {
            nInst++;
            instList = instList + " " + to_string(inst->id);
        }
    printlog(LOG_INFO, "(x,y)=(%lf,%lf), %d instances=[%s]", site->cx(), site->cy(), nInst, instList.c_str());
}

bool Pack::IsEmpty() {
    bool isEmptyPack = true;
    for (auto inst : instances) {
        if (inst != NULL) {
            isEmptyPack = false;
            break;
        }
    }
    return isEmptyPack;
}