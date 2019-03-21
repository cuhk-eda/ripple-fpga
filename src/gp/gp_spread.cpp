#include "gp_setting.h"
#include "gp_data.h"
#include "gp_spread.h"
#include "../db/db.h"

int minExpandSize = 2;
int maxLevel = 100;

class ExpandBox {
public:
    int lx, ly, hx, hy;
    vector<int> cells;
    char dir;
    int level;
    ExpandBox() {
        lx = ly = hx = hy = -1;
        dir = 'h';
        level = 0;
    }
    ExpandBox(int lx, int ly, int hx, int hy, int level) {
        this->lx = lx;
        this->ly = ly;
        this->hx = hx;
        this->hy = hy;
        this->dir = 'h';
        this->level = level;
    }
    ExpandBox(int lx, int ly, int hx, int hy, int level, vector<int> cells) {
        this->lx = lx;
        this->ly = ly;
        this->hx = hx;
        this->hy = hy;
        this->cells = cells;
        this->dir = 'h';
        this->level = level;
    }
    void clear() {
        lx = ly = hx = hy = -1;
        cells.clear();
    }
};

inline bool cmpX(const int& a, const int& b) { return cellX[a] < cellX[b]; }
inline bool cmpY(const int& a, const int& b) { return cellY[a] < cellY[b]; }

void updateLGGrid() {
    // clear information
    for (int r = 0; r < numFences; r++) {
        for (int x = 0; x < LGNumX; x++) {
            for (int y = 0; y < LGNumY; y++) {
                LGGrid[r][x][y].cArea = 0;
                LGGrid[r][x][y].cells.clear();
            }
        }
    }
    // add cell to bins
    for (int i = 0; i < numCells; i++) {
        int r = cellFence[i];
        int x = max(0, min(LGNumX - 1, (int)((cellX[i] - coreLX) / LGBinW)));
        int y = max(0, min(LGNumY - 1, (int)((cellY[i] - coreLY) / LGBinH)));
        LGGrid[r][x][y].cArea += cellW[i] * cellH[i];
        LGGrid[r][x][y].cells.push_back(i);
    }
}

void updateLGGridFast(int lx, int ly, int hx, int hy, int r, const vector<int>& cells) {
    // update cell area information inside the box (lx,ly,hx,hy)
    // clear information
    for (int x = lx; x <= hx; x++) {
        for (int y = ly; y <= hy; y++) {
            LGGrid[r][x][y].cArea = 0;
            LGGrid[r][x][y].cells.clear();
        }
    }
    // add cell to bins
    for (auto i : cells) {
        assert(r == cellFence[i]);
        int x = max(0, min(LGNumX - 1, (int)((cellX[i] - coreLX) / LGBinW)));
        int y = max(0, min(LGNumY - 1, (int)((cellY[i] - coreLY) / LGBinH)));
        assert(x >= lx && x <= hx && y >= ly && y <= hy);
        LGGrid[r][x][y].cArea += cellW[i] * cellH[i];
        LGGrid[r][x][y].cells.push_back(i);
    }
}

