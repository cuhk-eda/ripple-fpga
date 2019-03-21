#include "db.h"

using namespace db;

Database db::database;

Database::Database() {
    sitemap_nx = 0;
    sitemap_ny = 0;
    crmap_nx = 0;
    crmap_ny = 0;

    canvas = NULL;
}

Database::~Database() {
    for (auto m : masters) delete m;
    for (auto i : instances) delete i;
    for (auto n : nets) delete n;
    for (auto s : sitetypes) delete s;
    for (auto r : resources) delete r;
    for (auto p : packs) delete p;
    for (auto swv : switchboxes)
        for (auto sw : swv) delete sw;
    for (unsigned int i = 0; i < sites.size(); ++i)
        for (unsigned int j = 0; j < sites[i].size(); ++j)
            if (j == 0 || sites[i][j] != sites[i][j - 1]) delete sites[i][j];
    if (canvas) delete canvas;
}

void Database::setup() {
    for (int x = 0; x < sitemap_nx; x++) {
        for (int y = 1; y < sitemap_ny; y++) {
            if (sites[x][y] == NULL && sites[x][y - 1] != NULL) {
                sites[x][y] = sites[x][y - 1];
                sites[x][y]->h++;
            }
        }
    }

    tdmap_nx = sitemap_nx;
    tdmap_ny = sitemap_ny;
    targetDensity.resize(tdmap_nx, vector<double>(tdmap_ny, 1.0));

    for (int x = 0; x < crmap_nx; x++) {
        for (int y = 0; y < crmap_ny; y++) {
            clkrgns[x][y]->Init();
        }
    }

    /*
    //remove clock net
    bool clkFound = false;
    for(int i=0; i<(int)nets.size(); i++){
        int nPins = (int)nets[i]->pins.size();
        for(int p=0; p<nPins; p++){
            if(nets[i]->pins[p]->type->type == 'c'){
                clkFound = true;
                break;
            }
        }
        if(clkFound){
            printlog(LOG_INFO, "clock net found: %s with %d pins", nets[i]->name.c_str(), (int)nets[i]->pins.size());
            delete nets[i];
            nets.erase(nets.begin()+i);
            break;
            //i--;
            //clkFound = false;
        }
    }
    */

    for (unsigned i = 0; i < instances.size(); ++i) instances[i]->id = i;
    for (unsigned i = 0; i < sitetypes.size(); ++i) sitetypes[i]->id = i;
    for (unsigned i = 0; i < nets.size(); ++i) nets[i]->id = i;

    setPinMap();
    setSupplyAll();
    setCellPinIndex();
}

void Database::print() {
    printlog(LOG_INFO, "#instances:  %d", (int)instances.size());
    printlog(LOG_INFO, "#nets:       %d", (int)nets.size());
    printlog(LOG_INFO, "#sites:      %d x %d", sitemap_nx, sitemap_ny);
    printlog(LOG_INFO, "#resources:  %d", (int)resources.size());
    printlog(LOG_INFO, "#sitetypes:  %d", (int)sitetypes.size());
}

