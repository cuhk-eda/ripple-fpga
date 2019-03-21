#ifndef _MATCHING_H_
#define _MATCHING_H_

#include "db/db.h"
using namespace db;

typedef long long s64;

const int INF = 2147483647;

struct Match;

struct Point2d {
    double y, x;
    Point2d() : y(0), x(0) {}
};

class DisjGraph {
public:
    vector<int> vertices;
    vector<int> u;
    vector<int> v;
    vector<int> w;
    int CalcMaxWeightMatch(vector<Match> &matches);
    static void GetDisjointGraphs(const vector<unordered_set<unsigned>> &graph,
                                  vector<DisjGraph> &vDisjGraph);  // ignore single-vertex disjoint graph
    static void StatGraphSize(const vector<DisjGraph> &graphs, unsigned step = 20);
    static void DivideByRegions(vector<vector<int>> &pointSets, vector<Point2d> &poss, unsigned maxSize = 2000);
};

struct Edge {
    int v, u, w;
    Edge() {}
    Edge(const int &_v, const int &_u, const int &_w) : v(_v), u(_u), w(_w) {}
};

struct Match {
    int u, v;
    Match(int _u, int _v) : u(_u), v(_v) {}
};

class GenGraph {
private:
    int n;
    vector<vector<Edge>> mat;

    int nMatches;
    s64 totWeight;
    vector<int> mate;
    vector<int> lab;

    int q_n;
    vector<int> q;
    vector<int> fa;
    vector<int> col;
    vector<int> slackv;

    int n_x;
    vector<int> bel;
    vector<vector<int>> blofrom;
    vector<vector<int>> bloch;
    vector<bool> book;

    int GetEdgeDelta(const Edge &e);
    void UpdateSlackV(int v, int x);
    void CalcSlackV(int x);
    void QPush(int x);
    void SetMate(int xv, int xu);
    void SetBel(int x, int b);
    void Augment(int xv, int xu);
    int GetLca(int xv, int xu);
    void AddBlossom(int xv, int xa, int xu);
    void ExpandBlossom(int b);
    void ExpandBlossomFinal(int b);
    bool OnFoundEdge(const Edge &e);
    bool DoMatching();

public:
    GenGraph(int n);
    ~GenGraph();
    void AddEdge(int u, int v, int w);
    void CalcMaxWeightMatch();
    s64 GetTotWeight();
    vector<Match> GetMate();
};

#endif