void initSpreadCells(int binSize) {
    if (binSize == LGBinSize) {
        return;
    }
    LGBinSize = binSize;
    LGBinW = LGBinSize;
    LGBinH = LGBinSize;
    LGNumX = ceil((coreHX - coreLX) / LGBinW);
    LGNumY = ceil((coreHY - coreLY) / LGBinH);

    // resize data structures
    LGGrid.clear();
    OFMap.clear();
    OFBins.clear();
    LGTD.clear();

    LGGrid.resize(numFences, vector<vector<LGBin> >(LGNumX, vector<LGBin>(LGNumY)));  // reinitialize bins
    OFMap.resize(numFences, vector<vector<char> >(LGNumX, vector<char>(LGNumY, 0)));  // nothing to be handled
    OFBins.resize(numFences);                          // no overfilled bin for each fence
    LGTD.resize(LGNumX, vector<double>(LGNumY, 1.0));  // target density = 1.0

    // for each legalization bin of each fence
    double siteArea = 1.0;
    double siteW = 1.0;
    double siteH = 1.0;
    for (int r = 0; r < numFences; r++) {
        for (int x = 0; x < LGNumX; x++) {
            for (int y = 0; y < LGNumY; y++) {
                double lx = coreLX + x * LGBinW;
                double ly = coreLY + y * LGBinH;
                double hx = lx + LGBinW;
                double hy = ly + LGBinH;
                LGGrid[r][x][y].lx = lx;
                LGGrid[r][x][y].ly = ly;
                LGGrid[r][x][y].hx = hx;
                LGGrid[r][x][y].hy = hy;
                // convert to unit of site map
                int blx = x * (int)round(LGBinW / siteW);
                int bhx = blx + (int)round(LGBinW / siteW) - 1;
                int bly = y * (int)round(LGBinH / siteH);
                int bhy = bly + (int)round(LGBinH / siteH) - 1;
                blx = max(0, blx);
                bly = max(0, bly);
                bhx = min((int)database.sitemap_nx - 1, bhx);
                bhy = min((int)database.sitemap_ny - 1, bhy);
                LGGrid[r][x][y].fArea = 0;
                for (int sx = blx; sx <= bhx; sx++) {
                    for (int sy = bly; sy <= bhy; sy++) {
                        if (database.sites[sx][sy]->type->id == r) {
                            LGGrid[r][x][y].fArea += siteArea;
                        }
                    }
                }
            }
        }
    }
}

void updateTargetDensity() {
    // Setup legalization bin target density (LGTD)
    // clear information
    for (int x = 0; x < LGNumX; x++) {
        for (int y = 0; y < LGNumY; y++) {
            LGTD[x][y] = 0.0;
        }
    }
    // for each target density bin, find the overlapping legalization bins
    for (int x = 0; x < numDBinX; x++) {
        for (int y = 0; y < numDBinY; y++) {
            double dlx = coreLX + x * DBinW;
            double dhx = min(coreHX, dlx + DBinW);
            double dly = coreLY + y * DBinH;
            double dhy = min(coreHY, dly + DBinH);
            int ilx = (dlx - coreLX) / LGBinW;
            int ihx = min(LGNumX, (int)((dhx - coreLX) / LGBinW));
            int ily = (dly - coreLY) / LGBinH;
            int ihy = min(LGNumY, (int)((dhy - coreLY) / LGBinH));
            // for each overlapping legalization bins, find the overlapping area
            for (int ix = ilx; ix <= ihx; ix++) {
                for (int iy = ily; iy <= ihy; iy++) {
                    double llx = coreLX + ix * LGBinW;
                    double lhx = min(coreHX, llx + LGBinW);
                    double lly = coreLY + iy * LGBinH;
                    double lhy = min(coreHY, lly + LGBinH);
                    double lw = lhx - llx;
                    double lh = lhy - lly;
                    double olw = min(dhx, lhx) - max(dlx, llx);
                    double olh = min(dhy, lhy) - max(dly, lly);
                    // contribute target density to legalization bin
                    if (olw > 0 && olh > 0) {
                        double ratio = (olw * olh) / (lw * lh);
                        LGTD[ix][iy] += ratio * binTD[x][y];
                    }
                }
            }
        }
    }
    /*
    for(int x=0; x<numDBinX; x++){
        for(int y=0; y<numDBinY; y++){
            printlog(LOG_WARN, "%d,%d : %lf", x, y, LGTD[x][y]);
            getchar();
        }
    }
    */
}

void updateBinUsableArea() {
    // Calculate placable area for each bin
    // for each fence

    totalFArea.resize(numFences, 0.0);
    totalUArea.resize(numFences, 0.0);
    totalCArea.resize(numFences, 0.0);

    for (int r = 0; r < numFences; r++) {
        totalFArea[r] = 0.0;
        totalUArea[r] = 0.0;
        totalCArea[r] = 0.0;

        double scale = 1.0;
        // calculate total cell area of the fence
        for (int i = 0; i < numCells; i++) {
            if (r == cellFence[i]) {
                totalCArea[r] += cellW[i] * cellH[i];
            }
        }
        // calculate total usable area under the current target density
        for (int x = 0; x < LGNumX; x++) {
            for (int y = 0; y < LGNumY; y++) {
                totalUArea[r] += LGGrid[r][x][y].fArea * LGTD[x][y];
                totalFArea[r] += LGGrid[r][x][y].fArea;
            }
        }
        if (totalFArea[r] < totalCArea[r]) {
            printlog(LOG_WARN, "total cell area exceed 95\% of total placable area");
        }
        if (totalUArea[r] <= totalCArea[r]) {
            // not enough free are to maintain the current target density
            // increase target density by scale
            scale = totalCArea[r] / totalUArea[r];
        } else {
            scale = 1.0;
        }
        // calculate usable area for each bin
        for (int x = 0; x < LGNumX; x++) {
            for (int y = 0; y < LGNumY; y++) {
                if (LGTD[x][y] * scale > LGGrid[r][x][y].fArea) {
                    LGGrid[r][x][y].uArea = LGGrid[r][x][y].fArea;
                } else {
                    LGGrid[r][x][y].uArea = LGGrid[r][x][y].fArea * LGTD[x][y] * scale;
                }
            }
        }
    }
}

