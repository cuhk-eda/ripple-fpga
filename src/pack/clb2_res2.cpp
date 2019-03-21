#include "clb2_res2.h"

void CLB2Result2::Run() {
    Prepare();
    SolveByILP();
    clb2stat.r2 += stat;
}

// *** Major steps ***

void CLB2Result2::Prepare() {
    // init ffgs
    unsigned next = 0;
    for (unsigned i = 0; i < 4; ++i) {
        if (clb.FFs[i].empty()) continue;
        unsigned j;
        for (j = 0; j < next; ++j)
            if (sameCSC(ffgs[j].type, clb.FFs[i][0])) break;
        if (j == next) {
            ffgs.emplace_back();
            ffgs[j].type = clb.FFs[i][0];
            ++next;
        }
        for (auto ff : clb.FFs[i]) ffgs[j].insts.push_back(ff);
    }
    // init lutps
    InitLUTPairs();
}

void CLB2Result2::SolveByILP() {
    // timer::timer myT;
    InitIndex();

    row = (REAL*)malloc(nVar * sizeof(REAL));
    colno = (int*)malloc(nVar * sizeof(int));
    lp = make_lp(0, nVar);
    for (unsigned i = 0; i < nVar; i++) {
        // set_binary(lp, i+1, TRUE);
        set_int(lp, i + 1, TRUE);
    }

    // Cons
    AddCons();

    // Obj
    unsigned j = 0;
    for (auto& vv : varlf)
        for (auto v : vv) {
            colno[j] = v;
            row[j++] = 1;
        }
    set_obj_fnex(lp, j, row, colno);
    set_maxim(lp);

    // Sol
    set_verbose(lp, IMPORTANT);
    auto ret = solve(lp);
    if (ret != OPTIMAL) cerr << "not optimally solved" << endl;
    obj = get_objective(lp);
    get_variables(lp, row);

    // for debug
    // AddVarNames();
    // write_LP(lp, stdout);
    // printf("Objective value: %f\n", obj);
    // for(j = 0; j < nVar; j++)
    //     printf("%s: %f\n", get_col_name(lp, j + 1), row[j]);

    // Put & check
    Put();
    // Check();

    free(colno);
    free(row);
    delete_lp(lp);

    stat.numBroken -= int(round(obj));
    // cout << myT.elapsed() << endl;
}

// *** Small gadgets ***

void CLB2Result2::InitIndex() {
    nFF = 0;
    for (unsigned t = 0; t < ffgs.size(); ++t) {
        nFF += ffgs[t].size();
    }
    nLUT = 0;
    for (auto& lutp : lutps) {
        if (!lutp.ffs.empty()) ++nLUT;
    }

    int index = 1;
    // varlh
    varlh.resize(nLUT);
    for (auto& vv : varlh)
        for (auto& v : vv) v = index++;
    // varfg & vartg
    varfg.resize(ffgs.size());
    for (unsigned t = 0; t < varfg.size(); ++t) {
        varfg[t].resize(ffgs[t].insts.size());
        for (auto& vv : varfg[t])
            for (auto& v : vv) v = -1;  // -1: no up-stream LUT
    }
    for (auto& lutp : lutps) {
        for (auto& ff : lutp.ffs) varfg[ff.g][ff.s][0] = 0;  // 0: to be assgined
    }
    vartg.resize(ffgs.size());
    for (unsigned t = 0; t < varfg.size(); ++t) {
        varfg[t].resize(ffgs[t].insts.size());
        for (auto& v : vartg[t]) v = index++;
        for (auto& vv : varfg[t]) {
            if (vv[0] == 0) {
                for (auto& v : vv) v = index++;
            }
        }
    }
    // varlf, varlfg
    varlf.resize(nLUT);
    varlfg.resize(nLUT);
    int l = 0;
    for (auto& lutp : lutps) {
        if (lutp.ffs.empty()) continue;
        varlf[l].resize(lutp.ffs.size());
        varlfg[l].resize(lutp.ffs.size());
        for (auto& v : varlf[l]) v = index++;
        for (auto& vv : varlfg[l])
            for (auto& v : vv) v = index++;
        ++l;
    }

    nVar = index - 1;
}

