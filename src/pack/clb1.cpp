#include "clb1.h"

void BleGroup::BackupBleGroup(BleGroupBackup& backup) const {
    backup.type = type;
    backup.ble_ptrs = bles;
    backup.ble_cnts.resize(bles.size());
    for (unsigned i = 0; i < bles.size(); ++i) backup.ble_cnts[i] = *bles[i];
}

void BleGroup::RecoverBleGroup(const BleGroupBackup& backup) {
    type = backup.type;
    bles = backup.ble_ptrs;
    for (unsigned i = 0; i < bles.size(); ++i) *bles[i] = backup.ble_cnts[i];
}

void BleGroup::UpdateTypeFrom1FF(const BleType& newType) {
    assert(newType.ce.size() == 1);
    type.clk = newType.clk;
    type.sr = newType.sr;
    if (type.ce.size() == 0 || (type.ce.size() == 1 && type.ce[0] != newType.ce[0])) type.ce.push_back(newType.ce[0]);
}

void BleGroup::Empty() {
    bles.clear();
    type.ce.clear();
}

//*******************************************************************************

void CLB1::RemoveInBles(const BLE* ble, vector<BLE*>& bles) {
    auto it = find(bles.begin(), bles.end(), ble);
    if (it != bles.end()) {
        *it = bles.back();
        bles.pop_back();
    }
}

void CLB1::Backup(pair<CLB1, vector<BLE>>& backup) {
    backup.first = *this;
    backup.second.resize(allBles.size());
    for (unsigned i = 0; i < allBles.size(); ++i) backup.second[i] = *allBles[i];
}

void CLB1::Recover(const pair<CLB1, vector<BLE>>& backup) {
    *this = backup.first;
    for (unsigned i = 0; i < allBles.size(); ++i) *allBles[i] = backup.second[i];
}

void CLB1::StatFle(
    vector<vector<BLE*>>& fleTypes, unsigned& maxDem, unsigned& minDem, unsigned& maxDemId, unsigned& minDemId) {
    assert(fleBleGroup.bles.size() != 0);
    for (auto fle : fleBleGroup.bles) {
        assert(fle->FFs.size() == 1);
        bool exist = false;
        for (auto& fleType : fleTypes)
            if (fleType[0]->type.ce[0] == fle->type.ce[0]) {
                fleType.push_back(fle);
                exist = true;
                break;
            }
        if (!exist) fleTypes.push_back(vector<BLE*>(1, fle));
    }
    maxDem = fleTypes[0].size();
    maxDemId = 0;
    for (unsigned i = 1; i < fleTypes.size(); ++i) {
        if (fleTypes[i].size() > maxDem) {
            maxDem = fleTypes[i].size();
            maxDemId = i;
        }
    }
    minDem = fleTypes.back().size();
    minDemId = fleTypes.size() - 1;
    for (int i = fleTypes.size() - 2; i >= 0; --i) {
        if (fleTypes[i].size() < minDem) {
            minDem = fleTypes[i].size();
            minDemId = i;
        }
    }
}

void CLB1::MoveOneTypeToOneGroup(vector<vector<BLE*>>& fleTypes, unsigned tarType, unsigned tarGroup) {
    for (unsigned i = 0; i < fleTypes.size(); ++i) {
        unsigned gid = (i == tarType) ? tarGroup : 1 - tarGroup;
        for (auto fle : fleTypes[i]) fixBleGroups[gid].bles.push_back(fle);
        fixBleGroups[gid].UpdateTypeFrom1FF(fleTypes[i][0]->type);
    }
    fleBleGroup.Empty();
}

void CLB1::MoveTwoTypesToOneGroup(vector<vector<BLE*>>& fleTypes,
                                  unsigned tarType1,
                                  unsigned tarType2,
                                  unsigned tarGroup) {
    for (unsigned i = 0; i < fleTypes.size(); ++i) {
        unsigned gid = (i == tarType1 || i == tarType2) ? tarGroup : 1 - tarGroup;
        for (auto fle : fleTypes[i]) fixBleGroups[gid].bles.push_back(fle);
        fixBleGroups[gid].UpdateTypeFrom1FF(fleTypes[i][0]->type);
    }
    fleBleGroup.Empty();
}

