#include "matching.h"

int DisjGraph::CalcMaxWeightMatch(vector<Match> &matches) {
    sort(vertices.begin(), vertices.end()); // for deterministic behavior

    unordered_map<int, int> old2new;
    for (size_t i=0; i<vertices.size(); ++i) old2new[vertices[i]] = i+1;
    GenGraph g(vertices.size());
    for (int i = 0; i < (int)u.size(); i++) g.AddEdge(old2new[u[i]], old2new[v[i]], w[i]);
    g.CalcMaxWeightMatch();
    vector<Match> curMatches = g.GetMate();

    //map node number to instance
    for (int i = 0; i < (int)curMatches.size(); i++) {
        matches.push_back(Match(vertices[curMatches[i].u - 1], vertices[curMatches[i].v - 1]));
    }
    return g.GetTotWeight();
}

void DisjGraph::GetDisjointGraphs(const vector<unordered_set<unsigned>>& graph, vector<DisjGraph>& vDisjGraph)
{
    vector<bool> processed(graph.size(), false);
    for (unsigned i = 0; i < graph.size(); ++i)
        if (graph[i].size() != 0 && !processed[i])
        {
            queue<unsigned> searchQ;
            DisjGraph disjG;
            searchQ.push(i);
            processed[i] = true;
            while (!searchQ.empty())
            {
                unsigned curV = searchQ.front(); searchQ.pop();
                disjG.vertices.push_back(curV);
                for (auto adj : graph[curV])
                {
                    if (!processed[adj])
                    {
                        searchQ.push(adj);
                        processed[adj] = true;
                    }
                    if (adj > curV)
                    {
                        disjG.u.push_back(curV);
                        disjG.v.push_back(adj);
                        disjG.w.push_back(1);
                    }
                }
            }
            vDisjGraph.push_back(disjG);
        }
}

void DisjGraph::StatGraphSize(const vector<DisjGraph>& graphs, unsigned step)
{
    vector<unsigned> count;
    for (auto g : graphs)
    {
        unsigned n = g.vertices.size() / step;
        if (n >= count.size()) count.resize(n + 1, 0);
        ++count[n];
    }
    for (unsigned i = 0; i < count.size(); ++i)
        if (count[i] != 0) printlog(LOG_INFO, "\t(%4d-%4d)-vertex graph #: %d", i*step, (i + 1)*step - 1, count[i]);
}

void DisjGraph::DivideByRegions(vector<vector<int>>& pointSets, vector<Point2d>& poss, unsigned maxSize)
{
    assert(pointSets.size() == 1);
    // too many or not?
    unsigned oriSize = pointSets[0].size();
    bool tooMany = false;
    for (const auto& pointSet : pointSets)
        if (pointSet.size() > maxSize){
            tooMany = true;
            break;
        }
    if (!tooMany) return;
    // update bounds
    vector<Box<double>> bounds(pointSets.size());
    for (unsigned i = 0; i < pointSets.size(); ++i)
        for (auto v : pointSets[i])
            bounds[i].update(poss[v].y, poss[v].x);
    while (tooMany)
    {
        tooMany = false;
        for (unsigned i = 0; i < pointSets.size(); ++i)
        {
            if (pointSets[i].size() > maxSize)
            {
                tooMany = true;
                vector<int> tempSet = pointSets[i];
                pointSets[i].clear();
                pointSets.push_back(vector<int>(0));
                if ((bounds[i].y()) > (bounds[i].x()))
                {// horizontally divide
                    double boundary = bounds[i].cy();
                    for (auto v : tempSet)
                    {
                        if (poss[v].y > boundary) pointSets[i].push_back(v);
                        else pointSets.back().push_back(v);
                    }
                    bounds.push_back(bounds[i]);
                    bounds.back().uy() = boundary;
                    bounds[i].ly() = boundary;
                }
                else
                {// vertically divide
                    double boundary = bounds[i].cx();
                    for (auto v : tempSet)
                    {
                        if (poss[v].x > boundary) pointSets[i].push_back(v);
                        else pointSets.back().push_back(v);
                    }
                    bounds.push_back(bounds[i]);
                    bounds.back().ux() = boundary;
                    bounds[i].lx() = boundary;
                }
            }
        }
    }
    printlog(LOG_INFO, "\t a disjgraph with %d vertices is divided into %d regions", oriSize, pointSets.size());
    //for (unsigned i = 0; i < graphs.size(); ++i)
    //    printlog(LOG_INFO, "\tsize:%d, lo:%lf, up:%lf, le:%lf, ri:%lf", (int)graphs[i].vertices.size(), bounds[i].lower, bounds[i].upper, bounds[i].left, bounds[i].right);
}

