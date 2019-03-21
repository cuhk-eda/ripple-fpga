#pragma once

#include "../global.h"
#include "site.h"

namespace db {

class PinType;
class Pin;
class Resource;
class SiteType;
class Pack;

// TODO: rename to CellType
class Master {
public:
    enum Name {
        LUT1,
        LUT2,
        LUT3,
        LUT4,
        LUT5,
        LUT6,      // LUT
        FDRE,      // FF
        CARRY8,    // CARRY
        DSP48E2,   // DSP
        RAMB36E2,  // RAM
        IBUF,
        OBUF,
        BUFGCE  // IO: input buf, output buf, clock buf
    };
    static Name NameString2Enum(const string &name);
    static string NameEnum2String(const Master::Name &name);

    Name name;
    Resource *resource;
    vector<PinType *> pins;

    Master(Name n);
    Master(const Master &master);
    ~Master();

    void addPin(PinType &pin);
    PinType *getPin(const string &name);
};

// TODO: rename to Cell
class Instance {
public:
    int id;
    string name;
    Master *master;
    Pack *pack;
    int slot;
    bool fixed;
    bool inputFixed;
    vector<Pin *> pins;

    Instance();
    Instance(const string &name, Master *master);
    Instance(const Instance &instance);
    ~Instance();

    inline bool IsLUT() { return master->resource->name == Resource::LUT; }
    inline bool IsFF() { return master->name == Master::FDRE; }
    inline bool IsCARRY() { return master->name == Master::CARRY8; }
    inline bool IsLUTFF() { return IsLUT() || IsFF(); }  // TODO: check CARRY8
    inline bool IsDSP() { return master->name == Master::DSP48E2; }
    inline bool IsRAM() { return master->name == Master::RAMB36E2; }
    inline bool IsDSPRAM() { return IsDSP() || IsRAM(); }
    inline bool IsIO() { return master->resource->name == Resource::IO; }

    Pin *getPin(const string &name);
    bool matchSiteType(SiteType *type);
};

}  // namespace db