Master *Database::addMaster(const Master &master) {
    if (getMaster(master.name) != NULL) {
        cerr << "Master::Name: " << master.name << " duplicated" << endl;
        return NULL;
    }
    Master *newmaster = new Master(master);
    name_masters[master.name] = newmaster;
    masters.push_back(newmaster);
    return newmaster;
}
Instance *Database::addInstance(const Instance &instance) {
    if (getInstance(instance.name) != NULL) {
        cerr << "Instance: " << instance.name << " duplicated" << endl;
        return NULL;
    }
    Instance *newinstance = new Instance(instance);
    name_instances[instance.name] = newinstance;
    instances.push_back(newinstance);
    return newinstance;
}
Net *Database::addNet(const Net &net) {
    if (getNet(net.name) != NULL) {
        cerr << "Net: " << net.name << " duplicated" << endl;
        return NULL;
    }
    Net *newnet = new Net(net);
    name_nets[net.name] = newnet;
    nets.push_back(newnet);
    return newnet;
}
Resource *Database::addResource(const Resource &resource) {
    if (getResource(resource.name) != NULL) {
        cerr << "Resource: " << resource.name << " duplicated" << endl;
        return NULL;
    }
    Resource *newresource = new Resource(resource);
    name_resources[resource.name] = newresource;
    resources.push_back(newresource);
    return newresource;
}
SiteType *Database::addSiteType(const SiteType &sitetype) {
    if (getSiteType(sitetype.name) != NULL) {
        cerr << "SiteType: " << sitetype.name << " duplicated" << endl;
        return NULL;
    }
    SiteType *newsitetype = new SiteType(sitetype);
    name_sitetypes[sitetype.name] = newsitetype;
    sitetypes.push_back(newsitetype);
    return newsitetype;
}
Site *Database::addSite(int x, int y, SiteType *sitetype) {
    if (x < 0 || x >= sitemap_nx || y < 0 || y >= sitemap_ny) {
        printlog(LOG_ERROR, "Invalid position (%d,%d) (w=%d h=%d)", x, y, sitemap_nx, sitemap_ny);
        return NULL;
    }
    if (sites[x][y] != NULL) {
        // site can be repeatly defined, simply ignore it
        printlog(LOG_ERROR, "Site already defined at %d,%d", x, y);
        return NULL;
    }
    Site *newsite = new Site(x, y, sitetype);
    sites[x][y] = newsite;
    return newsite;
}

Pack *Database::addPack(SiteType *sitetype) {
    Pack *newpack = new Pack(sitetype);
    packs.push_back(newpack);
    return newpack;
}

SwitchBox *Database::addSwitchBox(int x, int y) {
    if (x < 0 || x >= switchbox_nx || y < 0 || y >= switchbox_ny) {
        printlog(LOG_ERROR, "Invalid position (%d,%d) (w=%d h=%d)", x, y, switchbox_nx, switchbox_ny);
        return NULL;
    }
    if (switchboxes[x][y] != NULL) {
        printlog(LOG_ERROR, "Switch box already defined at %d,%d", x, y);
        return NULL;
    }
    SwitchBox *newswitchbox = new SwitchBox(x, y);
    switchboxes[x][y] = newswitchbox;
    return newswitchbox;
}

Master *Database::getMaster(Master::Name name) {
    auto mi = name_masters.find(name);
    if (mi == name_masters.end()) return NULL;
    return mi->second;
}
Instance *Database::getInstance(const string &name) {
    auto mi = name_instances.find(name);
    if (mi == name_instances.end()) {
        return NULL;
    }
    return mi->second;
}
Net *Database::getNet(const string &name) {
    auto mi = name_nets.find(name);
    if (mi == name_nets.end()) {
        return NULL;
    }
    return mi->second;
}
Resource *Database::getResource(Resource::Name name) {
    auto mi = name_resources.find(name);
    if (mi == name_resources.end()) return NULL;
    return mi->second;
}
SiteType *Database::getSiteType(SiteType::Name name) {
    auto mi = name_sitetypes.find(name);
    if (mi == name_sitetypes.end()) return NULL;
    return mi->second;
}
Site *Database::getSite(int x, int y) {
    if (x < 0 || x >= sitemap_nx || y < 0 || y >= sitemap_ny) {
        return NULL;
    }
    return sites[x][y];
}

SwitchBox *Database::getSwitchBox(int x, int y) {
    if (x < 0 || x >= switchbox_nx || y < 0 || y >= switchbox_ny) {
        return NULL;
    }
    return switchboxes[x][y];
}

void Database::setSiteMap(int nx, int ny) {
    sitemap_nx = nx;
    sitemap_ny = ny;
    sites.resize(nx, vector<Site *>(ny, NULL));
}