void CLB2Result2::AddCons() {
    unsigned l, h, f, g;
    set_add_rowmode(lp, TRUE);

    // 1. LUT/FF uniqueness
    for (l = 0; l < nLUT; ++l) {
        for (h = 0; h < 2; ++h) {
            colno[h] = varlh[l][h];
            row[h] = 1;
        }
        add_constraintex(lp, 2, row, colno, EQ, 1);
    }
    for (auto& vvv : varfg) {
        for (auto& vv : vvv) {
            if (vv[0] == -1) continue;
            for (g = 0; g < 4; ++g) {
                colno[g] = vv[g];
                row[g] = 1;
            }
            add_constraintex(lp, 4, row, colno, EQ, 1);
        }
    }

    // 2. half/group capacity
    for (h = 0; h < 2; ++h) {
        l = 0;
        for (auto& vv : varlh) {
            colno[l] = vv[h];
            row[l++] = 1;
        }
        add_constraintex(lp, l, row, colno, LE, 4);
    }
    for (g = 0; g < 4; ++g) {
        f = 0;
        for (auto& vvv : varfg) {
            for (auto& vv : vvv) {
                if (vv[0] == -1) continue;
                colno[f] = vv[g];
                row[f++] = 1;
            }
        }
        add_constraintex(lp, f, row, colno, LE, 4);
    }

    // 3. FF type conflicts
    // FF group (ce)
    unsigned t;
    for (g = 0; g < 4; ++g) {
        for (t = 0; t < ffgs.size(); ++t) {
            for (unsigned i = 0; i < ffgs[t].size(); ++i) {
                if (varfg[t][i][0] == -1) continue;
                // vartg[t][g] >= varfg[t][i][g]
                colno[0] = varfg[t][i][g];
                row[0] = 1;
                colno[1] = vartg[t][g];
                row[1] = -1;
                add_constraintex(lp, 2, row, colno, LE, 0);
            }
        }
        for (t = 0; t < ffgs.size(); ++t) {
            colno[t] = vartg[t][g];
            row[t] = 1;
        }
        add_constraintex(lp, t, row, colno, LE, 1);  // each group can hold one type only
    }
    for (t = 0; t < ffgs.size(); ++t) {
        // if (ffgs[t].size()<=4) continue;
        for (g = 0; g < 4; ++g) {
            colno[g] = vartg[t][g];
            row[g] = 1;
        }
        int lb = ceil(double(ffgs[t].size()) / 4);
        if (lb == 0) lb = 1;
        add_constraintex(lp, g, row, colno, GE, lb);
    }
    // force type 0 using group 0
    colno[0] = vartg[0][0];
    row[0] = 1;
    add_constraintex(lp, 1, row, colno, EQ, 1);
    // clk sr
    vector<Instance*> csTypes;  // <clk, sr> types
    for (t = 0; t < ffgs.size(); ++t) {
        bool found = false;
        for (auto cst : csTypes) {
            if (sameClk(ffgs[t].type, cst) && sameSr(ffgs[t].type, cst)) {
                found = true;
                break;
            }
        }
        if (!found) csTypes.push_back(ffgs[t].type);
    }
    assert(csTypes.size() <= 2);
    if (csTypes.size() == 2) {
        for (t = 0; t < ffgs.size(); ++t) {
            unsigned cst = 0;
            for (; cst < csTypes.size(); ++cst) {
                if (sameClk(ffgs[t].type, csTypes[cst]) && sameSr(ffgs[t].type, csTypes[cst])) break;
            }
            // disable corresponding groups
            unsigned g0 = (1 - cst) * 2;
            for (g = g0; g < g0 + 2; ++g) {
                colno[0] = vartg[t][g];
                row[0] = 1;
                add_constraintex(lp, 1, row, colno, EQ, 0);
            }
        }
    }

    // 4. LUT-FF connection
    l = 0;
    for (auto& lutp : lutps) {
        if (lutp.ffs.empty()) continue;
        for (unsigned locF = 0; locF < lutp.ffs.size(); ++locF) {
            auto& ff = lutp.ffs[locF];
            assert(varfg[ff.g][ff.s][0] != -1);
            for (g = 0; g < 4; ++g) {
                // 4.1 s(l,f,g): #LUT * #FF * 4
                int h = g / 2;
                int Blh = varlh[l][h];
                int Bfg = varfg[ff.g][ff.s][g];
                int Slfg = varlfg[l][locF][g];
                // Slfg = Blh AND Bfg
                // - Slfg + Blh + Bfg <= 1 (no need)
                // Slfg - Blh <= 0
                colno[0] = Slfg;
                colno[1] = Blh;
                row[0] = 1;
                row[1] = -1;
                add_constraintex(lp, 2, row, colno, LE, 0);
                // Slfg - Bfg <= 0
                colno[0] = Slfg;
                colno[1] = Bfg;
                row[0] = 1;
                row[1] = -1;
                add_constraintex(lp, 2, row, colno, LE, 0);
            }
            // 4.2 b(l,f): #LUT * #FF
            // Blf = OR Slfg
            // - Slf1 - Slf2 - Slf3 - Slf4 + Blf <= 0
            for (g = 0; g < 4; ++g) {
                colno[g] = varlfg[l][locF][g];
                row[g] = -1;
            }
            colno[4] = varlf[l][locF];
            row[4] = 1;
            add_constraintex(lp, 5, row, colno, LE, 0);
        }
        ++l;
    }

    // 5. BLE capacity
    l = 0;
    for (auto& lutp : lutps) {
        if (lutp.ffs.empty()) continue;
        auto& ffs = lutp.ffs;
        for (unsigned i = 0; i < ffs.size(); ++i) {
            for (unsigned j = i + 1; j < ffs.size(); ++j) {
                if (ffs[i].g != ffs[j].g) continue;
                for (g = 0; g < 4; ++g) {
                    row[0] = 1;
                    row[1] = 1;
                    row[2] = 1;
                    row[3] = 1;
                    colno[0] = varfg[ffs[i].g][ffs[i].s][g];
                    colno[1] = varlf[l][i];
                    colno[2] = varfg[ffs[j].g][ffs[j].s][g];
                    colno[3] = varlf[l][j];
                    add_constraintex(lp, 4, row, colno, LE, 3);  // < 4
                }
            }
        }
        ++l;
    }

    set_add_rowmode(lp, FALSE);
}