void CLB1::MoveToOneGroupFirst(vector<BLE*> bles, unsigned tarGroup) {
    for (auto ble : bles) {
        if (fixBleGroups[tarGroup].bles.size() < 4)
            fixBleGroups[tarGroup].bles.push_back(ble);
        else
            fixBleGroups[1 - tarGroup].bles.push_back(ble);
    }
}

//*******************************************************************************

bool CLB1::AddBleWith1FF(BLE* const ble) {
    assert(ble->FFs.size() == 1);
    if (fixBleGroups[0].bles.size() == 0 && fixBleGroups[1].bles.size() == 0) {  // fixBleGroups are empty
        if (fleBleGroup.type.ce.size() == 0) {  // all three groups are empty (i.e. no ble at all)
            fleBleGroup.bles.push_back(ble);
            fleBleGroup.type = ble->type;
        } else if (fleBleGroup.type.sr == ble->type.sr &&
                   fleBleGroup.type.clk == ble->type.clk) {  // same <sr, clk> with fleBleGroup
            if (find(fleBleGroup.type.ce.begin(), fleBleGroup.type.ce.end(), ble->type.ce[0]) !=
                fleBleGroup.type.ce.end())
                fleBleGroup.bles.push_back(ble);
            else if (fleBleGroup.type.ce.size() < 4) {
                fleBleGroup.bles.push_back(ble);
                fleBleGroup.type.ce.push_back(ble->type.ce[0]);
            } else
                return false;
        } else if (fleBleGroup.type.ce.size() <= 2 &&
                   fleBleGroup.bles.size() <= 4) {  // different <sr, clk> with fleBleGroup
            fixBleGroups[0] = fleBleGroup;
            fleBleGroup.Empty();
            fixBleGroups[1].bles.push_back(ble);
            fixBleGroups[1].type = ble->type;
        } else
            return false;
    } else {  // at least one fixBleGroup is loaded
        int idx = -1;
        unsigned nFit = 0;
        for (unsigned i = 0; i < 2; ++i)
            if (fixBleGroups[i].bles.size() == 0 ||
                (fixBleGroups[i].bles.size() < 4 && IsMatched(ble->type, fixBleGroups[i].type))) {
                idx = i;
                ++nFit;
            }
        if (nFit == 0)
            return false;
        else if (nFit == 1) {  // only 1 fixBleGroup fits, need to check the influence to fleBleGroup
            if (!AddBleIntoFixGroup(fixBleGroups[idx], ble)) return false;
        } else if (nFit == 2) {  // Both fixBleGroups fit, may need to update fleBleGroup.type
            // check
            assert(fleBleGroup.type.ce.size() <= 2);
            if (fleBleGroup.type.ce.size() != 0) {
                assert(fleBleGroup.type.clk == ble->type.clk && fleBleGroup.type.sr == ble->type.sr);
                assert(fleBleGroup.type.ce.size() == 1 || fleBleGroup.type.ce[0] == ble->type.ce[0] ||
                       fleBleGroup.type.ce[1] == ble->type.ce[0]);
            }
            // update
            fleBleGroup.bles.push_back(ble);
            if (fleBleGroup.type.ce.size() == 0)
                fleBleGroup.type = ble->type;
            else if (fleBleGroup.type.ce.size() == 1 && fleBleGroup.type.ce[0] != ble->type.ce[0])
                fleBleGroup.type.ce.push_back(ble->type.ce[0]);
        }
    }
    return true;
}

