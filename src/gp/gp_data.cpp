#include "gp_data.h"
#include "gp_setting.h"

// BASICS
int numNodes = 0;
int numCells = 0;
int numMacros = 0;
int numPads = 0;

int numNets = 0;
int numPins = 0;
int numFences = 0;

double coreLX, coreLY;
double coreHX, coreHY;

vector<vector<int> > cellNet;
vector<vector<int> > netCell;

vector<double> netWeight;

vector<double> cellX;
vector<double> cellY;
vector<double> cellW;
vector<double> cellH;

// LOWER BOUND
// last positions
vector<double> lastVarX;
vector<double> lastVarY;
// nearest fence region
vector<double> cellFenceX;
vector<double> cellFenceY;
vector<double> cellFenceDist;
vector<int> cellFenceRect;
vector<int> cellFence;

// UPPER BOUND
// local target density
vector<vector<double> > binTD;
int numDBinX;
int numDBinY;
double DBinW;
double DBinH;
// legalization bin
vector<vector<vector<LGBin> > > LGGrid;  // information of each bin    [fence][binx][biny]
vector<vector<double> > LGTD;            // target density of each bin [binx][biny]
vector<list<OFBin> > OFBins;             // [fence][bin]
vector<vector<vector<char> > > OFMap;    // mark if a bin is handled   [fence][binx][biny]
int LGNumX;
int LGNumY;
double LGBinW;
double LGBinH;
int LGBinSize;  // in number of rows
// area
vector<double> totalFArea;
vector<double> totalUArea;
vector<double> totalCArea;
// region
vector<FRegion> regions;

vector<Group *> cellGroupMap;
void gp_copy_group_in(vector<Group> &groups);
void gp_copy_place_in(vector<Group> &groups);

void gp_copy_in(vector<Group> &groups, bool layout) {
    if (layout)
        gp_copy_group_in(groups);
    else
        gp_copy_place_in(groups);
}

void reset() {
    // BASICS
    numNodes = 0;
    numCells = 0;
    numMacros = 0;
    numPads = 0;

    numNets = 0;
    numPins = 0;
    numFences = 0;

    coreLX = 0;
    coreLY = 0;
    coreHX = 0;
    coreHY = 0;

    cellNet.clear();
    netCell.clear();

    netWeight.clear();

    cellX.clear();
    cellY.clear();
    cellW.clear();
    cellH.clear();

    // LOWER BOUND
    lastVarX.clear();
    lastVarY.clear();
    cellFenceX.clear();
    cellFenceY.clear();
    cellFenceDist.clear();
    cellFenceRect.clear();
    cellFence.clear();

    // UPPER BOUND
    binTD.clear();
    numDBinX = 0;
    numDBinY = 0;
    DBinW = 0;
    DBinH = 0;
    LGGrid.clear();
    LGTD.clear();
    OFBins.clear();
    OFMap.clear();
    LGNumX = 0;
    LGNumY = 0;
    LGBinW = 0;
    LGBinH = 0;
    LGBinSize = 0;
    regions.clear();

    cellGroupMap.clear();
}