double totOF = 0.0;
double avgOF = 0.0;
double maxOF = 0.0;
double maxOFR = 0.0;
double maxAOF = 0.0;
double maxAOFR = 0.0;
int nOF = 0;

void findOverfillBins() {
    totOF = 0.0;
    avgOF = 0.0;
    maxOF = 0.0;
    maxOFR = 0.0;
    maxAOF = 0.0;
    maxAOFR = 0.0;
    nOF = 0;

    double siteArea = 1.0;
    // double siteArea=circuit.site_w*circuit.site_h;
    for (int r = 0; r < numFences; r++) {
        OFBins[r].clear();
        // for each legalization bin
        for (int x = 0; x < LGNumX; x++) {
            for (int y = 0; y < LGNumY; y++) {
                // cell area overfill
                if (LGGrid[r][x][y].cArea > LGGrid[r][x][y].uArea) {
                    OFBin ofb(r, x, y);
                    ofb.ofArea = LGGrid[r][x][y].cArea - LGGrid[r][x][y].uArea;
                    ofb.aofArea = LGGrid[r][x][y].cArea - LGGrid[r][x][y].fArea;
                    ofb.exCArea = LGGrid[r][x][y].cArea;
                    ofb.exFArea = LGGrid[r][x][y].fArea;
                    ofb.exUArea = LGGrid[r][x][y].uArea;
                    OFMap[r][x][y] = 1;
                    if (LGGrid[r][x][y].uArea < siteArea) {
                        ofb.ofRatio = ofb.ofArea;
                        ofb.aofRatio = ofb.aofArea;
                    } else {
                        ofb.ofRatio = ofb.ofArea / LGGrid[r][x][y].uArea;
                        ofb.aofRatio = ofb.aofArea / LGGrid[r][x][y].fArea;
                    }
                    OFBins[r].push_back(ofb);
                    totOF += ofb.ofArea;
                    maxOF = max(ofb.ofArea, maxOF);
                    maxOFR = max(ofb.ofRatio, maxOFR);
                    maxAOF = max(ofb.aofArea, maxAOF);
                    maxAOFR = max(ofb.aofRatio, maxAOFR);
                    nOF++;
                } else {
                    OFMap[r][x][y] = 0;
                }
            }
        }
    }
}

void cutCells(const ExpandBox& box, ExpandBox& lobox, ExpandBox& hibox, double loRate) {
    double totalCArea = 0;
    for (auto c : box.cells) totalCArea += cellW[c] * cellH[c];
    double halfCArea = loRate * totalCArea;
    double loCArea = 0;
    auto ci = box.cells.begin();
    for (; ci != box.cells.end(); ++ci) {
        loCArea += cellW[*ci] * cellH[*ci];
        if (loCArea > halfCArea) break;
    }
    lobox.cells.insert(lobox.cells.end(), box.cells.begin(), ci);
    reverse(lobox.cells.begin(), lobox.cells.end());
    hibox.cells.insert(hibox.cells.end(), ci, box.cells.end());
}

