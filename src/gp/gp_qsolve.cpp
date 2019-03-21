#include "../alg/Eigen/Eigen"

#include "gp_data.h"
#include "gp_qsolve.h"
#include "gp_region.h"

//#define MATRIX_LO_UP_MODE // for Eigen CG solver

double MinCellDist = 1;  // to avoid zero distance between two cells
int CGi_max = 500;       // maximum iteration times of CG

int nMovables = 0;
int nConnects = 0;

// Eigen (TODO: lo, hi)
class SpMatSolver {
private:
    // Ax=b, lo[i]<=x[i]<=hi[i]
    // Mat A, sparse, PSD
    vector<Eigen::Triplet<double>> A_nondig;  // A(i,j), Lower
    vector<double> A_dig;                     // A(i,i)
    Eigen::VectorXd b;                        // Vec b, dense
    Eigen::VectorXd x;                        // variables

public:
    int numIter;
    double err;

    inline double getLoc(int i) { return x[i]; }
    inline void setLoc(int i, double v) { x[i] = v; }
    inline const Eigen::VectorXd& getX() { return x; }

    void init(const vector<double>& cells) {
        b.resize(nMovables);
        assert(cells.size() >= (unsigned)nMovables);
        x.resize(nMovables);
        for (int i = 0; i < nMovables; ++i) x[i] = cells[i];
    }

    void finish(vector<double>& cells) {
        for (int i = 0; i < nMovables; ++i) cells[i] = x[i];
    }

    void reset() {
        A_nondig.clear();
        A_dig.assign(nMovables, 0);
        b.setZero();
    }

    void add_a_net(int i1,
                   int i2,  // cell index
                   double x1,
                   double x2,  // cell x/y coordinate
                   bool mov1,
                   bool mov2,  // movable or not
                   double w)   // weight
    {
        if (i1 == i2) return;
        if (mov1 && mov2) {
            A_dig[i1] += w;
            A_dig[i2] += w;
#ifdef MATRIX_LO_UP_MODE
            A_nondig.emplace_back(i1, i2, -w);
            A_nondig.emplace_back(i2, i1, -w);
#else
            if (i1 < i2)
                A_nondig.emplace_back(i1, i2, -w);
            else
                A_nondig.emplace_back(i2, i1, -w);
#endif
        } else if (mov1) {
            A_dig[i1] += w;
            b[i1] += w * x2;
        } else if (mov2) {
            A_dig[i2] += w;
            b[i2] += w * x1;
        }
    }

    void add_a_pseudonet(int i, double w) {
        A_dig[i] += w;
        b[i] += w * x[i];
    }

    void compute(int maxIter, double tolerance) {
        // load A
        for (int i = 0; i < nMovables; ++i)
            if (A_dig[i] != 0) A_nondig.emplace_back(i, i, A_dig[i]);
        Eigen::SparseMatrix<double> A;
        A.resize(nMovables, nMovables);
        A.setFromTriplets(A_nondig.begin(), A_nondig.end());

        // compute x (TODO: OpenMP)
#ifdef MATRIX_LO_UP_MODE
        Eigen::ConjugateGradient<Eigen::SparseMatrix<double>, Eigen::Lower | Eigen::Upper> cg;
#else
        Eigen::ConjugateGradient<Eigen::SparseMatrix<double>, Eigen::Upper> cg;
#endif
        cg.setMaxIterations(maxIter);
        cg.setTolerance(tolerance);
        cg.compute(A);
        x = cg.solveWithGuess(b, x);
        numIter = cg.iterations();
        err = cg.error();
        // cout << "#iterations:     " << cg.iterations() << endl;
        // cout << "estimated error: " << cg.error()      << endl;
    }
};

array<SpMatSolver, 2> solvers;  // solvers[0] for x, solvers[1] for y

void initCG() {
    nConnects = 2 * numPins + numCells;
    nMovables = numCells;

    solvers[0].init(cellX);
    solvers[1].init(cellY);
}