void Database::setSwitchBoxes(int nx, int ny) {
    switchbox_nx = nx;
    switchbox_ny = ny;
    switchboxes.resize(nx, vector<SwitchBox *>(ny, NULL));

    for (int x = 0; x < nx; x++) {
        for (int y = 0; y < ny; y++) {
            switchboxes[x][y] = new SwitchBox(x, y);
        }
    }
}

bool Database::place(Instance *instance, int x, int y, int slot) {
    Site *site = getSite(x, y);
    return place(instance, site, slot);
}

bool Database::place(Instance *instance, Site *site, int slot) {
    if (site == NULL) {
        return false;
    }
    if (site->pack == NULL) {
        site->pack = addPack(site->type);
        site->pack->site = site;
    }
    if (slot < 0 || slot >= (int)site->pack->instances.size()) {
        return false;
    }
    if (site->pack->instances[slot] != NULL) {
        return false;
    }
    if (site->type->resources[slot] != instance->master->resource) {
        return false;
    }
    site->pack->instances[slot] = instance;
    instance->pack = site->pack;
    instance->slot = slot;
    return true;
}

bool Database::unplace(Instance *instance) {
    if (instance->pack == NULL) {
        return false;
    }
    instance->pack->instances[instance->slot] = NULL;
    instance->pack = NULL;
    instance->slot = -1;
    return true;
}

bool Database::unplaceAll() {
    for (int i = 0; i < (int)instances.size(); i++) {
        if (!instances[i]->fixed) unplace(instances[i]);
    }
    return true;
}

bool Database::place(Pack *pack, int x, int y) {
    Site *site = getSite(x, y);
    if (site == NULL) {
        return false;
    }
    if (site->type != pack->type) {
        return false;
    }
    if (site->pack != NULL) {
        return false;
    }
    if (pack->site != NULL) {
        unplace(pack);
    }
    site->pack = pack;
    pack->site = site;
    return true;
}

bool Database::place(Pack *pack, Site *site) {
    if (site == NULL) {
        return false;
    }
    if (site->type != pack->type) {
        return false;
    }
    if (site->pack != NULL) {
        return false;
    }
    if (pack->site != NULL) {
        unplace(pack);
    }
    site->pack = pack;
    pack->site = site;
    return true;
}

bool Database::unplace(Pack *pack) {
    if (pack->site == NULL) {
        return false;
    }
    pack->site->pack = NULL;
    pack->site = NULL;
    return true;
}

bool Database::isPackValid(Pack *pack) {
    if (pack->type->name == SiteType::SLICE) {
        /***** LUT Rule *****/
        for (int i = 0; i < 8; i++) {
            Instance *a = pack->instances[2 * i];
            Instance *b = pack->instances[2 * i + 1];
            if (a == NULL || b == NULL) continue;
            if (a->master->name == Master::LUT6) return false;
            if (!IsLUTCompatible(*a, *b)) return false;
        }
        /***** FF Rule *****/
        int ceset[4][4] = {{16, 18, 20, 22}, {17, 19, 21, 23}, {24, 26, 28, 30}, {25, 27, 29, 31}};
        int ckset[2][8] = {{16, 17, 18, 19, 20, 21, 22, 23}, {24, 25, 26, 27, 28, 29, 30, 31}};  // srset is the same
        for (int i = 0; i < 4; i++) {
            Net *CENet = NULL;
            for (int j = 0; j < 4; j++) {
                auto *ins = pack->instances[ceset[i][j]];
                if (ins != NULL) {
                    if (CENet == NULL)
                        CENet = ins->pins[cePinIdx]->net;
                    else if (ins->pins[cePinIdx]->net != CENet)
                        return false;
                }
            }
        }
        for (int i = 0; i < 2; i++) {
            Net *ClkNet = NULL;
            Net *SRNet = NULL;
            for (int j = 0; j < 8; j++) {
                auto ins = pack->instances[ckset[i][j]];
                if (ins != NULL) {
                    // clk
                    if (ClkNet == NULL)
                        ClkNet = ins->pins[clkPinIdx]->net;
                    else if (ins->pins[clkPinIdx]->net != ClkNet)
                        return false;
                    // sr
                    if (SRNet == NULL)
                        SRNet = ins->pins[srPinIdx]->net;
                    else if (ins->pins[srPinIdx]->net != SRNet)
                        return false;
                }
            }
        }
    }
    return true;
}