//#define EVENLY_SPREAD
bool spreadCellsH(int r, const ExpandBox& box, ExpandBox& lobox, ExpandBox& hibox) {
    // assume the cell list has been sorted by increasing X

    // 1. Init free/usable area and box
    vector<double> uArea(box.hx - box.lx + 1, 0);
    double totalFArea = 0;
    lobox.ly = hibox.ly = box.ly;
    lobox.hy = hibox.hy = box.hy;
    lobox.lx = box.lx;
    lobox.hx = box.lx;
    hibox.lx = box.hx;
    hibox.hx = box.hx;
    // calculate stripe free area
    for (int x = box.lx; x <= box.hx; x++) {
        for (int y = box.ly; y <= box.hy; y++) {
            double area = LGGrid[r][x][y].uArea;
            uArea[x - box.lx] += area;
            totalFArea += area;
        }
    }
    // shrink the box such that last stripe has free space
    for (int x = box.lx; x <= box.hx && lobox.hx < hibox.lx - 1; x++) {
        if (uArea[x - box.lx] > 0) break;
        lobox.lx++;
        lobox.hx++;
    }
    for (int x = box.hx; x >= box.lx && hibox.lx > lobox.hx + 1; x--) {
        if (uArea[x - box.lx] > 0) break;
        hibox.lx--;
        hibox.hx--;
    }
    if (lobox.hx == hibox.lx) {
        // TODO: how to handle this?
        // box.lx=lobox.hx;
        // box.hx=lobox.hx;
        // spreadCellsInBoxEven(r, box);
        return false;
    }
    // search for cutting line, such that loArea = hiArea
    double loFArea = uArea[lobox.hx - box.lx];
    double hiFArea = uArea[hibox.lx - box.lx];
    while (hibox.lx - lobox.hx > 1) {
        if (loFArea <= hiFArea) {
            lobox.hx++;
            loFArea += uArea[lobox.hx - box.lx];
        } else {
            hibox.lx--;
            hiFArea += uArea[hibox.lx - box.lx];
        }
    }

    // 2. Cut cells
    cutCells(box, lobox, hibox, loFArea / totalFArea);

    // 3. Spread cells
    auto ce = lobox.cells.end();
    auto cellFm = lobox.cells.begin();
    auto cellTo = lobox.cells.begin();
    // for each column of bins
    for (int x = lobox.hx; x >= lobox.lx; x--) {
        if (cellFm == ce) break;
#ifdef EVENLY_SPREAD
        double cArea = 0;
        int num = 0;
        for (cellTo = cellFm; cellTo != ce && (x == lobox.lx || cArea < uArea[x - box.lx]); ++cellTo) {
            cArea += cellW[*cellTo] * cellH[*cellTo];
            num++;
        }
        if (num > 0) {
            double step = LGBinW / (double)num;
            double nowx = coreLX + (x + 1) * LGBinW - step / 2.0;
            for (auto ci = cellFm; ci != cellTo; ++ci) {
                cellX[*ci] = nowx;
                nowx -= step;
            }
        }
#else
        double cArea = 0;
        int num = 0;
        double oldLoX = cellX[*cellFm];
        double oldHiX = oldLoX;
        for (cellTo = cellFm; cellTo != ce && (x == lobox.lx || cArea < uArea[x - box.lx]); ++cellTo) {
            cArea += cellW[*cellTo] * cellH[*cellTo];
            oldLoX = cellX[*cellTo];  // decreasing X
            num++;
        }
        if (num > 0) {
            double newLoX = coreLX + x * LGBinW + LGBinW / (double)(num * 2);
            double newHiX = newLoX + LGBinW - LGBinW / (double)num;
            double rangeold = max(1.0, oldHiX - oldLoX);
            double rangenew = newHiX - newLoX;
            double scale = rangenew / rangeold;
            for (auto ci = cellFm; ci != cellTo; ++ci) {
                cellX[*ci] = newLoX + (cellX[*ci] - oldLoX) * scale;
            }
        }
#endif
        cellFm = cellTo;
    }
    ce = hibox.cells.end();
    cellFm = hibox.cells.begin();
    cellTo = hibox.cells.begin();
    for (int x = hibox.lx; x <= hibox.hx; x++) {
        if (cellFm == ce) break;
#ifdef EVENLY_SPREAD
        double cArea = 0;
        int num = 0;
        for (cellTo = cellFm; cellTo != ce && (x == hibox.hx || cArea < uArea[x - box.lx]); ++cellTo) {
            cArea += cellW[*cellTo] * cellH[*cellTo];
            num++;
        }
        if (num > 0) {
            double step = LGBinW / (double)num;
            double nowx = coreLX + x * LGBinW + step / 2.0;
            for (auto ci = cellFm; ci != cellTo; ++ci) {
                cellX[*ci] = nowx;
                nowx += step;
            }
        }
#else
        double cArea = 0;
        int num = 0;
        double oldLoX = cellX[*cellFm];
        double oldHiX = oldLoX;
        for (cellTo = cellFm; cellTo != ce && (x == hibox.hx || cArea < uArea[x - box.lx]); ++cellTo) {
            cArea += cellW[*cellTo] * cellH[*cellTo];
            oldHiX = cellX[*cellTo];  // increasing X
            num++;
        }
        if (num > 0) {
            double newLoX = coreLX + x * LGBinW + LGBinW / (double)(num * 2);
            double newHiX = newLoX + LGBinW - LGBinW / (double)num;
            double rangeold = max(1.0, oldHiX - oldLoX);
            double rangenew = newHiX - newLoX;
            double scale = rangenew / rangeold;
            for (auto ci = cellFm; ci != cellTo; ++ci) {
                cellX[*ci] = newLoX + (cellX[*ci] - oldLoX) * scale;
            }
        }
#endif
        cellFm = cellTo;
    }
    return true;
}
bool spreadCellsV(int r, ExpandBox box, ExpandBox& lobox, ExpandBox& hibox) {
    // assume the cell list has been sorted by increasing Y

    // 1. Init free/usable area and box
    vector<double> uArea(box.hy - box.ly + 1, 0);
    double totalFArea = 0;
    lobox.lx = hibox.lx = box.lx;
    lobox.hx = hibox.hx = box.hx;
    lobox.ly = box.ly;
    lobox.hy = box.ly;
    hibox.ly = box.hy;
    hibox.hy = box.hy;
    // calculate stripe free area
    for (int x = box.lx; x <= box.hx; x++) {
        for (int y = box.ly; y <= box.hy; y++) {
            double area = LGGrid[r][x][y].uArea;
            uArea[y - box.ly] += area;
            totalFArea += area;
        }
    }
    // shrink the box such that last stripe has free space
    for (int y = box.ly; y <= box.hy && lobox.hy < hibox.ly - 1; y++) {
        if (uArea[y - box.ly] > 0) break;
        lobox.ly++;
        lobox.hy++;
    }
    for (int y = box.hy; y >= box.ly && hibox.ly > lobox.hx + 1; y--) {
        if (uArea[y - box.ly] > 0) break;
        hibox.ly--;
        hibox.hy--;
    }
    if (lobox.hy == hibox.ly) {
        // TODO: how to handle this?
        // box.ly=lobox.hy;
        // box.hy=lobox.hy;
        // spreadCellsInBoxEven(r, box);
        return false;
    }
    // search for cutting line, such that loArea = hiArea
    double loFArea = uArea[lobox.hy - box.ly];
    double hiFArea = uArea[hibox.ly - box.ly];
    while (hibox.ly - lobox.hy > 1) {
        if (loFArea <= hiFArea) {
            lobox.hy++;
            loFArea += uArea[lobox.hy - box.ly];
        } else {
            hibox.ly--;
            hiFArea += uArea[hibox.ly - box.ly];
        }
    }

    // 2. Cut cells
    cutCells(box, lobox, hibox, loFArea / totalFArea);

    // 3. Spread cells
    auto ce = lobox.cells.end();
    auto cellFm = lobox.cells.begin();
    auto cellTo = lobox.cells.begin();
    // for each row of bins
    for (int y = lobox.hy; y >= lobox.ly; y--) {
        if (cellFm == ce) break;
#ifdef EVENLY_SPREAD
        double cArea = 0;
        int num = 0;
        for (cellTo = cellFm; cellTo != ce && (y == lobox.ly || cArea < uArea[y - box.ly]); ++cellTo) {
            cArea += cellW[*cellTo] * cellH[*cellTo];
            num++;
        }
        if (num > 0) {
            double step = LGBinH / (double)num;
            double nowy = coreLY + (y + 1) * LGBinH - step / 2.0;
            for (auto ci = cellFm; ci != cellTo; ++ci) {
                cellY[*ci] = nowy;
                nowy -= step;
            }
        }
#else
        double cArea = 0;
        int num = 0;
        double oldLoY = cellY[*cellFm];
        double oldHiY = oldLoY;
        for (cellTo = cellFm; cellTo != ce && (y == lobox.ly || cArea < uArea[y - box.ly]); ++cellTo) {
            cArea += cellW[*cellTo] * cellH[*cellTo];
            oldLoY = cellY[*cellTo];  // decreasing Y
            num++;
        }
        if (num > 0) {
            double newLoY = coreLY + y * LGBinH + LGBinH / (double)(num * 2);
            double newHiY = newLoY + LGBinH - LGBinH / (double)num;
            double rangeold = max(1.0, oldHiY - oldLoY);
            double rangenew = newHiY - newLoY;
            double scale = rangenew / rangeold;
            for (auto ci = cellFm; ci != cellTo; ++ci) {
                cellY[*ci] = newLoY + (cellY[*ci] - oldLoY) * scale;
            }
        }
#endif
        cellFm = cellTo;
    }
    ce = hibox.cells.end();
    cellFm = hibox.cells.begin();
    cellTo = hibox.cells.begin();
    for (int y = hibox.ly; y <= hibox.hy; y++) {
        if (cellFm == ce) break;
#ifdef EVENLY_SPREAD
        double cArea = 0;
        int num = 0;
        for (cellTo = cellFm; cellTo != ce && (y == hibox.hy || cArea < uArea[y - box.ly]); ++cellTo) {
            cArea += cellW[*cellTo] * cellH[*cellTo];
            num++;
        }
        if (num > 0) {
            double step = LGBinH / (double)num;
            double nowy = coreLY + y * LGBinH + step / 2.0;
            for (auto ci = cellFm; ci != cellTo; ++ci) {
                cellY[*ci] = nowy;
                nowy += step;
            }
        }
#else
        double cArea = 0;
        int num = 0;
        double oldLoY = cellY[*cellFm];
        double oldHiY = oldLoY;
        for (cellTo = cellFm; cellTo != ce && (y == hibox.hy || cArea < uArea[y - box.ly]); ++cellTo) {
            cArea += cellW[*cellTo] * cellH[*cellTo];
            oldHiY = cellY[*cellTo];  // increasing Y
            num++;
        }
        if (num > 0) {
            double newLoY = coreLY + y * LGBinH + LGBinH / (double)(num * 2);
            double newHiY = newLoY + LGBinH - LGBinH / (double)num;
            double rangeold = max(1.0, oldHiY - oldLoY);
            double rangenew = newHiY - newLoY;
            double scale = rangenew / rangeold;
            for (auto ci = cellFm; ci != cellTo; ++ci) {
                cellY[*ci] = newLoY + (cellY[*ci] - oldLoY) * scale;
            }
        }
#endif
        cellFm = cellTo;
    }
    return true;
}