bool CLB1::AddBleIntoFixGroup(BleGroup& fixGroup, BLE* const ble) {
    if (fixGroup.bles.size() != 0 && !(fixGroup.bles.size() < 4 && IsMatched(ble->type, fixGroup.type)))
        return false;
    else if (fixGroup.type.ce.size() == 2) {
        fixGroup.bles.push_back(ble);
        return true;
    }
    // backup for recovery
    BleGroupBackup fixBleGroupsTmp[2], fleBleGroupTmp, blesWith0FFTmp;
    fixBleGroups[0].BackupBleGroup(fixBleGroupsTmp[0]);
    fixBleGroups[1].BackupBleGroup(fixBleGroupsTmp[1]);
    fleBleGroup.BackupBleGroup(fleBleGroupTmp);
    vector<BLE*> allBlesTmp = allBles;
    BLE bleTmp = *ble;
    // clear fleBleGroup
    fleBleGroup.Empty();
    // attempt to insert
    fixGroup.bles.push_back(ble);
    if (fixGroup.type.ce.size() == 0 || ble->type.ce.size() == 2)
        fixGroup.type = ble->type;
    else if (fixGroup.type.ce.size() == 1 && fixGroup.type.ce[0] != ble->type.ce[0]) {
        fixGroup.type.ce.push_back(ble->type.ce[0]);
    }
    for (const auto& fle : fleBleGroupTmp.ble_ptrs) {
        assert(fle->FFs.size() == 1);
        if (!AddBleWith1FF(fle)) {  // recover
            fixBleGroups[0].RecoverBleGroup(fixBleGroupsTmp[0]);
            fixBleGroups[1].RecoverBleGroup(fixBleGroupsTmp[1]);
            fleBleGroup.RecoverBleGroup(fleBleGroupTmp);
            allBles = allBlesTmp;
            *ble = bleTmp;
            return false;
        }
    }
    if (ble->FFs.size() == 1) {  // merge?
        for (auto tarBle : fixGroup.bles) {
            if (tarBle != ble && tarBle->FFs.size() == 1 && tarBle->type.ce[0] != ble->type.ce[0] &&
                ble->IsLUTCompatWith(tarBle)) {  // merge!
                printlog(LOG_VERBOSE, "new ble%d with 1ff merged into ble%d with 1ff", ble->id, tarBle->id);
                ble->MergeInto(tarBle);
                RemoveInBles(ble, allBles);
                RemoveInBles(ble, fixGroup.bles);
                break;
            }
        }
    }
    return true;
}

void CLB1::MoveFleToFix() {
    vector<vector<BLE*>> fleTypes;
    unsigned maxDem, minDem, maxDemId, minDemId;
    if (allBles.size() >= 6 && fixBleGroups[0].bles.size() != 0 &&
        fleBleGroup.bles.size() != 0) {  // strictly minDem (fle) == minSlot (fix)
        StatFle(fleTypes, maxDem, minDem, maxDemId, minDemId);
        unsigned minSlot = fixBleGroups[0].bles.size(), minSlotId = 0;
        if (fixBleGroups[1].bles.size() > minSlot) {
            minSlot = fixBleGroups[1].bles.size();
            minSlotId = 1;
        }
        minSlot = 4 - minSlot;
        if (minDem == minSlot) {
            bool strictMin = true;
            for (unsigned i = 0; i < fleTypes.size(); ++i)
                if (i != minDemId && fleTypes[i].size() == minDem) {
                    strictMin = false;
                    break;
                }
            if (strictMin)
                MoveOneTypeToOneGroup(fleTypes, minDemId, minSlotId);
            else
                return;
        }
    } else if (fixBleGroups[0].bles.size() == 0 && fixBleGroups[1].bles.size() == 0 && fleBleGroup.bles.size() >= 4 &&
               fleBleGroup.type.ce.size() != 0) {  // maxDem is big enough
        StatFle(fleTypes, maxDem, minDem, maxDemId, minDemId);
        if (maxDem == 4)
            MoveOneTypeToOneGroup(fleTypes, maxDemId, 0);
        else if (fleBleGroup.type.ce.size() == 4 && (maxDem + minDem) == 4)
            MoveTwoTypesToOneGroup(fleTypes, minDemId, maxDemId, 0);  // no improvement if requiring strict min
        else
            return;
    }
    // merge
    for (auto& group : fixBleGroups)
        for (unsigned i = 0; i < group.bles.size(); ++i)
            for (unsigned j = i + 1; j < group.bles.size(); ++j) {
                BLE *&blei = group.bles[i], *&blej = group.bles[j];
                if (blei->FFs.size() == 1 && blej->FFs.size() == 1 && blei->type.ce[0] != blej->type.ce[0] &&
                    blei->IsLUTCompatWith(blej)) {  // merge j into i
                    printlog(LOG_VERBOSE,
                             "CLB1::MoveFleToFix: ble%d with 1ff merged into ble%d with 1ff",
                             blej->id,
                             blei->id);
                    blej->MergeInto(blei);
                    RemoveInBles(blej, allBles);
                    blej = group.bles.back();
                    group.bles.pop_back();
                }
            }
}

