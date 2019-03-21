#pragma once

#include "db/db.h"

using namespace db;

class Cluster {
public:
    static unsigned numNonEmpty;
    static unsigned numEmpty;
    unsigned id;

    vector<int> instIds;
    array<Cluster*, 2> children;
    unsigned cutSize = 0;

    Box<double> bbox;
    double avgY;
    double area;

    Cluster() {
        children[0] = NULL;
        children[1] = NULL;
    }
    ~Cluster() {
        delete children[0];
        delete children[1];
    }
    void InitSeed();
    void Partition();
    void UpdateInfo(const vector<Point2<double>>& instXY);
    unsigned maxCutSize();
    void PrintLeaf() const;
    void Print() const;
    void GetResult(vector<Cluster*>& clusters);
};

bool ReallocByCluster(vector<Group>& groups);

void PartitionPlFile();

// return cut size
unsigned Bipartition(const vector<int>& inputCluster, array<vector<int>, 2>& outputClusters);