#pragma once

#include "ble.h"
#include "clb_base.h"

struct BleGroupBackup {
    vector<BLE*> ble_ptrs;
    vector<BLE> ble_cnts;
    BleType type;
};

class BleGroup {
    friend class CLB1;

private:
    vector<BLE*> bles;
    BleType type;
    void BackupBleGroup(BleGroupBackup& backup) const;
    void RecoverBleGroup(const BleGroupBackup& backup);
    void UpdateTypeFrom1FF(const BleType& newType);  // for fixBleGroup ONLY
    void Empty();
};

class CLB1 : public CLBBase {
    using CLBBase::CLBBase;

private:
    BleGroup fixBleGroups[2];  // max size 4 for each, either has 2-FF BLE or is full
    BleGroup fleBleGroup;      // max 4 types, sr, clk should be the same
    BleGroup blesWith0FF;
    vector<BLE*> allBles;  // ref

    // Small gadgets
    void RemoveInBles(const BLE* ble, vector<BLE*>& bles);
    void Backup(pair<CLB1, vector<BLE>>& backup);  // TODO: why it doesn't work?
    void Recover(const pair<CLB1, vector<BLE>>& backup);
    void StatFle(
        vector<vector<BLE*>>& fleTypes, unsigned& maxDem, unsigned& minDem, unsigned& maxDemId, unsigned& minDemId);
    void MoveOneTypeToOneGroup(vector<vector<BLE*>>& fleTypes,
                               unsigned tarType,
                               unsigned tarGroup);  // fleTypes is not updated (to destroy)
    void MoveTwoTypesToOneGroup(vector<vector<BLE*>>& fleTypes,
                                unsigned tarType1,
                                unsigned tarType2,
                                unsigned tarGroup);
    void MoveToOneGroupFirst(vector<BLE*> bles, unsigned tarGroup);

    // Major steps
    bool AddBleWith1FF(BLE* const ble);
    bool AddBleIntoFixGroup(BleGroup& fixGroup, BLE* const ble);
    void MoveFleToFix();
    void PostAdjust();  // ignore updating BleType

    // Interfaces
    bool AddBle(BLE* const ble);

public:
    ~CLB1() {
        for (auto ble : allBles) delete ble;
        allBles.clear();
    }
    bool AddInsts(const Group& group);
    inline bool IsEmpty() { return allBles.size() == 0; }
    void GetResult(Group& group);
    void GetFinalResult(Group& group) { GetResult(group); }
    void Print() const;
};