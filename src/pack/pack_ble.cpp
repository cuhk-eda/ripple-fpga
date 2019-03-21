#include "pack_ble.h"
#include "../gp/gp.h"

#define MAX_GRAPH 2000

// vector<unordered_set<unsigned int>> isValidConnPair;
vector<bool> with2FF;

PackBLE::PackBLE(vector<Group>& groups) {
    numInst = database.instances.size();
    num2LUTBle = 0;
    num2FFBle = 0;
    numPackedFF = 0;
    inst2ble = vector<BLE*>(numInst, NULL);

    // init instPoss
    instPoss = vector<Point2d>(numInst);
    for (unsigned int i = 0; i < numInst; ++i) {
        instPoss[i].x = groups[i].x;
        instPoss[i].y = groups[i].y;
    }
    groups.clear();

    // init numLUT, isLUT
    numLUT = 0;
    numFF = 0;
    instType = vector<InstType>(numInst, OTHERS);
    for (unsigned int i = 0; i < numInst; ++i) {
        auto name = database.instances[i]->master->resource->name;
        if (name == Resource::LUT) {
            ++numLUT;
            instType[i] = InstType(database.instances[i]->master->name - Master::LUT1 + 1);
        } else if (name == Resource::FF) {
            instType[i] = FF;
            ++numFF;
        }
    }
    printlog(LOG_INFO, "PackBLE: inst #:\t%d", numInst);
    printlog(LOG_INFO, "PackBLE: FF #:\t%d", numFF);
    printlog(LOG_INFO, "PackBLE: LUT #:\t%d", numLUT);

    // ignore clk, sr, ce
    for (auto inst : database.instances)
        if (inst->IsFF()) {
            ignoreNetId.insert(inst->pins[database.clkPinIdx]->net);
            ignoreNetId.insert(inst->pins[database.srPinIdx]->net);
            ignoreNetId.insert(inst->pins[database.cePinIdx]->net);
        }

    // 1-step connection degree (ignore FF-FF connections via clk, sr, ce)
    table1StepConn = vector<unordered_map<unsigned int, conn_deg_type>>(numInst);
    for (auto net : database.nets)
        if (ignoreNetId.find(net) == ignoreNetId.end())
            for (auto itPin1 = net->pins.begin(); itPin1 != net->pins.end(); ++itPin1) {
                unsigned int id1 = (*itPin1)->instance->id;
                for (auto itPin2 = next(itPin1); itPin2 != net->pins.end(); ++itPin2) {
                    unsigned int id2 = (*itPin2)->instance->id;
                    ++table1StepConn[id1][id2];
                    ++table1StepConn[id2][id1];
                }
            }
    unsigned int num1StepConnPair = 0;
    conn_deg_type max1StepConn = 0;
    for (const auto& adjs : table1StepConn) {
        num1StepConnPair += adjs.size();
        for (const auto& conn : adjs)
            if (conn.second > max1StepConn) max1StepConn = conn.second;
    }
    printlog(LOG_INFO, "PackBLE: 1-step connected pair #:\t%d", num1StepConnPair / 2);
    printlog(LOG_INFO, "PackBLE: max 1-step connection:\t%d", max1StepConn);
}

PackBLE::~PackBLE() {
    for (auto& ble : bles) delete ble;
}

BLE* PackBLE::NewBle() {
    BLE* newB = new BLE(bles.size());
    bles.push_back(newB);
    return newB;
}

void PackBLE::DeleteBle(BLE* ble) {
    // reduce the size of bles by 1
    bles.back()->SetId(ble->id_);
    bles[ble->id_] = bles.back();
    bles.pop_back();
    // delete the content pointed by ble
    delete ble;
}

void PackBLE::AddLUT(BLE* ble, Instance* lut) {
    ble->AddLUT(lut);
    inst2ble[lut->id] = ble;
    if (ble->LUTs_.size() == 2) ++num2LUTBle;
}

void PackBLE::AddFF(BLE* ble, Instance* ff) {
    ble->AddFF(ff);
    inst2ble[ff->id] = ble;
    if (ble->FFs_.size() == 2) ++num2FFBle;
}

void PackBLE::MergeBLE(BLE* ble1, BLE* ble2) {  // merge ble2 to ble1
    for (auto lut : ble2->LUTs_) AddLUT(ble1, lut);
    for (auto ff : ble2->FFs_) AddFF(ble1, ff);
    if (ble2->LUTs_.size() == 2) --num2LUTBle;
    if (ble2->FFs_.size() == 2) --num2FFBle;
    DeleteBle(ble2);
}