void B2BModelNet(int net, double weight = 1.0) {
    // find the net bounding box and the bounding cells
    int nPins = netCell[net].size();
    if (nPins < 2) return;
    double lx, hx, ly, hy;    // loc bounds
    int lxp, hxp, lyp, hyp;   // pin id for loc bounds
    int lxc, hxc, lyc, hyc;   // cell id for loc bounds
    bool lxm, hxm, lym, hym;  // movable for loc bounds
    lxp = hxp = lyp = hyp = 0;
    lxc = hxc = lyc = hyc = netCell[net][0];
    if (lxc < numCells) {
        lx = hx = solvers[0].getLoc(lxc);
        ly = hy = solvers[1].getLoc(lyc);
        lxm = hxm = lym = hym = true;
    } else {
        lx = hx = cellX[lxc];
        ly = hy = cellY[lyc];
        lxm = hxm = lym = hym = false;
    }
    for (int p = 1; p < nPins; p++) {
        int cell = netCell[net][p];
        double px, py;
        bool mov;
        if (cell < numCells) {
            px = solvers[0].getLoc(cell);
            py = solvers[1].getLoc(cell);
            mov = true;
        } else {
            px = cellX[cell];
            py = cellY[cell];
            mov = false;
        }
        if (px <= lx && cell != hxc) {
            lx = px;
            lxp = p;
            lxc = cell;
            lxm = mov;
        } else if (px >= hx && cell != lxc) {
            hx = px;
            hxp = p;
            hxc = cell;
            hxm = mov;
        }
        if (py <= ly && cell != hyc) {
            ly = py;
            lyp = p;
            lyc = cell;
            lym = mov;
        } else if (py >= hy && cell != lyc) {
            hy = py;
            hyp = p;
            hyc = cell;
            hym = mov;
        }
    }

    if (lxc == hxc || lyc == hyc) return;

    // enumerate B2B connections
    double w = 2.0 * weight / (double)(nPins - 1);
    solvers[0].add_a_net(lxc, hxc, lx, hx, lxm, hxm, w / max(MinCellDist, hx - lx));
    solvers[1].add_a_net(lyc, hyc, ly, hy, lym, hym, w / max(MinCellDist, hy - ly));
    for (int p = 0; p < nPins; p++) {
        int cell = netCell[net][p];
        int px, py;
        bool mov;
        if (cell < numCells) {
            px = solvers[0].getLoc(cell);
            py = solvers[1].getLoc(cell);
            mov = true;
        } else {
            px = cellX[cell];
            py = cellY[cell];
            mov = false;
        }
        if (p != lxp && p != hxp) {
            solvers[0].add_a_net(cell, lxc, px, lx, mov, lxm, w / max(MinCellDist, px - lx));
            solvers[0].add_a_net(cell, hxc, px, hx, mov, hxm, w / max(MinCellDist, hx - px));
        }
        if (p != lyp && p != hyp) {
            solvers[1].add_a_net(cell, lyc, py, ly, mov, lym, w / max(MinCellDist, py - ly));
            solvers[1].add_a_net(cell, hyc, py, hy, mov, hym, w / max(MinCellDist, hy - py));
        }
    }
}

void B2BModel() {
    for (int net = 0; net < numNets; net++) {
        int nPins = netCell[net].size();
        if (nPins < 2) {
            continue;
        }
        B2BModelNet(net, netWeight[net]);
    }
}

/*-------------------------------------
        for pseudonet in matrix C and vector b
-------------------------------------*/
void pseudonetAddInC_b(double alpha) {
    for (int i = 0; i < numCells; i++) {
        double weightX =
            gpSetting.pseudoNetWeightRatioX * alpha / max(MinCellDist, abs(lastVarX[i] - solvers[0].getLoc(i)));
        solvers[0].add_a_pseudonet(i, weightX);
        double weightY =
            gpSetting.pseudoNetWeightRatioY * alpha / max(MinCellDist, abs(lastVarY[i] - solvers[1].getLoc(i)));
        solvers[1].add_a_pseudonet(i, weightY);
    }
}

void lowerBound(double epsilon, double pseudoAlpha, int repeat, LBMode mode, double maxDisp) {
    initCG();

    for (int r = 0; r < repeat; r++) {
        for (auto& sol : solvers) sol.reset();

        B2BModel();

        if (pseudoAlpha > 0.0) {
            pseudonetAddInC_b(pseudoAlpha);
        }

        if (gpSetting.nThreads > 1) {
            thread t[2];
            auto thread_func = [&](int idx) { solvers[idx].compute(CGi_max, epsilon); };
            for (int i = 0; i < 2; ++i) t[i] = thread(thread_func, i);
            for (int i = 0; i < 2; ++i) t[i].join();
        } else {
            for (int i = 0; i < 2; ++i) solvers[i].compute(CGi_max, epsilon);
        }
        // double wlX, wlY;
        // hpwl(&wlX, &wlY);
        // solvers[0].finish(cellX);
        // solvers[1].finish(cellY);
        // printlog(LOG_INFO, "\tCG #%d: wl=%ld+%ld, it=%d/%d, err=%e/%e", r, (long)wlX, (long)wlY,
        //     solvers[0].numIter, solvers[1].numIter, solvers[0].err, solvers[1].err);
    }
    solvers[0].finish(cellX);
    solvers[1].finish(cellY);
    copy(cellX.begin(), cellX.begin() + numCells, lastVarX.begin());
    copy(cellY.begin(), cellY.begin() + numCells, lastVarY.begin());
}