template <class T>
inline void tension(T &a, const T &b) {
    if (b < a)
        a = b;
}

template <class T>
inline void relax(T &a, const T &b) {
    if (b > a)
        a = b;
}

int GenGraph::GetEdgeDelta(const Edge &e) {
    return lab[e.v] + lab[e.u] - mat[e.v][e.u].w * 2;
}

void GenGraph::UpdateSlackV(int v, int x) {
    if (!slackv[x] || GetEdgeDelta(mat[v][x]) < GetEdgeDelta(mat[slackv[x]][x]))
        slackv[x] = v;
}

void GenGraph::CalcSlackV(int x) {
    slackv[x] = 0;
    for (int v = 1; v <= n; v++)
        if (mat[v][x].w > 0 && bel[v] != x && col[bel[v]] == 0)
            UpdateSlackV(v, x);
}

void GenGraph::QPush(int x) {
    if (x <= n)
        q[q_n++] = x;
    else
    {
        for (int i = 0; i < (int)bloch[x].size(); i++)
            QPush(bloch[x][i]);
    }
}

void GenGraph::SetMate(int xv, int xu) {
    mate[xv] = mat[xv][xu].u;
    if (xv > n) {
        Edge e = mat[xv][xu];
        int xr = blofrom[xv][e.v];
        int pr = find(bloch[xv].begin(), bloch[xv].end(), xr) - bloch[xv].begin();
        if (pr % 2 == 1)
        {
            reverse(bloch[xv].begin() + 1, bloch[xv].end());
            pr = bloch[xv].size() - pr;
        }

        for (int i = 0; i < pr; i++)
            SetMate(bloch[xv][i], bloch[xv][i ^ 1]);
        SetMate(xr, xu);

        rotate(bloch[xv].begin(), bloch[xv].begin() + pr, bloch[xv].end());
    }
}

void GenGraph::SetBel(int x, int b) {
    bel[x] = b;
    if (x > n) {
        for (int i = 0; i < (int)bloch[x].size(); i++)
            SetBel(bloch[x][i], b);
    }
}

void GenGraph::Augment(int xv, int xu) {
    while (true) {
        int xnu = bel[mate[xv]];
        SetMate(xv, xu);
        if (!xnu)
            return;
        SetMate(xnu, bel[fa[xnu]]);
        xv = bel[fa[xnu]], xu = xnu;
    }
}

int GenGraph::GetLca(int xv, int xu) {
    for (int x = 1; x <= n_x; x++)
        book[x] = false;
    while (xv || xu) {
        if (xv) {
            if (book[xv])
                return xv;
            book[xv] = true;
            xv = bel[mate[xv]];
            if (xv)
                xv = bel[fa[xv]];
        }
        swap(xv, xu);
    }
    return 0;
}