void expandLGRegion(OFBin& ofBin) {
    ofBin.lx = ofBin.hx = ofBin.x;
    ofBin.ly = ofBin.hy = ofBin.y;
    ofBin.exFArea = LGGrid[ofBin.region][ofBin.x][ofBin.y].fArea;
    ofBin.exUArea = LGGrid[ofBin.region][ofBin.x][ofBin.y].uArea;
    ofBin.exCArea = LGGrid[ofBin.region][ofBin.x][ofBin.y].cArea;
    char direction = 0;
    double boxW = 1.0;
    double boxH = 1.0;
    double dirXRatio;
    double dirYRatio;
    double dirXCost;
    double dirYCost;
    bool disableBoxRatio;
    double expandBoxRatio = gpSetting.expandBoxRatio;
    double expandBoxLimit = gpSetting.expandBoxLimit;
    if (expandBoxRatio == 0.0) {
        disableBoxRatio = true;
    } else {
        disableBoxRatio = false;
    }

    for (; ofBin.exCArea > ofBin.exUArea; direction = (direction + 1) % 4) {
        if (!disableBoxRatio) {
            dirXRatio = (boxW + 1.0) / boxH;
            dirYRatio = boxW / (boxH + 1.0);
            if (dirXRatio < expandBoxRatio) {
                dirXCost = expandBoxRatio / dirXRatio;
            } else {
                dirXCost = dirXRatio / expandBoxRatio;
            }
            if (dirYRatio < expandBoxRatio) {
                dirYCost = expandBoxRatio / dirYRatio;
            } else {
                dirYCost = dirYRatio / expandBoxRatio;
            }
        }
        if (direction == 0 && ofBin.lx > 0 && (disableBoxRatio || dirXCost <= dirYCost)) {
            // expand to left
            ofBin.lx--;
            boxW += 1.0;
            for (int y = ofBin.ly; y <= ofBin.hy; y++) {
                ofBin.exFArea += LGGrid[ofBin.region][ofBin.lx][y].fArea;
                ofBin.exUArea += LGGrid[ofBin.region][ofBin.lx][y].uArea;
                ofBin.exCArea += LGGrid[ofBin.region][ofBin.lx][y].cArea;
            }
        } else if (direction == 1 && ofBin.ly > 0 && (disableBoxRatio || dirYCost <= dirXCost)) {
            // expand to bottom
            ofBin.ly--;
            boxH += 1.0;
            for (int x = ofBin.lx; x <= ofBin.hx; x++) {
                ofBin.exFArea += LGGrid[ofBin.region][x][ofBin.ly].fArea;
                ofBin.exUArea += LGGrid[ofBin.region][x][ofBin.ly].uArea;
                ofBin.exCArea += LGGrid[ofBin.region][x][ofBin.ly].cArea;
            }
        } else if (direction == 2 && ofBin.hx < LGNumX - 1 && (disableBoxRatio || dirXCost <= dirYCost)) {
            // expand to right
            ofBin.hx++;
            boxW += 1.0;
            for (int y = ofBin.ly; y <= ofBin.hy; y++) {
                ofBin.exFArea += LGGrid[ofBin.region][ofBin.hx][y].fArea;
                ofBin.exUArea += LGGrid[ofBin.region][ofBin.hx][y].uArea;
                ofBin.exCArea += LGGrid[ofBin.region][ofBin.hx][y].cArea;
            }
        } else if (direction == 3 && ofBin.hy < LGNumY - 1 && (disableBoxRatio || dirYCost <= dirXCost)) {
            // expand to top
            ofBin.hy++;
            boxH += 1.0;
            for (int x = ofBin.lx; x <= ofBin.hx; x++) {
                ofBin.exFArea += LGGrid[ofBin.region][x][ofBin.hy].fArea;
                ofBin.exUArea += LGGrid[ofBin.region][x][ofBin.hy].uArea;
                ofBin.exCArea += LGGrid[ofBin.region][x][ofBin.hy].cArea;
            }
        }
        bool fullyExpandedX = (ofBin.lx == 0 && ofBin.hx == LGNumX - 1);
        bool fullyExpandedY = (ofBin.ly == 0 && ofBin.hy == LGNumY - 1);
        if (disableBoxRatio) {
            if (fullyExpandedX && fullyExpandedY) {
                break;
            }
        } else {
            if (fullyExpandedX || fullyExpandedY) {
                // disableBoxRatio = true;
                break;
            }
        }
        if (ofBin.exFArea / totalFArea[ofBin.region] > expandBoxLimit) {
            break;
        }
    }

    if (expandBoxLimit == 1.0 && expandBoxRatio == 0.0 && ofBin.exUArea < ofBin.exCArea) {
        printlog(LOG_ERROR, "ERROR: expand fail %d < %d", (int)ofBin.exUArea, (int)ofBin.exCArea);
    }
    if (ofBin.lx == 0 && ofBin.ly == 0 && ofBin.hx == LGNumX - 1 && ofBin.hy == LGNumY - 1) {
        printlog(LOG_WARN, "WARN: expand to chip size");
    }
}

