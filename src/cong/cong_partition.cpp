#include "cong_partition.h"
#include "alg/patoh/patoh.h"
#include "gp/gp.h"

unsigned Cluster::numNonEmpty;
unsigned Cluster::numEmpty;

void Cluster::InitSeed() {
    assert(instIds.size() == 0);
    instIds.resize(database.instances.size());
    for (unsigned i = 0; i < instIds.size(); ++i) instIds[i] = i;
    numNonEmpty = 1;
    numEmpty = 0;
    id = 0;
}

void Cluster::Partition() {
    array<vector<int>, 2> res;
    cutSize = Bipartition(instIds, res);
    if (res[0].size() != 0) {
        children[0] = new Cluster;
        children[1] = new Cluster;
        children[0]->instIds = res[0];
        children[1]->instIds = res[1];
        children[0]->id = numNonEmpty + numEmpty;
        children[1]->id = numNonEmpty + numEmpty + 1;
        ++numNonEmpty;
        ++numEmpty;
        children[0]->Partition();
        children[1]->Partition();
    }
}

void Cluster::UpdateInfo(const vector<Point2<double>>& instXY) {
    assert(instXY.size() == database.instances.size());
    if (children[0] != NULL) {
        children[0]->UpdateInfo(instXY);
        children[1]->UpdateInfo(instXY);
        if (children[0]->avgY > children[1]->avgY) swap(children[0]->avgY, children[1]->avgY);
        avgY = (children[0]->avgY * children[0]->instIds.size() + children[1]->avgY * children[1]->instIds.size()) /
               (children[0]->instIds.size() + children[1]->instIds.size());
        area = children[0]->area + children[1]->area;
    } else {
        avgY = 0;
        area = 0;
        for (auto instId : instIds) {
            bbox.update(instXY[instId]);
            avgY += instXY[instId].y();
            auto type = database.instances[instId]->master->resource->name;
            if (type == Resource::LUT) {
                if (database.instances[instId]->master->name == Master::LUT6)
                    area += 2 * gpSetting.lutArea;
                else
                    area += gpSetting.lutArea;
            } else if (type == Resource::FF)
                area += gpSetting.ffArea;
            else if (type == Resource::DSP48E2)
                area += gpSetting.dspArea;
            else if (type == Resource::RAMB36E2)
                area += gpSetting.ramArea;
        }
        avgY /= instIds.size();
    }
}

unsigned Cluster::maxCutSize() {
    unsigned max = cutSize;
    if (children[0] != NULL && children[0]->cutSize > max) max = children[0]->cutSize;
    if (children[1] != NULL && children[1]->cutSize > max) max = children[1]->cutSize;
    return max;
}

void Cluster::PrintLeaf() const {
    assert(children[0] == NULL);
    printlog(LOG_INFO,
             "cluster%d (avgY: %lf, area: %lf, size: %u, x: %lf-%lf, y: %lf-%lf)",
             id,
             avgY,
             area,
             instIds.size(),
             bbox.lx(),
             bbox.ux(),
             bbox.ly(),
             bbox.uy());
}

void Cluster::Print() const {
    if (children[0] != NULL) {
        printlog(LOG_INFO,
                 "cluster%d (avgY: %lf, area: %lf) is divided into cluster%d and cluster%d",
                 id,
                 avgY,
                 area,
                 children[0]->id,
                 children[1]->id);
        children[0]->Print();
        children[1]->Print();
    }
    // else PrintLeaf();
}

void Cluster::GetResult(vector<Cluster*>& clusters) {
    if (children[0] == NULL)
        clusters.push_back(this);
    else {
        children[0]->GetResult(clusters);
        children[1]->GetResult(clusters);
    }
}

