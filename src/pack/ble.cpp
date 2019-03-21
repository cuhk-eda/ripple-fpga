#include "ble.h"

// extern vector<unordered_set<unsigned int>> isValidConnPair;

void BleType::Print() const {
    if (ce.size() == 0)
        printf("no FF\n");
    else {
        printf("clk:%p, sr:%p, ce:%d (%p", clk, sr, (int)ce.size(), ce[0]);
        if (ce.size() == 2) printf(", %p", ce[1]);
        printf(")\n");
    }
}

bool IsMatched(BleType& type1, BleType& type2) {
    assert(type1.ce.size() <= 2 && type2.ce.size() <= 2);
    if (type1.ce.size() == 0 || type2.ce.size() == 0)
        return true;
    else if (type1.clk != type2.clk || type1.sr != type2.sr)
        return false;
    else if (type1.ce.size() == 2 && type2.ce.size() == 2) {
        // make sure ce[0] <= ce[1]
        if (type1.ce[0] > type1.ce[1]) swap(type1.ce[0], type1.ce[1]);
        if (type2.ce[0] > type2.ce[1]) swap(type2.ce[0], type2.ce[1]);
        return type1.ce[0] == type2.ce[0] && type1.ce[1] == type2.ce[1];
    } else if (type1.ce.size() == 1 && type2.ce.size() == 2)
        return type1.ce[0] == type2.ce[0] || type1.ce[0] == type2.ce[1];
    else if (type1.ce.size() == 1 && type2.ce.size() == 1)
        return true;
    else  // (type1.ce.size() == 2 && type2.ce.size() == 1)
        return type1.ce[0] == type2.ce[0] || type1.ce[1] == type2.ce[0];
}

//*******************************************************************************

BLE& BLE::operator=(const BLE& rhs) {
    id = rhs.id;
    // clb = rhs.clb;
    LUTs = rhs.LUTs;
    FFs = rhs.FFs;
    type = rhs.type;
    return *this;
}

void BLE::AddLUT(Instance* const inst) {
    assert(inst->IsLUT() && LUTs.size() < 2);
    LUTs.push_back(inst);
}

void BLE::AddFF(Instance* const inst) {
    assert(inst->IsFF() && FFs.size() < 2);
    // clk, sr, ce
    if (FFs.size() == 1)
        assert(type.clk == inst->pins[database.clkPinIdx]->net && type.sr == inst->pins[database.srPinIdx]->net);
    else {
        type.clk = inst->pins[database.clkPinIdx]->net;
        type.sr = inst->pins[database.srPinIdx]->net;
    }
    type.ce.push_back(inst->pins[database.cePinIdx]->net);
    // FF, nets
    FFs.push_back(inst);
}

bool BLE::IsLUTCompatWith(BLE* rhs) {
    return LUTs.size() == 0 || rhs->LUTs.size() == 0 ||
           (LUTs.size() == 1 && rhs->LUTs.size() == 1 &&
            (LUTs[0]->pins.size() + rhs->LUTs[0]->pins.size() <= 7 || IsLUTCompatible(*LUTs[0], *rhs->LUTs[0])));
    // isValidConnPair[LUTs[0]->id].find(rhs->LUTs[0]->id) != isValidConnPair[LUTs[0]->id].end()));
}

bool BLE::IsFFCompatWith(BLE* rhs) {
    return FFs.size() == 0 || rhs->FFs.size() == 0 ||
           (FFs.size() == 1 && rhs->FFs.size() == 1 && type.clk == rhs->type.clk && type.sr == rhs->type.sr);
}

void BLE::MergeInto(BLE* ble) {
    for (auto lut : this->LUTs) ble->AddLUT(lut);
    for (auto ff : this->FFs) ble->AddFF(ff);
    this->Empty();
}

void BLE::Empty() {
    LUTs.clear();
    FFs.clear();
    type.ce.clear();
}

void BLE::Print() const {
    printf("\tble:%d, ", this->id);
    for (auto inst : LUTs) printf("lut%d, ", inst->id);
    for (auto inst : FFs) printf("ff%d, ", inst->id);
    this->type.Print();
}

void BLE::GetResult(Group& group) const {
    for (auto inst : FFs) group.instances.push_back(inst);
    for (auto inst : LUTs) group.instances.push_back(inst);
}

void BLE::StatBles(const vector<BLE*>& bles) {
    printlog(LOG_INFO, "*******************************************************************");
    printlog(LOG_INFO, "BLE::StatBles: ");
    array<array<unsigned int, 3>, 3> num = {};
    array<unsigned int, 3> numLUT = {};
    array<unsigned int, 3> numFF = {};
    unsigned int sum = 0;
    for (auto ble : bles)
        if (ble != NULL && (ble->LUTs_.size() != 0 || ble->FFs_.size() != 0)) {
            ++num[ble->LUTs_.size()][ble->FFs_.size()];
            ++numLUT[ble->LUTs_.size()];
            ++numFF[ble->FFs_.size()];
            ++sum;
        }

    array<unsigned int, 3> n, p;
    printlog(LOG_INFO, "      \t0ff\t\t1ff\t\t2ff\t\tsum");
    for (unsigned int i = 0; i < 3; ++i) {
        for (unsigned int j = 0; j < 3; ++j) {
            n[j] = num[i][j];
            p[j] = num[i][j] * 100 / sum;
        }
        printlog(LOG_INFO,
                 "%dlut%8d (%3d%% )%8d (%3d%% )%8d (%3d%% )%8d (%3d%% )",
                 i,
                 n[0],
                 p[0],
                 n[1],
                 p[1],
                 n[2],
                 p[2],
                 numLUT[i],
                 numLUT[i] * 100 / sum);
    }
    for (unsigned int j = 0; j < 3; ++j) {
        n[j] = numFF[j];
        p[j] = numFF[j] * 100 / sum;
    }
    printlog(LOG_INFO, "sum %8d (%3d%% )%8d (%3d%% )%8d (%3d%% )%8d (100%% )", n[0], p[0], n[1], p[1], n[2], p[2], sum);
    printlog(LOG_INFO, "*******************************************************************");
}

void BLE::CheckBles(const vector<BLE*>& bles) {
    // bool existDup = false;
    vector<bool> missed(database.instances.size(), true);
    for (auto ble : bles)
        if (ble != NULL) {
            for (auto lut : ble->LUTs) {
                if (!missed[lut->id]) {
                    printlog(LOG_ERROR,
                             "BLE::CheckBles: %d %s (%s) occurs again in ble%d",
                             lut->id,
                             lut->name.c_str(),
                             lut->master->name,
                             ble->id);
                    // existDup = true;
                }
                missed[lut->id] = false;
            }
            for (auto ff : ble->FFs) {
                if (!missed[ff->id]) {
                    printlog(LOG_ERROR,
                             "BLE::CheckBles: %d %s (%s) occurs again in ble%d",
                             ff->id,
                             ff->name.c_str(),
                             ff->master->name,
                             ble->id);
                    // existDup = true;
                }
                missed[ff->id] = false;
            }
        }
    // if (!existDup) printlog(LOG_INFO, "BLE::CheckBles: there is no duplicated inst in bles");

    // bool existMiss = false;
    for (auto inst : database.instances)
        if (missed[inst->id] && (inst->IsLUTFF())) {
            printlog(
                LOG_ERROR, "CheckBles: %d %s (%s) is missed in bles", inst->id, inst->name.c_str(), inst->master->name);
            // existMiss = true;
        }
    // if (!existMiss) printlog(LOG_INFO, "BLE::CheckBles: there is no missed inst");
}