//*******************************************************************************

bool CLB1::AddBle(BLE* const ble) {
    assert(ble->LUTs.size() != 0 || ble->FFs.size() != 0);
    assert(ble->clb == NULL);

    // Quick Check
    if (ble->FFs.size() == 0) {  // for merge
        bool merge = false;
        for (auto existBle : allBles)
            if (ble->IsLUTCompatWith(existBle)) {  // Merge
                printlog(LOG_VERBOSE, "new ble%d with 0ff merged into ble%d with 0lut", ble->id, existBle->id);
                ble->MergeInto(existBle);
                merge = true;
            }
        if (!merge) {
            if (allBles.size() < 8) {
                blesWith0FF.bles.push_back(ble);
                allBles.push_back(ble);
            } else
                return false;
        }
    } else {
        // Quick Check
        if (allBles.size() == 8 && !(ble->LUTs.size() == 0 && !blesWith0FF.bles.empty())) return false;

        // Detailed Check
        if (ble->FFs.size() == 2) {
            bool succ = false;
            for (auto& g : fixBleGroups) {
                succ = AddBleIntoFixGroup(g, ble);
                if (succ) break;
            }
            if (!succ) return false;
        } else  //(ble->FFs.size() == 1)
            if (!AddBleWith1FF(ble))
            return false;

        // Move BLEs in fle group into fix group?
        MoveFleToFix();

        if (ble->LUTs.size() != 0 || ble->FFs.size() != 0) {
            allBles.push_back(ble);
            // for merge
            for (auto& existBle : blesWith0FF.bles) {
                if (existBle != ble && existBle->IsLUTCompatWith(ble)) {  // Merge
                    printlog(LOG_VERBOSE, "ble%d with 0ff merged into new ble%d", existBle->id, ble->id);
                    existBle->MergeInto(ble);
                    *find(allBles.begin(), allBles.end(), existBle) = allBles.back();
                    allBles.pop_back();
                    existBle = blesWith0FF.bles.back();
                    blesWith0FF.bles.pop_back();
                }
            }
        }
    }

    assert(allBles.size() <= 8);
    ble->clb = this;
    return true;
}

void CLB1::PostAdjust() {
    assert(allBles.size() == (fixBleGroups[0].bles.size() + fixBleGroups[1].bles.size() + fleBleGroup.bles.size() +
                              blesWith0FF.bles.size()));
    assert(allBles.size() <= 8);
    if (fleBleGroup.bles.size() != 0) {
        vector<vector<BLE*>> fleTypes;
        unsigned maxDem, minDem, maxDemId, minDemId;
        if (fleBleGroup.type.ce.size() <= 2) {  // <=2 CEs (easy)
            if ((fixBleGroups[0].type.ce.size() == 1 || fixBleGroups[1].type.ce.size() == 1) &&
                fleBleGroup.type.ce.size() == 2) {  // maxDem first
                assert(fixBleGroups[0].type.ce.size() == 2 || fixBleGroups[1].type.ce.size() == 2);
                unsigned gid = (fixBleGroups[0].type.ce.size() == 1) ? 0 : 1;
                StatFle(fleTypes, maxDem, minDem, maxDemId, minDemId);
                assert(minDem <= (4 - fixBleGroups[1 - gid].bles.size()));
                MoveToOneGroupFirst(fleTypes[maxDemId], gid);                                   // maxDem
                for (auto fle : fleTypes[minDemId]) fixBleGroups[1 - gid].bles.push_back(fle);  // minDem
            } else                                                                              // arbitrary
                MoveToOneGroupFirst(fleBleGroup.bles, 0);
        } else {
            StatFle(fleTypes, maxDem, minDem, maxDemId, minDemId);
            if (fleBleGroup.type.ce.size() == 3) {  // 3 CEs
                for (auto fle : fleTypes[maxDemId]) fixBleGroups[0].bles.push_back(fle);
                for (unsigned i = 0; i < fleTypes.size(); ++i)
                    if (i != maxDemId) MoveToOneGroupFirst(fleTypes[i], 1);
            } else {  // 4 CEs
                assert((maxDem + minDem) <= 4);
                MoveTwoTypesToOneGroup(fleTypes, maxDemId, minDemId, 0);
            }
        }
        fleBleGroup.bles.clear();
    }
    MoveToOneGroupFirst(blesWith0FF.bles, 0);
    blesWith0FF.bles.clear();
    assert(fixBleGroups[0].bles.size() != 0);
}