void GenGraph::AddBlossom(int xv, int xa, int xu) {
    int b = n + 1;
    while (b <= n_x && bel[b])
        b++;
    if (b > n_x)
        n_x++;

    lab[b] = 0;
    col[b] = 0;

    mate[b] = mate[xa];

    bloch[b].clear();
    bloch[b].push_back(xa);
    for (int x = xv; x != xa; x = bel[fa[bel[mate[x]]]])
        bloch[b].push_back(x), bloch[b].push_back(bel[mate[x]]), QPush(bel[mate[x]]);
    reverse(bloch[b].begin() + 1, bloch[b].end());
    for (int x = xu; x != xa; x = bel[fa[bel[mate[x]]]])
        bloch[b].push_back(x), bloch[b].push_back(bel[mate[x]]), QPush(bel[mate[x]]);

    SetBel(b, b);

    for (int x = 1; x <= n_x; x++) {
        mat[b][x].w = mat[x][b].w = 0;
        blofrom[b][x] = 0;
    }
    for (int i = 0; i < (int)bloch[b].size(); i++) {
        int xs = bloch[b][i];
        for (int x = 1; x <= n_x; x++)
            if (mat[b][x].w == 0 || GetEdgeDelta(mat[xs][x]) < GetEdgeDelta(mat[b][x]))
                mat[b][x] = mat[xs][x], mat[x][b] = mat[x][xs];
        for (int x = 1; x <= n_x; x++)
            if (blofrom[xs][x])
                blofrom[b][x] = xs;
    }
    CalcSlackV(b);
}

void GenGraph::ExpandBlossom(int b) {
    for (int i = 0; i < (int)bloch[b].size(); i++)
        SetBel(bloch[b][i], bloch[b][i]);

    int xr = blofrom[b][mat[b][fa[b]].v];
    int pr = find(bloch[b].begin(), bloch[b].end(), xr) - bloch[b].begin();
    if (pr % 2 == 1) {
        reverse(bloch[b].begin() + 1, bloch[b].end());
        pr = bloch[b].size() - pr;
    }

    for (int i = 0; i < pr; i += 2) {
        int xs = bloch[b][i], xns = bloch[b][i + 1];
        fa[xs] = mat[xns][xs].v;
        col[xs] = 1, col[xns] = 0;
        slackv[xs] = 0, CalcSlackV(xns);
        QPush(xns);
    }
    col[xr] = 1;
    fa[xr] = fa[b];
    for (int i = pr + 1; i < (int)bloch[b].size(); i++) {
        int xs = bloch[b][i];
        col[xs] = -1;
        CalcSlackV(xs);
    }

    bel[b] = 0;
}

void GenGraph::ExpandBlossomFinal(int b) {
    for (int i = 0; i < (int)bloch[b].size(); i++) {
        if (bloch[b][i] > n && lab[bloch[b][i]] == 0)
            ExpandBlossomFinal(bloch[b][i]);
        else
            SetBel(bloch[b][i], bloch[b][i]);
    }
    bel[b] = 0;
}

bool GenGraph::OnFoundEdge(const Edge &e) {
    int xv = bel[e.v], xu = bel[e.u];
    if (col[xu] == -1) {
        int nv = bel[mate[xu]];
        fa[xu] = e.v;
        col[xu] = 1, col[nv] = 0;
        slackv[xu] = slackv[nv] = 0;
        QPush(nv);
    }
    else if (col[xu] == 0) {
        int xa = GetLca(xv, xu);
        if (!xa) {
            Augment(xv, xu), Augment(xu, xv);
            for (int b = n + 1; b <= n_x; b++)
                if (bel[b] == b && lab[b] == 0)
                    ExpandBlossomFinal(b);
            return true;
        }
        else
            AddBlossom(xv, xa, xu);
    }
    return false;
}

