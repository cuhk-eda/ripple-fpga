#include "instance.h"
#include "db.h"

using namespace db;

/***** Master *****/
Master::Name Master::NameString2Enum(const string &name) {
    if (name == "LUT1")
        return LUT1;
    else if (name == "LUT2")
        return LUT2;
    else if (name == "LUT3")
        return LUT3;
    else if (name == "LUT4")
        return LUT4;
    else if (name == "LUT5")
        return LUT5;
    else if (name == "LUT6")
        return LUT6;
    else if (name == "FDRE")
        return FDRE;
    else if (name == "CARRY8")
        return CARRY8;
    else if (name == "DSP48E2")
        return DSP48E2;
    else if (name == "RAMB36E2")
        return RAMB36E2;
    else if (name == "IBUF")
        return IBUF;
    else if (name == "OBUF")
        return OBUF;
    else if (name == "BUFGCE")
        return BUFGCE;
    else {
        cerr << "unknown master name: " << name << endl;
        exit(1);
    }
}

string masterNameStrs[] = {
    "LUT1", "LUT2", "LUT3", "LUT4", "LUT5", "LUT6", "FDRE", "CARRY8", "DSP48E2", "RAMB36E2", "IBUF", "OBUF", "BUFGCE"};
string Master::NameEnum2String(const Master::Name &name) { return masterNameStrs[name]; }

Master::Master(Name n) {
    this->name = n;
    this->resource = NULL;
}

Master::Master(const Master &master) {
    name = master.name;
    resource = master.resource;
    pins.resize(master.pins.size());
    for (int i = 0; i < (int)master.pins.size(); i++) {
        pins[i] = new PinType(*master.pins[i]);
    }
}

Master::~Master() {
    for (int i = 0; i < (int)pins.size(); i++) {
        delete pins[i];
    }
}

void Master::addPin(PinType &pin) { pins.push_back(new PinType(pin)); }

PinType *Master::getPin(const string &name) {
    for (int i = 0; i < (int)pins.size(); i++) {
        if (pins[i]->name == name) {
            return pins[i];
        }
    }
    return NULL;
}

/***** Instance *****/
Instance::Instance() {
    id = -1;
    master = NULL;
    pack = NULL;
    slot = -1;
    fixed = false;
    inputFixed = false;
}

Instance::Instance(const string &name, Master *master) {
    this->id = -1;
    this->name = name;
    this->master = master;
    this->pack = NULL;
    this->slot = -1;
    this->fixed = false;
    this->inputFixed = false;
    this->pins.resize(master->pins.size());
    for (int i = 0; i < (int)master->pins.size(); i++) {
        this->pins[i] = new Pin(this, i);
    }
}

Instance::Instance(const Instance &instance) {
    id = -1;
    name = instance.name;
    master = instance.master;
    pack = NULL;
    slot = -1;
    fixed = instance.fixed;
    inputFixed = instance.inputFixed;
    pins.resize(instance.pins.size());
    for (int i = 0; i < (int)instance.pins.size(); i++) {
        // pins[i] = new Pin(*instance.pins[i]);
        pins[i] = new Pin(this, i);
    }
}

Instance::~Instance() {
    for (int i = 0; i < (int)pins.size(); i++) {
        delete pins[i];
    }
}

Pin *Instance::getPin(const string &name) {
    for (int i = 0; i < (int)pins.size(); i++) {
        if (pins[i]->type->name == name) {
            return pins[i];
        }
    }
    return NULL;
}

bool Instance::matchSiteType(SiteType *type) { return type->matchInstance(this); }