//*******************************************************************************

void PackBLE::StatFFType() const {
    unordered_set<Net*> clkNet, srNet, ceNet;
    unordered_map<pair<Net*, Net*>, unsigned int, boost::hash<pair<Net*, Net*>>> srceNet;
    for (auto inst : database.instances)
        if (instType[inst->id] == FF) {
            clkNet.insert(inst->pins[database.clkPinIdx]->net);
            srNet.insert(inst->pins[database.srPinIdx]->net);
            ceNet.insert(inst->pins[database.cePinIdx]->net);
            ++srceNet[make_pair(inst->pins[database.srPinIdx]->net, inst->pins[database.cePinIdx]->net)];
        }
    printlog(LOG_INFO, "StatFFType: clk type #:\t%d", clkNet.size());
    printlog(LOG_INFO, "StatFFType: sr type #:\t%d", srNet.size());
    printlog(LOG_INFO, "StatFFType: ce type #:\t%d", ceNet.size());
    printlog(LOG_INFO, "StatFFType: sr&ce type #:\t%d", srceNet.size());
}

void PackBLE::StatLUT2FF() const {
    vector<int> numFF;
    int num2PinNet = 0;
    for (auto inst : database.instances) {
        if (instType[inst->id] <= LUT6) {
            Net* oNet = inst->pins[database.lutOPinIdx]->net;
            unsigned int nFF = 0;
            for (auto pin : oNet->pins)
                if (instType[pin->instance->id] == FF && pin->type->name == "D") nFF++;
            if (nFF >= numFF.size()) numFF.resize(nFF + 1, 0);
            ++numFF[nFF];
            if (nFF == 1 && oNet->pins.size() == 2) num2PinNet++;
        }
    }
    int numxFF = 0;
    for (unsigned int i = 1; i < numFF.size(); ++i) numxFF += numFF[i];
    for (unsigned int i = 0; i < numFF.size(); ++i) printlog(LOG_INFO, "StatLUT2FF: %d-FF LUT #:\t%d", i, numFF[i]);
    printlog(LOG_INFO, "StatLUT2FF: only-1-FF LUT #:%d", num2PinNet);
}

void PackBLE::PairLUTFF() {
    StatFFType();
    StatLUT2FF();
    unsigned numInciLUT = 0;
    unsigned numTooFarFF = 0;
    unsigned numThrownFF = 0;
    with2FF = vector<bool>(numInst, false);

    for (unsigned int i = 0; i < numInst; ++i)
        if (instType[i] <= LUT6) {
            vector<pair<double, unsigned int>> cands;
            BLE* newBle = NewBle();
            Instance* lut = database.instances[i];
            AddLUT(newBle, lut);
            for (auto pin : lut->pins[database.lutOPinIdx]->net->pins) {
                unsigned int idFF = pin->instance->id;
                if (pin->type->name == "D" && instType[idFF] == FF) {
                    double dist =
                        abs(instPoss[idFF].x - instPoss[lut->id].x) / 2 + abs(instPoss[idFF].y - instPoss[lut->id].y);
                    if (dist <= mergeRange)
                        cands.push_back(make_pair(dist, idFF));
                    else
                        ++numTooFarFF;
                }
            }
            if (cands.size() != 0) {
                ++numInciLUT;
                sort(cands.begin(), cands.end());  // TODO: num of FF, sum of disp?
                Instance* ff0 = database.instances[cands[0].second];
                AddFF(newBle, ff0);
                ++numPackedFF;
                for (unsigned char c = 1; c < cands.size(); ++c) {
                    Instance* ff1 = database.instances[cands[c].second];
                    if (ff1->pins[database.clkPinIdx]->net == ff0->pins[database.clkPinIdx]->net &&
                        ff1->pins[database.srPinIdx]->net == ff0->pins[database.srPinIdx]->net) {
                        AddFF(newBle, ff1);
                        ++numPackedFF;
                        numThrownFF += cands.size() - 1 - c;
                        with2FF[i] = true;
                        break;
                    } else
                        ++numThrownFF;
                }
            }
        }
    printlog(LOG_INFO, "PairLUTFF: %d FFs packed, %d LUTs incident", numPackedFF, numInciLUT);
    printlog(LOG_INFO,
             "PairLUTFF: BLE #: %d, 2-FF BLE #: %d, thrown FF #: %d, too far FF#: %d",
             bles.size(),
             num2FFBle,
             numThrownFF,
             numTooFarFF);
}

