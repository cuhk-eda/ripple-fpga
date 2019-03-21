#ifndef _PACK_H_
#define _PACK_H_

#include "../db/db.h"
#include "dp/dp_data.h"

using namespace db;

#include "pack_ble.h"

// pack clb
void packble(vector<Group>& groups);

void maxLutFfCon(const DPData& dpData);

#endif