double Database::getHPWL(bool printXY) { return getSwitchBoxHPWL(printXY); }

double Database::getSiteHPWL() {
    // pre-compute locs to speed up
    vector<Point2<double>> locs;
    locs.reserve(instances.size());
    for (auto inst : instances) {
        auto site = inst->pack->site;
        locs.emplace_back(site->cx(), site->cy());
        // locs.emplace_back(site->cx(), inst->IsIO() ? getIOY(inst) : site->cy());
    }
    double totalHPWL = 0.0;
    for (auto net : nets) {
        // first pin
        Box<double> box(locs[net->pins[0]->instance->id]);
        // other pins
        for (size_t i = 1; i < net->pins.size(); ++i) {
            box.fupdate(locs[net->pins[i]->instance->id]);
        }
        totalHPWL += box.hp();
    }
    return totalHPWL;
}

double Database::getSwitchBoxHPWL(bool printXY) {
    // pre-compute locs to speed up
    vector<Point2<double>> locs;
    locs.reserve(instances.size());
    for (auto inst : instances) {
        auto site = inst->pack->site;
        locs.emplace_back(site->cx(), site->cy());
        // locs.emplace_back(site->cx(), inst->IsIO() ? getIOY(inst) : site->cy());
    }
    // iterate all nets
    double totalHPWLX = 0.0;
    double totalHPWLY = 0.0;
    for (auto net : nets) {
        // first pin
        Box<double> box(locs[net->pins[0]->instance->id]);
        // other pins
        for (size_t i = 1; i < net->pins.size(); ++i) {
            box.fupdate(locs[net->pins[i]->instance->id]);
        }
        totalHPWLX += box.x();
        totalHPWLY += box.y();
    }

    if (printXY) printlog(LOG_INFO, "xyhpwl: %.1lf %.1lf", totalHPWLX / 2, totalHPWLY);
    return totalHPWLX / 2 + totalHPWLY;
}

int Database::getRouteNet() {
    int totalRouteNet = 0;
    for (auto net : nets) {
        Site *exist = nullptr;
        bool needRoute = false;
        for (auto pin : net->pins) {
            Site *site = pin->instance->pack->site;
            if (exist == nullptr)
                exist = site;
            else if (site != exist) {
                needRoute = true;
                break;
            }
        }
        if (needRoute) ++totalRouteNet;
    }
    return totalRouteNet;
}

int Database::getTotNumDupInputs() {
    int tot = 0;
    for (auto &ss : sites)
        for (auto s : ss)
            if (s) {
                auto pack = s->pack;
                if (pack && pack->type->name == SiteType::SLICE) {
                    for (int i = 0; i < 8; i++) {
                        Instance *a = pack->instances[2 * i];
                        Instance *b = pack->instances[2 * i + 1];
                        if (a == NULL || b == NULL) continue;
                        tot += NumDupInputs(*a, *b);
                    }
                }
            }
    return tot;
}