bool GenGraph::DoMatching() {
    for (int x = 1; x <= n_x; x++)
        col[x] = -1, slackv[x] = 0;

    q_n = 0;
    for (int x = 1; x <= n_x; x++)
        if (bel[x] == x && !mate[x])
            fa[x] = 0, col[x] = 0, slackv[x] = 0, QPush(x);
    if (q_n == 0)
        return false;

    while (true) {
        for (int i = 0; i < q_n; i++) {
            int v = q[i];
            for (int u = 1; u <= n; u++)
                if (mat[v][u].w > 0 && bel[v] != bel[u]) {
                    int d = GetEdgeDelta(mat[v][u]);
                    if (d == 0) {
                        if (OnFoundEdge(mat[v][u]))
                            return true;
                    }
                    else if (col[bel[u]] == -1 || col[bel[u]] == 0)
                        UpdateSlackV(v, bel[u]);
                }
        }

        int d = INF;
        for (int v = 1; v <= n; v++)
            if (col[bel[v]] == 0)
                tension(d, lab[v]);
        for (int b = n + 1; b <= n_x; b++)
            if (bel[b] == b && col[b] == 1)
                tension(d, lab[b] / 2);
        for (int x = 1; x <= n_x; x++)
            if (bel[x] == x && slackv[x]) {
                if (col[x] == -1)
                    tension(d, GetEdgeDelta(mat[slackv[x]][x]));
                else if (col[x] == 0)
                    tension(d, GetEdgeDelta(mat[slackv[x]][x]) / 2);
            }

        for (int v = 1; v <= n; v++) {
            if (col[bel[v]] == 0)
                lab[v] -= d;
            else if (col[bel[v]] == 1)
                lab[v] += d;
        }
        for (int b = n + 1; b <= n_x; b++)
            if (bel[b] == b) {
                if (col[bel[b]] == 0)
                    lab[b] += d * 2;
                else if (col[bel[b]] == 1)
                    lab[b] -= d * 2;
            }

        q_n = 0;
        for (int v = 1; v <= n; v++)
            if (lab[v] == 0)
                return false;
        for (int x = 1; x <= n_x; x++)
            if (bel[x] == x && slackv[x] && bel[slackv[x]] != x && GetEdgeDelta(mat[slackv[x]][x]) == 0) {
                if (OnFoundEdge(mat[slackv[x]][x]))
                    return true;
            }
        for (int b = n + 1; b <= n_x; b++)
            if (bel[b] == b && col[b] == 1 && lab[b] == 0)
                ExpandBlossom(b);
    }
    return false;
}

void GenGraph::CalcMaxWeightMatch() {
    for (int v = 1; v <= n; v++)
        mate[v] = 0;

    n_x = n;
    nMatches = 0;
    totWeight = 0;

    bel[0] = 0;
    for (int v = 1; v <= n; v++)
        bel[v] = v, bloch[v].clear();
    for (int v = 1; v <= n; v++)
        for (int u = 1; u <= n; u++)
            blofrom[v][u] = v == u ? v : 0;

    int w_max = 0;
    for (int v = 1; v <= n; v++)
        for (int u = 1; u <= n; u++)
            relax(w_max, mat[v][u].w);
    for (int v = 1; v <= n; v++)
        lab[v] = w_max;

    while (DoMatching())
        nMatches++;

    for (int v = 1; v <= n; v++)
        if (mate[v] && mate[v] < v)
            totWeight += mat[v][mate[v]].w;
}

GenGraph::GenGraph(int _n) {
    if (_n > 20000) {
        cout << "warning, too many vertices" << endl;
    }
    else {
        n = 2 * _n;

        mate.resize(n + 1);
        lab.resize(n + 1);
        q.resize(n + 1);
        fa.resize(n + 1);
        col.resize(n + 1);
        slackv.resize(n + 1);
        bel.resize(n + 1);
        blofrom.resize(n + 1);
        mat.resize(n + 1);
        for (int i = 1; i <= n; i++) {
            blofrom[i].resize(n + 1);
            mat[i].resize(n + 1);
        }
        bloch.resize(n + 1);
        book.resize(n + 1);

        n = _n;
        for (int v = 1; v <= n; v++)
            for (int u = 1; u <= n; u++)
                mat[v][u] = Edge(v, u, 0);
    }

}

GenGraph::~GenGraph() {

}

void GenGraph::AddEdge(int u, int v, int w) {
    mat[v][u].w = mat[u][v].w = w;
}

s64 GenGraph::GetTotWeight() {
    return totWeight;
}

vector<Match> GenGraph::GetMate() {
    vector<Match> matches;
    for (int i = 1; i <= n; i++) {
        if (mate[i] == 0)
            continue;
        if (i <= mate[i])
            matches.push_back(Match(i, mate[i]));
    }
    return matches;
}