//*******************************************************************************

bool CLB1::AddInsts(const Group& group) {
    BLE* ble = new BLE(group.id);
    for (auto inst : group.instances) {
        if (inst->IsLUT())
            ble->AddLUT(inst);
        else if (inst->IsFF())
            ble->AddFF(inst);
        else
            printlog(LOG_ERROR, "invalid inst type in CLB1::AddInsts");
    }
    bool succ = AddBle(ble);
    if (!succ) delete ble;
    return succ;
}

void CLB1::GetResult(Group& group) {
    PostAdjust();
    assert(allBles.size() == (fixBleGroups[0].bles.size() + fixBleGroups[1].bles.size()));
    vector<BLE*> bles(8, NULL);
    for (unsigned i = 0; i < 2; ++i) {
        assert(fixBleGroups[i].bles.size() <= 4);
        for (unsigned j = 0; j < fixBleGroups[i].bles.size(); ++j) bles[i * 4 + j] = fixBleGroups[i].bles[j];
    }
    auto& insts = group.instances;
    insts.assign(33, NULL);
    unsigned j;
    for (unsigned gid = 0; gid < 2; ++gid) {
        unordered_map<Net*, int> ce;
        for (unsigned i = gid * 4; i < (gid + 1) * 4; ++i)
            if (bles[i] != NULL)
                for (auto c : bles[i]->type.ce) ce.insert({c, -1});
        assert(ce.size() <= 2);
        int cnt = 0;
        // if (ce.size()==1) ce.begin()->second = 1;
        // else for (auto& c : ce) c.second = cnt++;
        for (auto& c : ce) c.second = cnt++;
        for (unsigned i = gid * 4; i < (gid + 1) * 4; ++i) {
            if (bles[i] == NULL) continue;
            const auto& luts = bles[i]->LUTs;
            const auto& ffs = bles[i]->FFs;
            for (j = 0; j < luts.size(); ++j) insts[i * 2 + j] = luts[j];
            if (ffs.size() == 2 && ce.size() == 1) {
                insts[16 + i * 2] = ffs[0];
                insts[16 + i * 2 + 1] = ffs[1];
            } else {
                for (j = 0; j < ffs.size(); ++j) insts[16 + i * 2 + ce[ffs[j]->pins[database.cePinIdx]->net]] = ffs[j];
            }
        }
    }

    // move single LUT to odd postion
    for (int i = 0; i < 8; i++) {
        if (insts[i * 2] != NULL && insts[i * 2 + 1] == NULL) {
            insts[i * 2]->slot++;
            insts[i * 2 + 1] = insts[i * 2];
            insts[i * 2] = NULL;
        }
    }
}

void CLB1::Print() const {
    printf("CLB1 #: %d (%ld BLEs)\n", id, allBles.size());
    for (unsigned i = 0; i < 2; ++i)
        if (fixBleGroups[i].bles.size() != 0) {
            printf("fixBleGroups[%d] with %ld BLEs\n", i, fixBleGroups[i].bles.size());
            for (auto ble : fixBleGroups[i].bles) ble->Print();
        }
    if (fleBleGroup.bles.size() != 0) {
        printf("fleBleGroup with %ld BLEs\n", fleBleGroup.bles.size());
        for (auto ble : fleBleGroup.bles) ble->Print();
    }
    if (blesWith0FF.bles.size() != 0) {
        printf("%ld BLEs with 0ff\n", blesWith0FF.bles.size());
        for (auto ble : blesWith0FF.bles) ble->Print();
    }
    assert(allBles.size() == (fixBleGroups[0].bles.size() + fixBleGroups[1].bles.size() + fleBleGroup.bles.size() +
                              blesWith0FF.bles.size()));
}