bool Database::isPlacementValid() {
    for (int i = 0; i < (int)sites.size(); i++) {
        for (int j = 0; j < (int)sites[i].size(); j++) {
            if (sites[i][j]->pack != NULL && sites[i][j]->pack->site != sites[i][j]) {
                printlog(LOG_ERROR, "error in site->pack->site consistency validation");
                return false;
            }
        }
    }
    for (int i = 0; i < (int)packs.size(); i++) {
        if (packs[i]->site->pack != packs[i]) {
            printlog(LOG_ERROR, "error in pack->site->pack consistency validation");
            return false;
        }
    }
    for (int i = 0; i < (int)instances.size(); i++) {
        // cout<<instances[i]->id<<endl;
        if (instances[i]->pack->instances[instances[i]->slot] != instances[i]) {
            printlog(LOG_ERROR, "error in instance->pack->instance consistency validation");
            return false;
        }
    }
    for (int i = 0; i < (int)packs.size(); i++) {
        for (int j = 0; j < (int)packs[i]->instances.size(); j++) {
            if (packs[i]->instances[j] != NULL && packs[i]->instances[j]->pack != packs[i]) {
                printlog(LOG_ERROR, "error in pack->instance->pack consistency validation");
                return false;
            }
        }
    }
    for (int i = 0; i < (int)sites.size(); i++) {
        for (int j = 0; j < (int)sites[i].size(); j++) {
            if (sites[i][j]->pack != NULL && !isPackValid(sites[i][j]->pack)) {
                printlog(LOG_ERROR, "error in site legality validation");
                return false;
            }
        }
    }
    for (int i = 0; i < (int)instances.size(); i++) {
        if (!isPackValid(instances[i]->pack)) {
            printlog(LOG_ERROR, "error in instance legality validation");
            return false;
        }
    }

    if (crmap_nx == 0) return true;

    int nOFColRgn = 0, nOFCr = 0;
    bool isClkLeg = true;

    // check colrgn
    for (int x = 0; x < crmap_nx; x++) {
        for (int y = 0; y < crmap_ny; y++) {
            // clear prev clknet info
            ClkRgn *clkrgn = clkrgns[x][y];
            clkrgn->clknets.clear();
            for (int colx = 0; colx < (clkrgn->hx - clkrgn->lx); colx++) {
                for (int coly = 0; coly < 2; coly++) {
                    clkrgn->hfcols[colx][coly]->clknets.clear();
                }
            }

            // add new clknet info
            for (int coly = 0; coly < 2; coly++) {
                int colrgnCnt = (clkrgn->hx - clkrgn->lx) & 1;
                unordered_set<Net *> clknets;
                for (int colx = 0; colx < (clkrgn->hx - clkrgn->lx); colx++) {
                    colrgnCnt++;
                    for (auto site : clkrgn->hfcols[colx][coly]->sites) {
                        if (site->pack == NULL) continue;
                        for (auto inst : site->pack->instances) {
                            if (inst == NULL) continue;

                            if (inst->master->name == Master::FDRE || inst->master->name == Master::DSP48E2 ||
                                inst->master->name == Master::RAMB36E2) {
                                for (auto pin : inst->pins) {
                                    Net *net = pin->net;
                                    if (net != NULL && net->isClk) {
                                        clknets.insert(net);
                                        clkrgn->hfcols[colx][coly]->clknets.insert(net);
                                    }
                                }
                            }
                        }
                    }
                    if (colrgnCnt == 2) {
                        colrgnCnt = 0;
                        if (clknets.size() > COLRGN_NCLK) {
                            printlog(LOG_ERROR,
                                     "error in colrgn (%3d,%3d) legality validation, OF = %lu(%lu)",
                                     clkrgn->hfcols[colx][coly]->x - 1,
                                     clkrgn->hfcols[colx][coly]->ly,
                                     clknets.size(),
                                     COLRGN_NCLK);
                            nOFColRgn++;
                            isClkLeg = false;
                        }
                        clknets.clear();
                    }
                }
            }
        }
    }

    // check clkrgn
    for (auto net : nets) {
        if (net->isClk) {
            double lx = DBL_MAX, hx = DBL_MIN, ly = DBL_MAX, hy = DBL_MIN;
            for (auto pin : net->pins) {
                if (pin->type->type != 'o' && pin->instance != NULL) {
                    double x = pin->instance->pack->site->cx();
                    double y = pin->instance->pack->site->cy();
                    lx = min(x, lx);
                    ly = min(y, ly);
                    hx = max(x, hx);
                    hy = max(y, hy);
                }
            }

            for (int x = hfcols[lx][ly / 30]->clkrgn->cr_x; x <= hfcols[hx][hy / 30]->clkrgn->cr_x; x++) {
                for (int y = hfcols[lx][ly / 30]->clkrgn->cr_y; y <= hfcols[hx][hy / 30]->clkrgn->cr_y; y++) {
                    clkrgns[x][y]->clknets.insert(net);
                }
            }
        }
    }
    for (int x = 0; x < crmap_nx; x++) {
        for (int y = 0; y < crmap_ny; y++) {
            if (clkrgns[x][y]->clknets.size() > CLKRGN_NCLK) {
                printlog(LOG_ERROR,
                         "error in clkrgn_X%d_Y%d (%d, %d, %d, %d) legality validation, OF = %lu(%lu)",
                         x,
                         y,
                         clkrgns[x][y]->lx,
                         clkrgns[x][y]->ly,
                         clkrgns[x][y]->hx,
                         clkrgns[x][y]->hy,
                         clkrgns[x][y]->clknets.size(),
                         CLKRGN_NCLK);
                nOFCr++;
                isClkLeg = false;
            }
        }
    }

    if (!isClkLeg) {
        printlog(LOG_INFO, "%d %d", nOFColRgn, nOFCr);
        return false;
    }

    return true;
}

