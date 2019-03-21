#include "bipartite.h"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/successive_shortest_path_nonnegative_weights.hpp>
#include <boost/graph/find_flow_cost.hpp>

using namespace boost;

typedef adjacency_list_traits<vecS, vecS, directedS> Traits;

typedef adjacency_list<
    vecS,
    vecS,
    directedS,
    no_property,
    property<edge_capacity_t,
             long,
             property<edge_residual_capacity_t,
                      long,
                      property<edge_reverse_t, Traits::edge_descriptor, property<edge_weight_t, long>>>>>
    Graph;
typedef property_map<Graph, edge_capacity_t>::type Capacity;
typedef property_map<Graph, edge_residual_capacity_t>::type ResidualCapacity;
typedef property_map<Graph, edge_weight_t>::type Weight;
typedef property_map<Graph, edge_reverse_t>::type Reversed;
typedef boost::graph_traits<Graph>::vertices_size_type size_type;
typedef Traits::vertex_descriptor vertex_descriptor;

template <class Graph, class Weight, class Capacity, class Reversed, class ResidualCapacity>
class EdgeAdder {
public:
    EdgeAdder(Graph& g, Weight& w, Capacity& c, Reversed& rev, ResidualCapacity&)
        : m_g(g), m_w(w), m_cap(c), m_rev(rev) {}
    void addEdge(vertex_descriptor v, vertex_descriptor w, long weight, long capacity) {
        Traits::edge_descriptor e, f;
        e = add(v, w, weight, capacity);
        f = add(w, v, -weight, 0);
        m_rev[e] = f;
        m_rev[f] = e;
    }

private:
    Traits::edge_descriptor add(vertex_descriptor v, vertex_descriptor w, long weight, long capacity) {
        bool b;
        Traits::edge_descriptor e;
        boost::tie(e, b) = add_edge(vertex(v, m_g), vertex(w, m_g), m_g);
        if (!b) {
            std::cerr << "Edge between " << v << " and " << w << " already exists." << std::endl;
            std::abort();
        }
        m_cap[e] = capacity;
        m_w[e] = weight;
        return e;
    }
    Graph& m_g;
    Weight& m_w;
    Capacity& m_cap;
    Reversed& m_rev;
};

void MinCostBipartiteMatching(const vector<vector<pair<int, long>>>& bigraph,
                              size_t num1,
                              size_t num2,
                              vector<pair<int, long>>& res,
                              long& cost) {
    assert(num1 == bigraph.size());
    int cid2vid = 2;
    int sid2vid = cid2vid + num1;
    int numVer = sid2vid + num2 + 2;

    Graph g;
    Capacity capacity = get(edge_capacity, g);
    Reversed rev = get(edge_reverse, g);
    ResidualCapacity residual_capacity = get(edge_residual_capacity, g);
    Weight weight = get(edge_weight, g);

    size_type N(numVer);
    for (size_type i = 0; i < N; ++i) {
        add_vertex(g);
    }

    vertex_descriptor s = 0;
    vertex_descriptor t = 1;

    EdgeAdder<Graph, Weight, Capacity, Reversed, ResidualCapacity> ea(g, weight, capacity, rev, residual_capacity);

    // source to cells
    for (int vi = cid2vid; vi < sid2vid; ++vi) ea.addEdge(0, vi, 0, 1);

    // cells to sites
    for (size_t cid = 0; cid < num1; ++cid) {
        for (auto& edge : bigraph[cid]) {
            ea.addEdge(cid + cid2vid, edge.first + sid2vid, edge.second, 1);
        }
    }

    // sites to destination
    for (int vi = sid2vid; vi < numVer; ++vi) ea.addEdge(vi, 1, 0, 1);

    successive_shortest_path_nonnegative_weights(g, s, t);
    cost = find_flow_cost(g);

    res.resize(num1, {-1, -1});
    for (int vi = cid2vid; vi < sid2vid; ++vi) {
        BGL_FORALL_OUTEDGES_T(vi, e, g, Graph) {
            if (get(capacity, e) > 0 && get(residual_capacity, e) == 0) {
                // cout <<  get(capacity, e) << " " << get(residual_capacity, e) << " " << get(weight, e) << endl;
                auto vi2 = target(e, g);
                int c = vi - cid2vid, s = vi2 - sid2vid;
                res[c].first = s;
                res[c].second = get(weight, e);
                // cout << c << " goes to " << s << endl;
            }
        }
    }

    // BGL_FORALL_EDGES_T(e, g, Graph) {
    //    if(get(capacity, e) > Cost(0)) {
    //     	cout    <<  "(" << source(e,g) << "," << target(e,g) << ") "
    //                 << get(capacity, e) << " " << get(residual_capacity, e)
    //                 << " " << get(weight, e) << endl;
    //    }
    // }
}

void LiteMinCostMaxFlow(const vector<vector<pair<int, int>>>& supplyToDemand,
                        const vector<int>& demandCap,
                        size_t num1,
                        size_t num2,
                        vector<pair<int, int>>& res,
                        int& cost) {
    unsigned nVirVertex = 2;
    unsigned nSupplyVertex = num1;
    unsigned nDemandVertex = num2;
    unsigned nVertex = nVirVertex + nSupplyVertex + nDemandVertex;

    Graph g;
    Capacity capacity = get(edge_capacity, g);
    Reversed rev = get(edge_reverse, g);
    ResidualCapacity residual_capacity = get(edge_residual_capacity, g);
    Weight weight = get(edge_weight, g);

    size_type N(nVertex);
    for (size_type i = 0; i < N; ++i) {
        add_vertex(g);
    }

    vertex_descriptor s = 0;
    vertex_descriptor t = 1;

    EdgeAdder<Graph, Weight, Capacity, Reversed, ResidualCapacity> ea(g, weight, capacity, rev, residual_capacity);

    // source to cells
    for (unsigned c = 0; c < nSupplyVertex; c++) ea.addEdge(0, c + nVirVertex, 0, 1);

    // cells to sites
    for (unsigned c = 0; c < nSupplyVertex; c++) {
        for (auto pair : supplyToDemand[c]) {
            int s = pair.first;
            int w = pair.second;
            ea.addEdge(c + nVirVertex, s + nVirVertex + nSupplyVertex, w, 1);
            // cout << c << " goes to " << s << endl;
        }
    }

    // sites to destination
    for (unsigned s = 0; s < nDemandVertex; s++) {
        if (demandCap[s] != 0) ea.addEdge(s + nVirVertex + nSupplyVertex, 1, 0, demandCap[s]);
    }

    successive_shortest_path_nonnegative_weights(g, s, t);
    cost = find_flow_cost(g);
    res.resize(nSupplyVertex, {-1, -1});

    for (unsigned c = 0; c < nSupplyVertex; ++c) {
        BGL_FORALL_OUTEDGES_T(c + nVirVertex, e, g, Graph) {
            if (get(capacity, e) > 0 && get(residual_capacity, e) == 0) {
                // cout <<  get(capacity, e) << " " << get(residual_capacity, e) << " " << get(weight, e) << endl;
                auto s = target(e, g) - nVirVertex - nSupplyVertex;
                res[c].first = s;
                res[c].second = get(weight, e);
                // cout << c << " goes to " << s << endl;
            }
        }
    }
}