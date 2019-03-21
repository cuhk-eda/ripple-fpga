#pragma once

#include "global.h"

void MinCostBipartiteMatching(
    const vector<vector<pair<int, long>>>& bigraph, size_t num1, size_t num2, vector<pair<int, long>>& res, long& cost);

void LiteMinCostMaxFlow(const vector<vector<pair<int, int>>>& supplyToDemand,
                        const vector<int>& demandCap,
                        size_t num1,
                        size_t num2,
                        vector<pair<int, int>>& res,
                        int& cost);