bool Database::setDemand(int x1, int y1, int x2, int y2, int demand) {
    SwitchBox *a = switchboxes[x1][y1];
    SwitchBox *b = switchboxes[x2][y2];
    return setDemand(a, b, demand);
}
bool Database::setDemand(SwitchBox *a, SwitchBox *b, int demand) {
    int ai, bi;
    int dx = b->x - a->x;
    int dy = b->y - a->y;
    int f = -1;
    int g = -1;
    int d = 0;
    if (dx == 0) {
        if (dy > 0) {
            f = 0;
            g = 2;
            d = dy;
        }
        if (dy < 0) {
            f = 2;
            g = 0;
            d = -dy;
        }
    } else if (dy == 0) {
        if (dx > 0) {
            f = 1;
            g = 3;
            d = dx;
        }
        if (dx < 0) {
            f = 3;
            g = 1;
            d = -dx;
        }
    }
    if (f < 0) {
        return false;
    }
    bi = 4 * (d - 1) + f;
    ai = 4 * (d - 1) + g;
    a->demand[bi] = demand;
    b->demand[ai] = demand;
    return true;
}
bool Database::setSupply(SwitchBox *a, SwitchBox *b, int supply) {
    int bi;
    int dx = b->x - a->x;
    int dy = b->y - a->y;
    int f = -1;
    int d = 0;
    if (dx == 0) {
        if (dy > 0) {
            f = 0;
            d = dy;
        }
        if (dy < 0) {
            f = 2;
            d = -dy;
        }
    } else if (dy == 0) {
        if (dx > 0) {
            f = 1;
            d = dx;
        }
        if (dx < 0) {
            f = 3;
            d = -dx;
        }
    }
    if (f < 0) {
        printlog(LOG_ERROR, "invalid");
        return false;
    }
    bi = 4 * (d - 1) + f;
    a->supply[bi] = supply;
    return true;
}
bool Database::setSupply(int x1, int y1, int x2, int y2, int supply) {
    SwitchBox *a = switchboxes[x1][y1];
    SwitchBox *b = switchboxes[x2][y2];
    return setSupply(a, b, supply);
}
void Database::setSupplyAll() {
    for (int x = 0; x < switchbox_nx; x++) {
        for (int y = 0; y < switchbox_ny; y++) {
            if (y < switchbox_ny - 1) setSupply(switchboxes[x][y], switchboxes[x][y + 1], 21);
            if (y < switchbox_ny - 2) setSupply(switchboxes[x][y], switchboxes[x][y + 2], 14);
            if (y < switchbox_ny - 3) setSupply(switchboxes[x][y], switchboxes[x][y + 3], 2);
            if (y < switchbox_ny - 4) setSupply(switchboxes[x][y], switchboxes[x][y + 4], 8);
            if (y < switchbox_ny - 5) setSupply(switchboxes[x][y], switchboxes[x][y + 5], 7);
            if (y < switchbox_ny - 6) setSupply(switchboxes[x][y], switchboxes[x][y + 6], 1);

            if (y >= 1) setSupply(switchboxes[x][y], switchboxes[x][y - 1], 20);
            if (y >= 2) setSupply(switchboxes[x][y], switchboxes[x][y - 2], 18);
            if (y >= 4) setSupply(switchboxes[x][y], switchboxes[x][y - 4], 8);
            if (y >= 5) setSupply(switchboxes[x][y], switchboxes[x][y - 5], 8);

            if (x < switchbox_nx - 1) setSupply(switchboxes[x][y], switchboxes[x + 1][y], 24);
            if (x < switchbox_nx - 2) setSupply(switchboxes[x][y], switchboxes[x + 2][y], 16);
            if (x < switchbox_nx - 6) setSupply(switchboxes[x][y], switchboxes[x + 6][y], 8);

            if (x >= 1) setSupply(switchboxes[x][y], switchboxes[x - 1][y], 24);
            if (x >= 2) setSupply(switchboxes[x][y], switchboxes[x - 2][y], 16);
            if (x >= 6) setSupply(switchboxes[x][y], switchboxes[x - 6][y], 8);
        }
    }
}

