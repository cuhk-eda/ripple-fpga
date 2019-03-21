#pragma once

#include "../global.h"

namespace db {

class Instance;
class Net;
class Site;

class PinType {
public:
    string name;
    char type;
    /*
       I = primary input
       O = primary output
       i = input
       o = output
       c = clock
       e = control
    */
    PinType();
    PinType(const string &name, char type);
    PinType(const PinType &pintype);
};

class Pin {
public:
    Instance *instance;
    Net *net;
    PinType *type;

    Pin();
    Pin(Instance *instance, int i);
    Pin(Instance *instance, const string &pin);
    Pin(const Pin &pin);
};

class Net {
public:
    int id;
    std::string name;
    std::vector<Pin *> pins;
    bool isClk;

    Net();
    Net(const string &name);
    Net(const Net &net);
    ~Net();

    void addPin(Pin *pin);
};

}  // namespace db