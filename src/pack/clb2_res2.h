#pragma once

#include "clb2.h"
#include "clb2_res_base.h"
#include "alg/lpsolve/inc/lp_lib.h"

class CLB2Result2 : public CLB2ResultBase {
    using CLB2ResultBase::CLB2ResultBase;

public:
    void Run();

private:
    // Major steps
    void Prepare();
    void SolveByILP();

    // Small gadgets
    unsigned nLUT, nFF, nVar;
    // l: lut; h: half; f: ff; g: group; lf: lut-ff conn
    // b(l,h): #lut * 2
    // b(f,g): #ff * 4
    // b(t,g): #ff_type * 4
    // b(lf): #lut-ff
    // s(lf,g): #lut-ff * 4
    vector<array<int, 2>> varlh;
    vector<vector<array<int, 4>>> varfg;
    vector<array<int, 4>> vartg;
    vector<vector<int>> varlf;
    vector<vector<array<int, 4>>> varlfg;
    void InitIndex();
    int *colno;
    REAL *row;
    lprec *lp;
    REAL obj;
    void AddCons();
    void AddVarNames();
    void Put();
    void Check();
};