#include "gp_data.h"
#include "gp_region.h"

void findCellRegionDist() {
    for (int i = 0; i < numCells; i++) {
        // for each cell, find nearest region block
        double x = cellX[i];
        double y = cellY[i];
        int region = cellFence[i];
        vector<FRect>::iterator ri = regions[region].rects.begin();
        vector<FRect>::iterator re = regions[region].rects.end();
        double dist = -1;
        double regionx = -1;
        double regiony = -1;
        int regionr = -1;
        for (int r = 0; ri != re; ++ri, r++) {
            double distx = 0;
            double disty = 0;
            double rx = x;
            double ry = y;
            double lx = ri->lx;
            double ly = ri->ly;
            double hx = ri->hx;
            double hy = ri->hy;
            if (x < lx) {
                distx = lx - x;
                rx = lx;
            } else if (x > hx) {
                distx = x - hx;
                rx = hx;
            }
            if (y < ly) {
                disty = ly - y;
                ry = ly;
            } else if (y > hy) {
                disty = y - hy;
                ry = hy;
            }
            if (r == 0 || distx + disty < dist) {
                dist = distx + disty;
                regionx = rx;
                regiony = ry;
                regionr = r;
                if (dist == 0) {
                    break;
                }
            }
        }
        if (regionr < 0) {
            cerr << "no rect defined for region " << region << endl;
            exit(1);
        }
        cellFenceX[i] = regionx;
        cellFenceY[i] = regiony;
        cellFenceRect[i] = regionr;
        cellFenceDist[i] = dist;
    }
}