void PackBLE::PairLUTs() {
    // is valid connected pair
    unsigned int numValidConnPair = 0;
    vector<unordered_set<unsigned int>> packedValidConnPair(numInst);
    vector<unsigned int> countVCP;
    for (unsigned int i = 0; i < numInst; ++i) {  // instance i
        if (instType[i] > LUT5) continue;
        for (auto neigh : table1StepConn[i]) {  // instance j
            auto j = neigh.first;
            if (instType[j] > LUT5 || j < i || !inst2ble[i]->IsFFCompatWith(inst2ble[j]))
                continue;               // constraint 1: FF comp
            auto conn1 = neigh.second;  // num of 1-step conn
            auto tot = database.instances[i]->pins.size() + database.instances[j]->pins.size();
            if ((tot - conn1) > 7) continue;  // fast pruning for constraint 2
            auto numDupInputs = NumDupInputs(*database.instances[i], *database.instances[j]);  // reduce routing demand
            auto numDistInputs = tot - 2 - numDupInputs;
            if (numDistInputs <= 5) {  // constraint 2: number of distinct inputs is fewer than 5
                double dist = abs(instPoss[i].x - instPoss[j].x) / 2 + abs(instPoss[i].y - instPoss[j].y);
                if (numDupInputs > 1 && dist < mergeRange) {  // constraint 3&4: shared inputs, distance
                    packedValidConnPair[i].insert(j);
                    packedValidConnPair[j].insert(i);
                }
                // stat
                ++numValidConnPair;
                if (conn1 >= countVCP.size()) countVCP.resize(conn1 + 1, 0);
                ++countVCP[conn1];
            }
        }
    }
    printlog(LOG_INFO, "PairLUTs: stage 1: there are %d valid conneted pairs", numValidConnPair);
    for (unsigned int i = 0; i < countVCP.size(); ++i)
        printlog(LOG_INFO, "\t%d-1-step-connection valid connected pair #:\t%d", i, countVCP[i]);

    // 2-step LUT-LUT connection degree
    vector<unordered_map<unsigned int, conn_deg_type>> table2StepConn(numInst);  // 2-step LUT-LUT connection degree
    conn_deg_type max2StepConn;
    for (auto inst : database.instances) {
        for (unsigned int iPin = 0; iPin < inst->pins.size(); ++iPin) {  // pin i
            Net* iNet = inst->pins[iPin]->net;
            if (iNet == NULL || ignoreNetId.find(iNet) != ignoreNetId.end()) continue;
            for (auto pin1 : iNet->pins) {
                if (pin1 == inst->pins[iPin] || instType[pin1->instance->id] > LUT5) continue;
                for (unsigned int jPin = iPin + 1; jPin < inst->pins.size(); ++jPin) {  // pin j
                    Net* jNet = inst->pins[jPin]->net;
                    if (jNet == NULL || ignoreNetId.find(jNet) != ignoreNetId.end()) continue;
                    for (auto pin2 : jNet->pins) {
                        if (pin2 == inst->pins[jPin] || instType[pin2->instance->id] > LUT5 ||
                            pin1->instance == pin2->instance)
                            continue;
                        unsigned inst1 = pin1->instance->id, inst2 = pin2->instance->id;
                        if (packedValidConnPair[inst1].find(inst2) != packedValidConnPair[inst1].end()) {
                            ++table2StepConn[inst1][inst2];
                            ++table2StepConn[inst2][inst1];
                        }
                    }
                }
            }
        }
    }
    // stat
    unsigned int num2StepConnPair = 0;
    max2StepConn = 0;
    for (const auto& adjs : table2StepConn) {
        num2StepConnPair += adjs.size();
        for (const auto& conn : adjs)
            if (conn.second > max2StepConn) max2StepConn = conn.second;
    }
    printlog(
        LOG_INFO, "PackBLE: stage 1: 2-step connected pair (among valid conneted pairs) #:\t%d", num2StepConnPair / 2);
    printlog(LOG_INFO, "PackBLE: stage 1: max 2-step connection:\t%d", max2StepConn);

    // stage 1 : valid connected pair
    vector<DisjGraph> disjGraphs;
    DisjGraph::GetDisjointGraphs(packedValidConnPair, disjGraphs);
    DisjGraph::StatGraphSize(disjGraphs);
    printlog(LOG_INFO, "PairLUTs: stage 1: there are %d disjoint graphs", disjGraphs.size());
    for (auto& g : disjGraphs) {
        if (g.vertices.size() <= MAX_GRAPH) {
            for (unsigned int i = 0; i < g.u.size(); ++i) {
                g.w[i] = table1StepConn[g.u[i]].at(g.v[i]) * int(max2StepConn + 1);
                auto it = table2StepConn[g.u[i]].find(g.v[i]);
                if (it != table2StepConn[g.u[i]].end()) g.w[i] += it->second;
            }
            vector<Match> matches;
            g.CalcMaxWeightMatch(matches);
            for (auto m : matches) MergeBLE(inst2ble[m.u], inst2ble[m.v]);
        } else {
            vector<vector<int>> pointSets(1, g.vertices);
            DisjGraph::DivideByRegions(pointSets, instPoss, MAX_GRAPH);
            for (unsigned gid = 0; gid < pointSets.size(); ++gid) {
                DisjGraph graph;
                graph.vertices = pointSets[gid];
                for (unsigned i = 0; i < graph.vertices.size(); ++i)
                    for (unsigned j = i + 1; j < graph.vertices.size(); ++j) {
                        int vi = graph.vertices[i];
                        int vj = graph.vertices[j];
                        if (packedValidConnPair[vi].find(vj) != packedValidConnPair[vi].end()) {
                            graph.v.push_back(vi);
                            graph.u.push_back(vj);
                            int weight =
                                table1StepConn[vi].at(vj) * int(max2StepConn + 1);  // primary obj: 1-step connection
                            auto it = table2StepConn[vi].find(vj);
                            if (it != table2StepConn[vi].end())
                                weight += it->second;  // secondary obj: 2-step connection
                            graph.w.push_back(weight);
                        }
                    }
                vector<Match> matches;
                graph.CalcMaxWeightMatch(matches);
                for (auto m : matches) MergeBLE(inst2ble[m.u], inst2ble[m.v]);
            }
        }
    }
    printlog(
        LOG_INFO, "PairLUTs: stage 1: BLE #: %d, 2-LUT BLE #: %d, 2-FF BLE #: %d", bles.size(), num2LUTBle, num2FFBle);

    // stage 2: valid unconnected pair (removed)
}