Database::PinRule Database::getPinRule(Instance *inst) {
    auto site = inst->pack->site;
    auto sitetype = site->type->name;
    if (sitetype == SiteType::SLICE) {
        return PinRuleSlice;
    } else if (sitetype == SiteType::DSP) {
        return (site->y % 5 == 0) ? PinRuleDSP0 : PinRuleDSP1;
    } else if (sitetype == SiteType::BRAM) {
        return PinRuleBRAM;
    } else if (sitetype == SiteType::IO) {
        auto type = inst->master->name;
        if (site->h > 30) {
            return PinRuleNone;
        } else if (site->x == 103 && (site->y == 0 || site->y == 30)) {
            if (type == Master::BUFGCE || type == Master::IBUF) {
                return PinRuleInputS;
            } else if (type == Master::OBUF) {
                return PinRuleOutputS;
            }
        } else {
            if (type == Master::BUFGCE || type == Master::IBUF) {
                return PinRuleInput;
            } else if (type == Master::OBUF) {
                return PinRuleOutput;
            }
        }
    }
    printlog(LOG_ERROR, "Pin rule not known.");
    return PinRuleNone;
}
int Database::getIOY(Instance *inst) {
    assert(inst->IsIO() && inst->pack && inst->pack->site);
    auto site = inst->pack->site;
    auto rule = getPinRule(inst);
    if (rule == PinRuleInput || rule == PinRuleOutput || rule == PinRuleInputS || rule == PinRuleOutputS) {
        return site->y + 0.5 + pinSwitchBoxMap[rule][inst->slot];
    } else /* rule == PinRuleNone) */ {
        return site->cy();
    }
}
void Database::setSwitchBoxPins(Instance *inst) {
    auto site = inst->pack->site;
    int sbx, sby;
    int x = site->x, y = site->y;
    auto rule = getPinRule(inst);

    sbx = SWCol(x);
    sby = y;
    if (rule == PinRuleSlice) {
        vector<Pin *> &sbPins = switchboxes[sbx][sby]->pins;
        sbPins.insert(sbPins.end(), inst->pins.begin(), inst->pins.end());
    } else if (rule == PinRuleDSP0 || rule == PinRuleDSP1 || rule == PinRuleBRAM) {
        if (rule == PinRuleDSP1) sby = y - 2;
        for (int i = 0; i < (int)inst->pins.size(); i++) {
            Pin *pin = inst->pins[i];
            int offset = pinSwitchBoxMap[rule][i];
            if (offset >= 0) {
                switchboxes[sbx][sby + offset]->pins.push_back(pin);
            }
        }
    } else if (rule == PinRuleInput || rule == PinRuleOutput || rule == PinRuleInputS || rule == PinRuleOutputS) {
        int offset = pinSwitchBoxMap[rule][inst->slot];
        vector<Pin *> &sbPins = switchboxes[sbx][sby + offset]->pins;
        sbPins.insert(sbPins.end(), inst->pins.begin(), inst->pins.end());
    } else if (rule == PinRuleNone) {
        sby = y + site->h / 2;
        vector<Pin *> &sbPins = switchboxes[sbx][sby]->pins;
        sbPins.insert(sbPins.end(), inst->pins.begin(), inst->pins.end());
    } else {
        printlog(LOG_ERROR, "pin mapping rule not handled");
    }
}
void Database::setSwitchBoxPins() {
    for (int x = 0; x < switchbox_nx; x++) {
        for (int y = 0; y < switchbox_ny; y++) {
            switchboxes[x][y]->pins.clear();
        }
    }
    Site *prevSite = NULL;
    for (int x = 0; x < sitemap_nx; x++) {
        for (int y = 0; y < sitemap_ny; y++) {
            Site *site = sites[x][y];
            if (site->pack == NULL || site == prevSite) continue;
            for (auto inst : site->pack->instances) {
                if (inst != NULL) setSwitchBoxPins(inst);
            }
            prevSite = site;
        }
    }
}

