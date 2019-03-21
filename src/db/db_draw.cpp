#include "db/db.h"
using namespace db;

void Database::saveDraw(string file) {
    if (canvas == NULL) {
        return;
    }
    canvas->Save(file);
}

void Database::resetDraw() {
    if (canvas == NULL) {
        canvas = new DrawCanvas(1000, 0, 0, 0, sitemap_nx, sitemap_ny);
    } else {
        canvas->Clear();
    }
}

void Database::resetDraw(int color) {
    if (canvas == NULL) {
        canvas = new DrawCanvas(1000, 0, 0, 0, sitemap_nx, sitemap_ny);
    }
    canvas->Clear(color);
}

void Database::resetDraw(unsigned char r, unsigned char g, unsigned char b) {
    if (canvas == NULL) {
        canvas = new DrawCanvas(1000, 0, 0, 0, sitemap_nx, sitemap_ny);
    } else {
        canvas->Clear(r, g, b);
    }
}

void Database::draw(DrawType type) {
    switch (type) {
        case DrawSites:
            drawSites();
            break;
        case DrawInstances:
            drawInstances();
            break;
        case DrawFixedPins:
            drawFixedPins();
            break;
        default:
            printlog(LOG_INFO, "Drawing type not implemented");
    }
}

void Database::draw(string file, vector<DrawType> &types) {
    for (int i = 0; i < (int)types.size(); i++) {
        draw(types[i]);
    }
    saveDraw(file);
}

void Database::draw(string file, DrawType type) {
    if (canvas == NULL) {
        resetDraw();
    }
    draw(type);
    saveDraw(file);
}

/***** Draw database objects *****/

void Database::drawSites() {
    unordered_map<SiteType *, int> colors;

    for (int i = 0; i < (int)sitetypes.size(); i++) {
        switch (i) {
            case 0:
                colors[sitetypes[i]] = 0xff0000;
                break;
            case 1:
                colors[sitetypes[i]] = 0x00aa00;
                break;
            case 2:
                colors[sitetypes[i]] = 0x6666ff;
                break;
            case 3:
                colors[sitetypes[i]] = 0xaa9900;
                break;
            case 4:
                colors[sitetypes[i]] = 0x00aaaa;
                break;
            case 5:
                colors[sitetypes[i]] = 0x990099;
                break;
            default:
                colors[sitetypes[i]] = 0xaaaaaa;
        }
    }

    for (int x = 0; x < sitemap_nx; x++) {
        for (int y = 0; y < sitemap_ny; y++) {
            Site *site = sites[x][y];
            if (site != NULL && site->x == x && site->y == y) {
                int color = colors[site->type];
                canvas->SetFillColor(color);
                canvas->SetLineColor(0, 0, 0);
                canvas->DrawRect(site->x, site->y, site->x + site->w, site->y + site->h, true, 1);
            }
        }
    }
}

void Database::drawInstances() {
    unordered_map<SiteType *, int> colors;

    for (int i = 0; i < (int)sitetypes.size(); i++) {
        switch (i) {
            case 0:
                colors[sitetypes[i]] = 0xff0000;
                break;
            case 1:
                colors[sitetypes[i]] = 0x00aa00;
                break;
            case 2:
                colors[sitetypes[i]] = 0x6666ff;
                break;
            case 3:
                colors[sitetypes[i]] = 0xaa9900;
                break;
            case 4:
                colors[sitetypes[i]] = 0x00aaaa;
                break;
            case 5:
                colors[sitetypes[i]] = 0x990099;
                break;
            default:
                colors[sitetypes[i]] = 0xaaaaaa;
        }
    }

    for (auto instance : instances) {
        SiteType *sitetype = NULL;
        for (auto s : sitetypes) {
            if (s->matchInstance(instance)) {
                sitetype = s;
                break;
            }
        }
        if (sitetype == NULL) {
            printlog(LOG_ERROR, "instance site type not found");
            break;
        }
        int color = colors[sitetype];
        canvas->SetFillColor(color);
        int x = instance->pack->site->x;
        int y = instance->pack->site->y;
        int w = instance->pack->site->w;
        int h = instance->pack->site->h;
        canvas->DrawRect(x, y, x + w, y + h, true);
    }
}

void Database::drawFixedPins() {
    for (auto instance : instances) {
        if (!instance->fixed) continue;
        canvas->SetFillColor(0xff0000);
        int x = instance->pack->site->x;
        int y = instance->pack->site->y;
        int w = instance->pack->site->w;
        int h = instance->pack->site->h;
        canvas->DrawPoint(x + 0.5 * w, y + 0.5 * h, 5);
    }
}