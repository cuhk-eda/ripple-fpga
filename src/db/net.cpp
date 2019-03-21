#include "net.h"
#include "db.h"

using namespace db;

/***** PinType *****/
PinType::PinType() { type = 'x'; }

PinType::PinType(const string &name, char type) {
    this->name = name;
    this->type = type;
}

PinType::PinType(const PinType &pintype) {
    name = pintype.name;
    type = pintype.type;
}

/***** Pin *****/
Pin::Pin() {
    this->instance = NULL;
    this->net = NULL;
    this->type = NULL;
}

Pin::Pin(Instance *instance, int i) {
    this->instance = instance;
    this->net = NULL;
    this->type = instance->master->pins[i];
}

Pin::Pin(Instance *instance, const string &pin) {
    this->instance = instance;
    this->net = NULL;
    this->type = instance->master->getPin(pin);
}

Pin::Pin(const Pin &pin) {
    instance = pin.instance;
    net = pin.net;
    type = pin.type;
}

/***** Net *****/
Net::Net() {}

Net::Net(const string &name) {
    this->name = name;
    this->pins.resize(1, NULL);
    this->isClk = false;
}

Net::Net(const Net &net) {
    name = net.name;
    pins = net.pins;
    isClk = net.isClk;
    id = net.id;
}

Net::~Net() {}

void Net::addPin(Pin *pin) {
    if (pin->type->type == 'o' || pin->type->type == 'I') {
        if (pins[0] != NULL) {
            cerr << "source pin duplicated" << endl;
        } else {
            pins[0] = pin;
            pin->net = this;
            return;
        }
    }
    pins.push_back(pin);
    pin->net = this;
}