void spreadCellsForABin(OFBin& maxBin, int r)  // r is the fence id
{
    if (OFMap[r][maxBin.x][maxBin.y] == 0) return;  // overfill have been resolved

    // find expand region
    expandLGRegion(maxBin);
    ExpandBox box(maxBin.lx, maxBin.ly, maxBin.hx, maxBin.hy, maxLevel);

    // get list of cells in expanded region
    for (int x = maxBin.lx; x <= maxBin.hx; x++) {
        for (int y = maxBin.ly; y <= maxBin.hy; y++) {
            OFMap[r][x][y] = 0;
            auto& lcells = LGGrid[r][x][y].cells;
            box.cells.insert(box.cells.end(), lcells.begin(), lcells.end());  // better to "reserve"
        }
    }
    auto cellsCopy = box.cells;
    queue<ExpandBox> boxes;
    boxes.push(move(box));

    while (!boxes.empty()) {
        auto box = move(boxes.front());
        boxes.pop();

        ExpandBox lobox, hibox;
        lobox.level = box.level - 1;
        hibox.level = box.level - 1;

        if (box.hx - box.lx < minExpandSize - 1 && box.hy - box.ly < minExpandSize - 1) {
            continue;
        }

        bool success = false;
        if (box.dir == 'h') {
            if (box.hx - box.lx >= minExpandSize - 1) {
                sort(box.cells.begin(), box.cells.end(), cmpX);
                success = spreadCellsH(r, box, lobox, hibox);
            }
            if (!success && box.hy - box.ly >= minExpandSize - 1) {
                sort(box.cells.begin(), box.cells.end(), cmpY);
                success = spreadCellsV(r, box, lobox, hibox);
            }
            lobox.dir = 'v';
            hibox.dir = 'v';
        } else {
            if (box.hy - box.ly >= minExpandSize - 1) {
                sort(box.cells.begin(), box.cells.end(), cmpY);
                success = spreadCellsV(r, box, lobox, hibox);
            }
            if (!success && box.hx - box.lx >= minExpandSize - 1) {
                sort(box.cells.begin(), box.cells.end(), cmpX);
                success = spreadCellsH(r, box, lobox, hibox);
            }
            lobox.dir = 'h';
            hibox.dir = 'h';
        }
        if (success) {
            if (lobox.cells.size() > 0 && lobox.level > 0) boxes.push(move(lobox));
            if (hibox.cells.size() > 0 && hibox.level > 0) boxes.push(move(hibox));
        }
    }
    updateLGGridFast(box.lx, box.ly, box.hx, box.hy, r, cellsCopy);
}