void PackBLE::PackSingFF() {
    for (unsigned int i = 0; i < numInst; ++i)
        if (inst2ble[i] == NULL && instType[i] == FF) {
            BLE* newBle = NewBle();
            AddFF(newBle, database.instances[i]);
        }
    printlog(LOG_INFO, "PackSingFF: BLE #: %d (added BLEs are soft)", bles.size());
}

void PackBLE::GetResult(vector<Group>& groups) const {
    assert(groups.size() == 0);
    BLE::CheckBles(bles);
    BLE::StatBles(bles);

    for (auto ble : bles) {
        assert(ble->LUTs_.size() != 0 || ble->FFs_.size() != 0);
        Group group;
        ble->GetResult(group);
        group.id = groups.size();
        if (group.instances.size() == 1) {
            group.y = instPoss[group.instances[0]->id].y;
            group.x = instPoss[group.instances[0]->id].x;
        } else {
            group.y = 0;
            group.x = 0;
            unsigned int num = 0;
            for (auto inst : group.instances)
                if (inst->IsLUT()) {
                    group.y += instPoss[inst->id].y;
                    group.x += instPoss[inst->id].x;
                    ++num;
                }
            assert(num != 0);
            group.y /= num;
            group.x /= num;
        }
        groups.push_back(move(group));
    }

    for (auto inst : database.instances) {
        if (inst->IsDSPRAM() || inst->IsIO()) {
            Group group;
            group.instances.push_back(inst);
            group.id = groups.size();
            if (inst->fixed) {
                // just for safety, the locs will be updated by GP
                auto site = inst->pack->site;
                group.x = site->cx();
                group.y = site->cy();
                // group.y = inst->IsIO() ? database.getIOY(inst) : site->cy();
            } else {
                group.x = instPoss[inst->id].x;
                group.y = instPoss[inst->id].y;
            }
            groups.push_back(group);
        }
    }
}