#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <thread>
#include <iostream>
#include <fstream>
#include <sstream>

#include <string>
#include <array>
#include <vector>
#include <stack>
#include <queue>
#include <list>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <algorithm>

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <cfloat>
#include <cmath>
#include <cassert>

#include <boost/functional/hash.hpp>

#include "utils/log.h"
#include "utils/misc.h"
#include "utils/geo.h"

using namespace std;

class Setting {
public:
    string io_out;
    string io_aux;
    string io_nodes;
    string io_nets;
    string io_pl;
    string io_wts;
    string io_scl;
    string io_lib;
};

extern Setting setting;

#endif