void spreadCells(int binSize, int times) {
    vector<double> _cellX(numCells);
    vector<double> _cellY(numCells);

    initSpreadCells(binSize);
    updateTargetDensity();
    updateBinUsableArea();

    for (int iter = 0; iter < times; iter++) {  // start an iteration
        updateLGGrid();
        findOverfillBins();
        // if(nOF>0){
        //     avgOF=totOF/nOF;
        //     printlog(LOG_INFO, "\tRoughLegalize #%d OF: avg=%lf max=%lf #=%d", iter, avgOF, maxOF, nOF);
        // }else{
        //     printlog(LOG_INFO, "\tRoughLegalize #%d OF: none", iter);
        // }

        copy(cellX.begin(), cellX.begin() + numCells, _cellX.begin());
        copy(cellY.begin(), cellY.begin() + numCells, _cellY.begin());

        for (int r = 0; r < numFences; r++) {  // start a fence
            if (OFBins[r].size() == 0) continue;
            OFBins[r].sort();

            // legalize all overfilled bins
            while (OFBins[r].size() > 0) {
                spreadCellsForABin(OFBins[r].front(), r);
                OFBins[r].pop_front();
            }
        }  // finish a fence

        double dispx = 0.0;
        double dispy = 0.0;
        for (int i = 0; i < numCells; i++) {
            dispx += std::abs(cellX[i] - _cellX[i]);
            dispy += std::abs(cellY[i] - _cellY[i]);
        }
        printlog(LOG_VERBOSE, "disp = %d + %d = %d", (int)dispx, (int)dispy, (int)(dispx + dispy));
        if ((int)(dispx + dispy) == 0) break;
    }  // finish an iteration

    // updateLGGrid(0, 0, LGNumX-1, LGNumY-1);
    // for(int r=0; r<numFences; r++){
    //     for(int x=0; x<LGNumX; x++){
    //         for(int y=0; y<LGNumY; y++){
    //             LGBin &bin=LGGrid[r][x][y];
    //             ExpandBox box(x, y, x, y, 0, bin.cells);
    //             spreadCellsInBoxEven(r, box);
    //             spreadCellsInBoxOverlapFree(r, box);
    //             spreadCellsInBoxFullLegal(r, box);
    //         }
    //     }
    // }
}