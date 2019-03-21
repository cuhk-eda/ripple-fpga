#include "gp_setting.h"
#include "gp_data.h"
#include "gp_qsolve.h"
#include "gp_spread.h"
#include "gp_main.h"
#include "global.h"
#include "lg/lg.h"

int colors[4] = {0xff0000, 0xff9900, 0x009900, 0x00ccff};

void drawCell(string file) {
#ifdef DRAW
    database.resetDraw(0xffffff);
    for (int i = 0; i < numNodes; i++) {
        int color;
        if (i < numCells) {
            int fid = cellFence[i];
            if (fid < 4)
                color = colors[fid];
            else
                color = 0;
        } else
            color = 0x0000ff;
        database.canvas->SetFillColor(color);
        database.canvas->DrawPoint(cellX[i], cellY[i], 1);
    }
    database.saveDraw(file + ".png");
#endif
}

void drawNet(string file) {
#ifdef DRAW
    database.resetDraw(0xffffff);
    vector<vector<double> > slices(database.sitemap_nx, vector<double>(database.sitemap_ny, 0.0));
    int nNets = netCell.size();
    for (int i = 0; i < nNets; i++) {
        int nPins = netCell[i].size();
        double lx = coreHX;
        double ly = coreHY;
        double hx = coreLX;
        double hy = coreLY;
        for (int j = 0; j < nPins; j++) {
            double px = cellX[netCell[i][j]];
            double py = cellY[netCell[i][j]];
            lx = min(lx, px);
            ly = min(ly, py);
            hx = max(hx, px);
            hy = max(hy, py);
        }
        double hpwl = (hx - lx) + (hy - ly);
        lx = max(0, min(database.sitemap_nx - 1, (int)floor(lx)));
        ly = max(0, min(database.sitemap_ny - 1, (int)floor(ly)));
        hx = max(0, min(database.sitemap_nx - 1, (int)ceil(hx)));
        hy = max(0, min(database.sitemap_ny - 1, (int)ceil(hy)));

        double value = netWeight[i] * hpwl / ((hx - lx + 1) * (hy - ly + 1));
        for (int x = lx; x <= hx; x++) {
            for (int y = ly; y <= hy; y++) {
                slices[x][y] += value;
            }
        }
    }
    double maxValue = 0;
    for (int x = 0; x < database.sitemap_nx; x++) {
        for (int y = 0; y < database.sitemap_ny; y++) {
            if (slices[x][y] > maxValue) {
                maxValue = slices[x][y];
            }
        }
    }
    for (int x = 0; x < database.sitemap_nx; x++) {
        for (int y = 0; y < database.sitemap_ny; y++) {
            double v = (double)slices[x][y] / (double)maxValue;
            database.canvas->SetFillColor(v, 1);
            database.canvas->DrawRect(x, y, x + 1, y + 1, true, -1);
        }
    }
    database.saveDraw(file + ".png");
#endif
}

void upperBound(int binSize, int legalTimes) { spreadCells(binSize, legalTimes); }

void GP_Main() {
    static int GPCount = 0;
    ++GPCount;
    const auto& gs = gpSetting;
    double PWBegin = gs.pseudoNetWeightBegin;
    double PWEnd = gs.pseudoNetWeightEnd;
    double PWStep = (gs.mainWLIter < 2) ? 0 : (PWEnd - PWBegin) / (gs.mainWLIter - 1);

    printlog(LOG_INFO,
             "#nodes: %d, #nets: %d, #pins: %d, #cells: %d, #macros: %d, #pads: %d, #fences: %d",
             numNodes,
             numNets,
             numPins,
             numCells,
             numMacros,
             numPads,
             numFences);
    printlog(LOG_INFO, "Input WL:  %ld", (long)hpwl());

    double hpwlxy, hpwlx, hpwly;

    if (gs.initIter > 0) {
        //--Random placement
        for (int i = 0; i < numCells; i++) {
            cellX[i] = getrand((double)(coreLX + 0.5 * cellW[i]), (double)(coreHX - 0.5 * cellW[i]));
            cellY[i] = getrand((double)(coreLY + 0.5 * cellH[i]), (double)(coreHY - 0.5 * cellH[i]));
        }
        printlog(LOG_INFO, "Random WL: %ld", (long)hpwl());

        //--Initial placement (CG)
        lowerBound(0.001, 0.0, gs.initIter, LBModeSimple);
        printlog(LOG_INFO, "Init WL: %ld", (long)hpwl());
        drawCell("gp" + to_string(GPCount) + "_init");
    }

    //--Global Placement Iteration
    double PW = PWBegin;
    for (int iter = 1; iter <= gs.mainWLIter; iter++, PW += PWStep) {
        printlog(LOG_INFO, " = = = = GP iteration %d = = = =", iter);

        //--Upper Bound
        upperBound((database.crmap_nx == 0) ? 3 : 1, gs.ubIter);

        hpwlxy = hpwl(&hpwlx, &hpwly);
        printlog(LOG_INFO, "UB WL: %ld (x: %ld, y: %ld)", (long)hpwlxy, (long)hpwlx, (long)hpwly);
        drawCell("gp" + to_string(GPCount) + "_up" + to_string(iter));

        //--Lower Bound
        auto fence = (iter <= 8 || !gs.enableFence) ? LBModeSimple : LBModeFenceRect;
        lowerBound(0.001, PW, gs.lbIter, fence);

        hpwlxy = hpwl(&hpwlx, &hpwly);
        printlog(LOG_INFO, "LB WL: %ld (x: %ld, y: %ld)", (long)hpwlxy, (long)hpwlx, (long)hpwly);
        drawCell("gp" + to_string(GPCount) + "_lo" + to_string(iter));
    }

    //--Final Upper Bound
    if (gs.finalIter > 0) {
        upperBound((database.crmap_nx == 0) ? 3 : 1, gs.finalIter);
        hpwlxy = hpwl(&hpwlx, &hpwly);
        printlog(LOG_INFO, "UB WL: %ld (x: %ld, y: %ld)", (long)hpwlxy, (long)hpwlx, (long)hpwly);
    }
    drawCell("gp" + to_string(GPCount) + "_final");

    printlog(LOG_INFO, "Output WL: %ld", (long)hpwl());
}