void Database::setCellPinIndex() {
    Master *FF = getMaster(Master::FDRE);
    if (FF == NULL) {
        printlog(LOG_ERROR, "FF not found");
    } else {
        for (unsigned int i = 0; i < FF->pins.size(); ++i) {
            if (FF->pins[i]->name == "C") {
                clkPinIdx = i;
            } else if (FF->pins[i]->name == "R") {
                srPinIdx = i;
            } else if (FF->pins[i]->name == "CE") {
                cePinIdx = i;
            } else if (FF->pins[i]->name == "D") {
                ffIPinIdx = i;
            }
        }
    }
    lutOPinIdx = 0;
}

void Database::ClearEmptyPack() {
    for (unsigned p = 0; p < packs.size(); p++) {
        Pack *curPack = packs[p];
        bool isEmptyPack = curPack->IsEmpty();
        if (isEmptyPack) {
            packs[p] = NULL;
            getSite(curPack->site->x, curPack->site->y)->pack = NULL;
            delete curPack;
        }
    }
    int i = 0;
    for (size_t p = 0; p < packs.size(); p++) {
        if (packs[p] != NULL) {
            packs[i] = packs[p];
            packs[i]->id = i;
            ++i;
        }
    }
    packs.resize(i);
}

// input range: 0 - 167
// output range: 0 - 83
// 0-65 -> 0-32 (66->33)
// 66 -> 33
// 67-102 -> 33-50 (36->18)
// 103 -> 51
// 104-167 -> 51-82 (64->32)
int db::SWCol(int x) {
    if (x <= 66)
        return x / 2;
    else if (x <= 103)
        return (x - 1) / 2;
    else if (x <= 167)
        return (x - 2) / 2;
    else {
        printlog(LOG_ERROR, "invalid x in SWCol()");
        return -1;
    }
}

// for LUT (#input <= 6), brute force (O(m*n)) is efficient
int db::NumDupInputs(const Instance &lhs, const Instance &rhs) {
    int numDup = 0;
    vector<Net *> nets1;
    for (auto pin1 : lhs.pins)
        if (pin1->type->type == 'i') nets1.push_back(pin1->net);
    for (auto pin2 : rhs.pins)
        if (pin2->type->type == 'i') {
            auto pos1 = find(nets1.begin(), nets1.end(), pin2->net);
            if (pos1 != nets1.end()) {
                ++numDup;
                *pos1 = NULL;
            }
        }
    return numDup;
}

bool db::IsLUTCompatible(const Instance &lhs, const Instance &rhs) {
    int tot = lhs.pins.size() + rhs.pins.size();
    return (tot <= 7) || (tot - 2 - db::NumDupInputs(lhs, rhs) <= 5);
}