void gp_copy_group_in(vector<Group> &groups) {
    reset();
    const auto &gs = gpSetting;
    double areaScale = gs.areaScale;

    unordered_map<Instance *, int> dbCellMap;
    vector<bool> groupIsMacro(groups.size(), false);
    vector<int> groupFence(groups.size(), -1);
    vector<double> groupArea(groups.size(), 0.0);

    coreLX = 0.0;
    coreLY = 0.0;
    coreHX = database.sitemap_nx;
    coreHY = database.sitemap_ny;

    numDBinX = database.tdmap_nx;
    numDBinY = database.tdmap_ny;
    DBinW = (coreHX - coreLX) / numDBinX;
    DBinH = (coreHY - coreLY) / numDBinY;
    binTD.resize(numDBinX, vector<double>(numDBinY, 0.0));
    for (int x = 0; x < numDBinX; x++) {
        for (int y = 0; y < numDBinY; y++) {
            binTD[x][y] = database.targetDensity[x][y];
        }
    }

    numNodes = 0;
    numCells = 0;
    numMacros = 0;
    numPads = 0;
    numFences = database.sitetypes.size();

    printlog(LOG_INFO, "area scale: %lf", gs.areaScale);

    unordered_map<Master *, int> MasterFence;
    unordered_map<Master *, double> MasterArea;
    for (auto master : database.masters) {
        if (gs.packed) {
            switch (master->name) {
                case Master::FDRE:
                case Master::LUT6:
                case Master::LUT5:
                case Master::LUT4:
                case Master::LUT3:
                case Master::LUT2:
                case Master::LUT1:
                case Master::CARRY8:
                    MasterArea[master] = 1.0 * areaScale;
                    break;
                case Master::DSP48E2:
                    MasterArea[master] = gs.dspArea;
                    break;
                case Master::RAMB36E2:
                    MasterArea[master] = gs.ramArea;
                    break;
                case Master::BUFGCE:
                case Master::IBUF:
                case Master::OBUF:
                    MasterArea[master] = 1.0;
                    break;
                default:
                    printlog(LOG_WARN, "unknown master name: %s", master->name);
                    MasterArea[master] = 1.0;
                    break;
            }
        } else {
            switch (master->name) {
                case Master::FDRE:
                    MasterArea[master] = gs.ffArea * areaScale;
                    break;
                case Master::LUT6:
                    MasterArea[master] = 2 * gs.lutArea * areaScale;
                    break;
                case Master::LUT5:
                case Master::LUT4:
                case Master::LUT3:
                case Master::LUT2:
                case Master::LUT1:
                    MasterArea[master] = gs.lutArea * areaScale;
                    break;
                case Master::CARRY8:
                    MasterArea[master] = 0.1 * areaScale;
                    break;
                case Master::DSP48E2:
                    MasterArea[master] = gs.dspArea;
                    break;
                case Master::RAMB36E2:
                    MasterArea[master] = gs.ramArea;
                    break;
                case Master::BUFGCE:
                case Master::IBUF:
                case Master::OBUF:
                    MasterArea[master] = 0.1;
                    break;
                default:
                    printlog(LOG_WARN, "unknown master name: %s", master->name);
                    MasterArea[master] = 0.1;
                    break;
            }
        }

        for (auto sitetype : database.sitetypes) {
            bool found = false;
            for (auto resource : sitetype->resources) {
                if (resource == master->resource) {
                    MasterFence[master] = sitetype->id;
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
    }

    // **assumption: fixed instance must be io instance
    for (size_t i = 0; i < groups.size(); ++i) {
        Group &group = groups[i];
        bool fixed = false;
        for (auto instance : group.instances) {
            if (instance == NULL) continue;
            if (!fixed && instance->fixed) {
                fixed = true;
                Site *site = instance->pack->site;
                group.x = site->cx();
                group.y = site->cy();
                // group.y = instance->IsIO() ? database.getIOY(instance) : site->cy();
            }
            groupFence[i] = MasterFence[instance->master];
            if (gs.packed) {
                groupArea[i] = MasterArea[instance->master];
            } else {
                groupArea[i] += MasterArea[instance->master];
            }
        }
        groupArea[i] *= group.areaScale;
        if (fixed) {
            groupIsMacro[i] = true;
            // numMacros++;
            // commented on 3/5/2017 by GJ, use pad instead of macro
        } else {
            numCells++;
        }
    }

    numNets = database.nets.size();
    netWeight.resize(numNets, 1.0);
    netCell.resize(numNets);

    int nNets = 0;
    for (size_t i = 0; i < database.nets.size(); i++) {
        Net *net = database.nets[i];
        nNets++;
        int nPins = net->pins.size();
        netCell[i].resize(nPins, -1);

        for (auto pin : net->pins) {
            if (pin == NULL) {
                printlog(LOG_WARN, "null pin found in net: %s", net->name.c_str());
                continue;
            }
            if (pin->instance != NULL && pin->instance->fixed) {
                numPads++;
            }
            numPins++;
        }

        if (gs.degreeNetWeight) {
            if (nPins < 10)
                netWeight[i] = 1.06;
            else if (nPins < 20)
                netWeight[i] = 1.2;
            else if (nPins < 30)
                netWeight[i] = 1.4;
            else if (nPins < 50)
                netWeight[i] = 1.6;
            else if (nPins < 100)
                netWeight[i] = 1.8;
            else if (nPins < 200)
                netWeight[i] = 2.1;
            else
                netWeight[i] = 3.0;
        } else {
            netWeight[i] = 1.0;
        }
    }

    numNodes = numCells + numMacros + numPads;

    cellNet.resize(numNodes);
    cellX.resize(numNodes, 0.0);
    cellY.resize(numNodes, 0.0);
    cellW.resize(numNodes, 0.0);
    cellH.resize(numNodes, 0.0);
    lastVarX.resize(numCells, 0.0);
    lastVarY.resize(numCells, 0.0);

    cellGroupMap.resize(numNodes, NULL);
    cellFenceX.resize(numCells, 0.0);
    cellFenceY.resize(numCells, 0.0);
    cellFenceDist.resize(numCells, 0.0);
    cellFenceRect.resize(numCells, 0);
    cellFence.resize(numCells, -1);

    int c_i = 0;  // cell
    // int m_i = numCells;             // macro
    int p_i = numCells + numMacros;  // pad

    double minArea = DBL_MAX;
    double maxArea = -DBL_MAX;
    for (size_t i = 0; i < groups.size(); ++i) {
        Group &group = groups[i];
        int cellID = -1;
        if (groupIsMacro[i]) {
            // cellW[m_i] = sqrt(groupArea[i]);
            // cellH[m_i] = cellW[m_i];
            // cellX[m_i] = group.x;
            // cellY[m_i] = group.y;
            // cellID = m_i;
            // m_i++;
            // commented on 3/5/2017 by GJ, use pad instead of macro
        } else {
            cellW[c_i] = sqrt(groupArea[i]);
            cellH[c_i] = cellW[c_i];
            cellX[c_i] = group.x;
            cellY[c_i] = group.y;
            if (gs.useLastXY) {
                lastVarX[c_i] = group.lastX;
                lastVarY[c_i] = group.lastY;
            } else {
                lastVarX[c_i] = group.x;
                lastVarY[c_i] = group.y;
            }
            cellFence[c_i] = groupFence[i];
            cellID = c_i;
            c_i++;
            double area = cellW[cellID] * cellH[cellID];
            if (area < minArea) minArea = area;
            if (area > maxArea) maxArea = area;
        }
        for (auto instance : group.instances) {
            if (instance != NULL) {
                // assume instance to group is one-to-one mapping
                dbCellMap[instance] = cellID;
            }
        }
        if (cellID != -1) cellGroupMap[cellID] = &group;
    }
    printlog(LOG_INFO, "min area = %lf; max area = %lf", minArea, maxArea);

    // area adjustment
    vector<double> fenceCellArea(numFences, 0.0);
    vector<double> fenceFreeArea(numFences, 0.0);
    for (int i = 0; i < numCells; i++) {
        fenceCellArea[cellFence[i]] += cellW[i] * cellH[i];
    }
    for (int x = 0; x < database.sitemap_nx; x++) {
        for (int y = 0; y < database.sitemap_ny; y++) {
            int fence = database.sites[x][y]->type->id;
            fenceFreeArea[fence] += 1.0;
        }
    }
    for (int i = 0; i < numFences; i++) {
        if (fenceCellArea[i] > fenceFreeArea[i] * 0.95) {
            printlog(LOG_WARN, "cell area too high for fence %d : %lf vs %lf", i, fenceCellArea[i], fenceFreeArea[i]);
            double scale = sqrt(fenceFreeArea[i] * 0.95 / fenceCellArea[i]);
            printlog(LOG_WARN, "width and height are scaled by: %lf", scale);
            for (int i = 0; i < numCells; i++) {
                cellH[i] *= scale;
                cellW[i] *= scale;
            }
        } else {
            if (fenceCellArea[i] != 0)
                printlog(LOG_INFO, "cell area OK for fence %d : %lf vs %lf", i, fenceCellArea[i], fenceFreeArea[i]);
        }
    }

    for (size_t i = 0; i < database.nets.size(); i++) {
        Net *net = database.nets[i];
        bool ioNet = false;
        for (size_t p = 0; p < net->pins.size(); ++p) {
            Pin *pin = net->pins[p];
            if (pin->instance != NULL && pin->instance->fixed) {
                Site *site = pin->instance->pack->site;
                cellW[p_i] = 0;
                cellH[p_i] = 0;
                if (pin->instance->master->name == Master::IBUF) {
                    cellX[p_i] = site->x /*+ 0.5 * site->w*/;
                    cellY[p_i] = site->y + 0.5 + database.pinSwitchBoxMap[Database::PinRuleInput][pin->instance->slot];
                    ioNet = true;
                } else if (pin->instance->master->name == Master::OBUF) {
                    cellX[p_i] = site->x /*+ 0.5 * site->w*/;
                    cellY[p_i] = site->y + 0.5 + database.pinSwitchBoxMap[Database::PinRuleOutput][pin->instance->slot];
                    ioNet = true;
                } else {
                    cellX[p_i] = site->cx();
                    cellY[p_i] = site->cy();
                }
                // cellX[p_i] = site->cx();
                // if(pin->instance->IsIO()){
                //     cellY[p_i] = database.getIOY(pin->instance);
                //     ioNet = true;
                // }else{
                //     cellY[p_i] = site->cy();
                // }

                // TODO: BRAM and DSP
                // printlog(LOG_INFO, "fixed cell pin at (%lf,%lf) (%d,%d)", cellX[p_i], cellY[p_i], site->x, site->y);
                netCell[i][p] = p_i;
                p_i++;
            } else {
                netCell[i][p] = dbCellMap[pin->instance];
            }
        }
        if (gs.ioNetWeight > 1.0 && ioNet) {
            netWeight[i] *= gs.ioNetWeight;
        }
    }

    // region initialization
    regions.resize(numFences);
    for (int i = 0; i < numFences; i++) {
        SiteType *type = database.sitetypes[i];
        regions[type->id].type = type;
    }
    for (int x = 0; x < database.sitemap_nx; x++) {
        for (int i = 0; i < numFences; i++) {
            if (regions[i].type != database.sites[x][0]->type) {
                continue;
            }
            FRect rect(x, 0, x + 1, database.sitemap_ny);

            int nRects = regions[i].rects.size();
            if (nRects == 0 || regions[i].rects[nRects - 1].hx != x) {
                // new rect or not connected to the previous one
                regions[i].addRect(rect);
            } else {
                // merge with the previous one
                regions[i].rects[nRects - 1].hx = x + 1;
            }
            break;
        }
    }

    /*
    for(int i=0; i<(int)regions.size();i++){
        printlog(LOG_INFO,"region id:%d sitetype->id:%d site
    type:%s",i,regions[i].type->id,regions[i].type->name.c_str());
    }
    */
}

void gp_copy_place_in(vector<Group> &group) {
    for (int i = 0; i < numCells; i++) {
        cellX[i] = cellGroupMap[i]->x;
        cellY[i] = cellGroupMap[i]->y;
    }
}

void gp_copy_out() {
    for (int i = 0; i < numCells; i++) {
        cellGroupMap[i]->x = cellX[i];
        cellGroupMap[i]->y = cellY[i];
        cellGroupMap[i]->lastX = lastVarX[i];
        cellGroupMap[i]->lastY = lastVarY[i];
    }
}

double hpwl(double *wx, double *wy) {
    double totalHPWLX = 0.0;
    double totalHPWLY = 0.0;
    for (size_t nid = 0; nid < netCell.size(); ++nid) {
        Box<double> box(cellX[netCell[nid][0]], cellY[netCell[nid][0]]);
        for (size_t cid = 1; cid < netCell[nid].size(); ++cid)
            box.fupdate(cellX[netCell[nid][cid]], cellY[netCell[nid][cid]]);
        totalHPWLX += box.x();
        totalHPWLY += box.y();
    }
    if (wx != NULL) *wx = totalHPWLX / 2;
    if (wy != NULL) *wy = totalHPWLY;
    return totalHPWLX / 2 + totalHPWLY;
}

/****** FRect *****/
FRect::FRect() {
    lx = -1;
    ly = -1;
    hx = -1;
    hy = -1;
}
FRect::FRect(double lx, double ly, double hx, double hy) {
    this->lx = lx;
    this->ly = ly;
    this->hx = hx;
    this->hy = hy;
}
FRect::~FRect() {}

/****** Region *****/
FRegion::FRegion() { type = NULL; }
FRegion::FRegion(SiteType *type) { this->type = type; }
void FRegion::addRect(FRect rect) { this->rects.push_back(rect); }