bool ReallocByCluster(vector<Group>& groups) {
    printlog(LOG_INFO, "**************");
    // get clusters according to netlist
    assert(groups.size() == database.instances.size());
    Cluster cluster;
    cluster.InitSeed();
    cluster.Partition();

    double netRate[2] = {0.03, 0.015};
    double as[2] = {1.7, 1.7};
    for (int i = 0; i < 2; ++i) {
        unsigned thres = database.nets.size() * netRate[i];
        printlog(LOG_INFO, "max cut: %d, threshold: %d", cluster.maxCutSize(), thres);
        if (cluster.maxCutSize() >= thres) {
            gpSetting.areaScale = as[i];
            printlog(LOG_INFO, "increase area scale to %f", gpSetting.areaScale);
            break;
        }
    }

    if (cluster.numNonEmpty == 1) {
        printlog(LOG_INFO, "no significant cluster exists, no reallocation occurs");
        printlog(LOG_INFO, "**************");
        return false;
    }

    // update info
    vector<Point2<double>> instXY(database.instances.size());
    for (unsigned i = 0; i < instXY.size(); ++i) {
        instXY[i].x() = groups[i].x;
        instXY[i].y() = groups[i].y;
    }
    cluster.UpdateInfo(instXY);
    printlog(LOG_INFO, "INTERNAL NODES:");
    cluster.Print();

    // from tree to array
    vector<Cluster*> clusters;
    cluster.GetResult(clusters);
    printlog(LOG_INFO, "SORTED LEAVES:");
    for (auto c : clusters) c->PrintLeaf();

        // export locs
#ifdef WRITE
    ofstream os;
    os.open("before_part.txt");
    for (unsigned i = 0; i < clusters.size(); ++i) {
        for (auto id : clusters[i]->instIds) os << groups[id].y << ' ' << groups[id].x << ' ' << i << endl;
    }
    os.close();
#endif

    // divide by area and distribute uniformally
    printlog(LOG_INFO, "cluster area: %d", cluster.area);
    double nx = database.sitemap_nx, ny = database.sitemap_ny;
    const double areaScale = gpSetting.areaScale * 1.2;               // 1.2?
    double ratio = min(1.0, (nx * ny) / (cluster.area * areaScale));  // dy / area
    double dyAll = cluster.area * areaScale * ratio / nx;
    double lyAll = max(0.0, cluster.avgY - dyAll / 2.0);
    double ly = lyAll;
    for (auto c : clusters) {
        double dy = min(c->area * areaScale * ratio / nx, ny - ly - 2);  // 2 is for safety
        // x
        vector<pair<double, int>> sortInstByX;  // (x, id)
        for (auto inst : c->instIds) sortInstByX.push_back({groups[inst].x, inst});
        sort(sortInstByX.begin(), sortInstByX.end());
        for (unsigned i = 0; i < sortInstByX.size(); ++i)
            groups[sortInstByX[i].second].x = double(i) / sortInstByX.size() * nx;
        // y
        vector<pair<double, int>> sortInstByY;  // (y, id)
        for (auto inst : c->instIds) sortInstByY.push_back({groups[inst].y, inst});
        sort(sortInstByY.begin(), sortInstByY.end());
        for (unsigned i = 0; i < sortInstByY.size(); ++i)
            groups[sortInstByY[i].second].y = double(i) / sortInstByY.size() * dy + ly;
        ly += dy;
    }
#ifdef WRITE
    // export locs
    os.open("after_part.txt");
    for (unsigned i = 0; i < clusters.size(); ++i) {
        for (auto id : clusters[i]->instIds) os << groups[id].y << ' ' << groups[id].x << ' ' << i << endl;
    }
    os.close();
#endif

    printlog(LOG_INFO, "**************");
    return true;
}

void PartitionPlFile() {
    vector<vector<int>> clusters(1, vector<int>(database.instances.size()));
    for (unsigned i = 0; i < clusters[0].size(); ++i) clusters[0][i] = i;

    for (unsigned i = 0; i < clusters.size();) {
        array<vector<int>, 2> bipart;
        Bipartition(clusters[i], bipart);
        if (bipart[0].size() != 0) {
            clusters[i] = bipart[0];
            clusters.push_back(bipart[1]);
        } else
            ++i;
    }

    for (unsigned i = 0; i < clusters.size(); ++i) {
        ofstream os;
        os.open("part" + to_string(i) + ".txt");
        for (auto id : clusters[i]) {
            Site* site = database.instances[id]->pack->site;
            os << site->y << ' ' << site->x << endl;
        }
        os.close();
    }
}