template <typename T>
void AddVarNames2(lprec* lp, const T& t, const string& name) {
    for (unsigned i = 0; i < t.size(); ++i)
        for (unsigned j = 0; j < t[i].size(); ++j) {
            if (t[i][j] == -1) continue;
            char cstr[10];
            strcpy(cstr, (name + to_string(i) + to_string(j)).c_str());
            set_col_name(lp, t[i][j], cstr);
        }
}

template <typename T>
void AddVarNames3(lprec* lp, const T& t, const string& name) {
    for (unsigned i = 0; i < t.size(); ++i)
        for (unsigned j = 0; j < t[i].size(); ++j) {
            for (unsigned k = 0; k < t[i][j].size(); ++k) {
                if (t[i][j][k] == -1) continue;
                char cstr[10];
                strcpy(cstr, (name + to_string(i) + to_string(j) + to_string(k)).c_str());
                set_col_name(lp, t[i][j][k], cstr);
            }
        }
}

void CLB2Result2::AddVarNames() {
    AddVarNames2(lp, varlh, "lh");
    AddVarNames2(lp, vartg, "tg");
    AddVarNames2(lp, varlf, "lf");
    AddVarNames3(lp, varfg, "fg");
    AddVarNames3(lp, varlfg, "lfg");
}

extern array<unsigned, 4> groupShifts;

