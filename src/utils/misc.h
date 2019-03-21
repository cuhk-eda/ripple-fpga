#pragma once
#include "log.h"
#include "draw.h"

inline double getrand(double lo, double hi) { return (((double)rand() / (double)RAND_MAX) * (hi - lo) + lo); }

template <typename T>
void WriteMap(const string& fileNamePrefix, const vector<vector<T>>& map) {
#ifndef WRITE
    return;
#endif
    ofstream oStream;
    ostringstream fileName;
    oStream.open(fileNamePrefix + ".txt");

    for (const auto& col : map) {
        for (auto el : col) oStream << el << ' ';
        oStream << endl;
    }

    oStream.close();
}

template <typename T>
void DrawMap(const string& fileNamePrefix, const vector<vector<T>>& map, double xscale = 1.0, double yscale = 1.0) {
#ifndef DRAW
    return;
#endif
    double minValue = map[0][0], maxValue = map[0][0];
    for (unsigned x = 0; x < map.size(); x++)
        for (unsigned y = 0; y < map[0].size(); y++) {
            if (map[x][y] > maxValue)
                maxValue = map[x][y];
            else if (map[x][y] < minValue)
                minValue = map[x][y];
        }

    DrawCanvas canvas(xscale * map[0].size(), yscale * map.size(), 0, 0, map.size(), map[0].size());
    for (unsigned x = 0; x < map.size(); x++) {
        for (unsigned y = 0; y < map[0].size(); y++) {
            double v = (map[x][y] - minValue) / (maxValue - minValue);
            canvas.SetFillColor(v, 1);
            canvas.DrawRect(x, y, x + 1, y + 1);
        }
    }
    canvas.Save(fileNamePrefix + ".png");
}

template <typename T>
void DrawMap2(const string& fileNamePrefix,
              const vector<vector<T>>& map,
              double maxValue,
              double minValue = 0.0,
              double xscale = 1.0,
              double yscale = 1.0) {
#ifndef DRAW
    return;
#endif
    DrawCanvas canvas(xscale * map[0].size(), yscale * map.size(), 0, 0, map.size(), map[0].size());
    for (unsigned x = 0; x < map.size(); x++) {
        for (unsigned y = 0; y < map[0].size(); y++) {
            double v = map[x][y];
            if (v >= maxValue)
                v = 1;
            else if (v <= minValue)
                v = 0.0;
            else
                v = (v - minValue) / (maxValue - minValue);
            canvas.SetFillColor(v, 1);
            canvas.DrawRect(x, y, x + 1, y + 1);
        }
    }
    canvas.Save(fileNamePrefix + ".png");
}

template <typename T>
void PrintMapStat(const string& name, const vector<vector<T>>& map) {
    vector<unsigned> stat;
    for (const auto& col : map) {
        for (auto el : col) {
            if (el > 0) {
                if (el >= stat.size()) stat.resize(el + 1, 0);
                ++stat[el];
            }
        }
    }

    printlog(LOG_INFO, "\tStat of %s", name.c_str());
    for (unsigned i = 0; i < stat.size(); ++i)
        if (stat[i] != 0) printlog(LOG_INFO, "\t\t%d: %d ", i, stat[i]);
}

template <typename key, typename value>
struct sorting_order {
    static bool desc(const pair<key, value>& left, const pair<key, value>& right) { return left.first > right.first; }
    static bool asc(const pair<key, value>& left, const pair<key, value>& right) { return left.first < right.first; }
};