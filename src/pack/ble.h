#ifndef _BLE_H_
#define _BLE_H_

#include "../alg/matching.h"

class BleType {  // clk, sr should always be checked after ce, in case that ce.size()==0
public:
    Net* clk;
    Net* sr;
    vector<Net*> ce;  // always ordered for a ble
    // for dev
    void Print() const;
};

// For BleGroup
bool IsMatched(BleType& type1, BleType& type2);

class CLB1;

class BLE {
    friend class BleGroup;
    friend class CLB1;

private:
    unsigned int id;
    CLB1* clb = NULL;

    vector<Instance*> LUTs;  // max size 2
    vector<Instance*> FFs;   // max size 2
    BleType type;

    BLE& operator=(const BLE& rhs);

public:
    BLE(unsigned int i = 0) { id = i; }

    // read only
    const unsigned int& id_ = id;
    // CLB1* const & clb_ = clb;
    const vector<Instance*>& LUTs_ = LUTs;
    const vector<Instance*>& FFs_ = FFs;
    const BleType& type_ = type;

    void SetId(unsigned int i) { id = i; }
    void AddLUT(Instance* inst);
    void AddFF(Instance* inst);
    bool IsLUTCompatWith(BLE* rhs);
    bool IsFFCompatWith(BLE* rhs);
    void MergeInto(BLE* ble);
    void Empty();
    void Print() const;
    void GetResult(Group& group) const;

    // for bles
    static void StatBles(const vector<BLE*>& bles);
    static void CheckBles(const vector<BLE*>& bles);
};

#endif