void CLB2Result2::Put() {
    // Step 1.1: get lut half id
    vector<int> lutHids(nLUT);
    for (unsigned l = 0; l < nLUT; ++l) {
        lutHids[l] = round(row[varlh[l][1] - 1]);
    }
    // Step 1.2: get ff type group id
    vector<stack<int>> typeGids(vartg.size());
    for (unsigned t = 0; t < vartg.size(); ++t) {
        for (int g = 0; g < 4; ++g) {
            if (int(round(row[vartg[t][g] - 1])) == 1) typeGids[t].push(g);
        }
    }
    // Step 1.3: get ff group id
    vector<vector<int>> ffGids(varfg.size());
    for (unsigned t = 0; t < varfg.size(); ++t) {
        ffGids[t].resize(varfg[t].size(), -1);
        for (unsigned i = 0; i < varfg[t].size(); ++i) {
            if (varfg[t][i][0] == -1)
                ffGids[t][i] = -1;  // to be determined by type group ids
            else {
                int g = 0;
                for (; g < 4; ++g) {
                    if (int(round(row[varfg[t][i][g] - 1])) == 1) break;
                }
                assert(g < 4);
                ffGids[t][i] = g;
            }
        }
    }
    // Step 1.4: lf connected
    int l = 0;
    vector<vector<bool>> lfConn(nLUT);
    for (auto& lutp : lutps) {
        if (lutp.ffs.empty()) continue;
        lfConn[l].resize(lutp.ffs.size());
        for (unsigned f = 0; f < lfConn[l].size(); ++f) {
            lfConn[l][f] = (int(round(row[varlf[l][f] - 1])) == 1);
        }
        ++l;
    }

    // Step 2.1: commit lut pairs with ffs
    l = 0;
    for (auto& lutp : lutps) {
        if (lutp.ffs.empty()) continue;
        unsigned shift = GetShift(lutHids[l]);
        CommitLUTPair(lutp, shift);
        for (unsigned f = 0; f < lfConn[l].size(); ++f) {
            if (lfConn[l][f]) {
                auto& ff = lutp.ffs[f];
                int g = ffGids[ff.g][ff.s];
                CommitFF(lutp.ffs[f], 16 + shift + g % 2);
            }
        }
        ++l;
    }
    // Step 2.2: commit left lut pairs
    for (auto& lutp : lutps) {
        if (!lutp.assigned) {
            CommitLUTPair(lutp, GetShift(lutSlots[0] / 8));
        }
    }
    // Step 2.3: commit left ffs
    for (unsigned t = 0; t < ffgs.size(); ++t)
        for (unsigned i = 0; i < ffgs[t].size(); ++i)
            if (ffgs[t][i] != nullptr) {
                unsigned initSlot, slot;
                do {
                    assert(!typeGids[t].empty());
                    unsigned g = typeGids[t].top();
                    initSlot = groupShifts[g];
                    slot = initSlot;
                    while (res[slot] != nullptr) slot += 2;
                    if (slot - initSlot >= 6) typeGids[t].pop();
                } while (slot - initSlot > 6);
                CommitFF({t, i}, slot);
            }

    // Step 3: move single LUT to odd postion
    for (unsigned i = 0; i < 16; i += 2)
        if (res[i] != nullptr && res[i + 1] == nullptr) {
            res[i + 1] = res[i];
            res[i] = nullptr;
        }
}

void CLB2Result2::Check() {
    int nDirConn = 0;
    for (int l0 = 0; l0 < 16; l0 += 2) {
        for (int l = l0; l < l0 + 2; ++l) {
            if (res[l] == nullptr) continue;
            Net* lutO = res[l]->pins[database.lutOPinIdx]->net;
            for (int f = l0 + 16; f < l0 + 18; ++f) {
                if (res[f] != nullptr && res[f]->pins[database.ffIPinIdx]->net == lutO) {
                    // cout << l << " " << res[l]->id << "  " << f << " directly connects to " << res[f]->id << endl;
                    ++nDirConn;
                }
            }
        }
    }
    if (nDirConn != int(round(obj))) cerr << "mismatched" << endl;
}