unsigned Bipartition(const vector<int>& inputCluster, array<vector<int>, 2>& outputClusters) {
    // Constraints
    unsigned minClusterSize = max(unsigned(database.instances.size() * 0.25), 10000u);
    double maxCutRate = 0.05;
    if (inputCluster.size() < minClusterSize * 2) return 0;

    // Preprocessing
    PaToH_Parameters args;
    // cell/instance
    int c = 0;  // num of cells
    vector<int> inst2cell(database.instances.size(), -1);
    for (auto instId : inputCluster) inst2cell[instId] = c++;
    int* cwghts = new int[c];
    for (int i = 0; i < c; ++i) cwghts[i] = 1;
    // net/pin
    int n = 0;  // num of nets
    vector<bool> isCsdNet(database.nets.size(), false);
    int p = 0;  // num of pins
    for (auto net : database.nets) {
        bool exist = false;
        for (auto pin : net->pins)
            if (inst2cell[pin->instance->id] != -1) {
                ++p;
                exist = true;
            }
        if (exist) {
            ++n;
            isCsdNet[net->id] = true;
        }
    }
    int maxCut = maxCutRate * n;
    // net to cell
    int* xpins = new int[n + 1];
    xpins[0] = 0;
    int* pins = new int[p];
    int in = 0, ip = 0;
    for (auto net : database.nets)
        if (isCsdNet[net->id]) {
            for (auto pin : net->pins)
                if (inst2cell[pin->instance->id] != -1) pins[ip++] = inst2cell[pin->instance->id];
            xpins[++in] = ip;
        }
    assert(ip == p && in == n);

    // Init
    PaToH_Initialize_Parameters(&args, PATOH_CONPART, PATOH_SUGPARAM_QUALITY);
    args.seed = 0;
    args._k = 2;
    args.final_imbal = max(0.5 - double(minClusterSize) / c, 0.2);  // (c - minClusterSize) / c - 0.5
    int* partvec = new int[c];
    int* partweights = new int[args._k];
    int cut;
    PaToH_Check_User_Parameters(&args, true);
    PaToH_Alloc(&args, c, n, 1, cwghts, NULL, xpins, pins);
    static int cnt = 0;

    // Processing
    PaToH_Part(&args, c, n, 1, 0, cwghts, NULL, xpins, pins, NULL, partvec, partweights, &cut);

    // Postprocessing
    for (int i = 0; i < c; ++i) outputClusters[partvec[i]].push_back(inputCluster[i]);
    unsigned minCluster = min(outputClusters[0].size(), outputClusters[1].size());
    double imbal = 0.5 - double(minCluster) / c;
    if (cut > maxCut) {
        printlog(LOG_INFO,
                 "%d: n: %d, c: %d, max cut: %d, max imbal: %lf, get too large cut (%d)",
                 cnt,
                 n,
                 c,
                 maxCut,
                 args.final_imbal,
                 cut);
        outputClusters[0].clear();
        outputClusters[1].clear();
    } else {
        printlog(LOG_INFO,
                 "%d: n: %d, c: %d, max cut: %d, max imbal: %lf, succeed with %d and %d (cut: %d, imbal: %lf)",
                 cnt,
                 n,
                 c,
                 maxCut,
                 args.final_imbal,
                 partweights[0],
                 partweights[1],
                 cut,
                 imbal);
    }
    ++cnt;

    delete[] cwghts;
    delete[] xpins;
    delete[] pins;
    delete[] partweights;
    delete[] partvec;
    PaToH_Free();

    return cut;
}
