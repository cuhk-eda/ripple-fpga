#include "./lg_data.h"
#include "../gp/gp_setting.h"

using namespace db;

void LGData::GetDispStatics() {
    // get disp info
    double ffDisp = 0, lutDisp = 0, ramDisp = 0, dspDisp = 0;
    int nFf = 0, nLut = 0, nRam = 0, nDsp = 0;
    double ffMaxDisp = -1, lutMaxDisp = -1, ramMaxDisp = -1, dspMaxDisp = -1;

    for (const auto& group : groups) {
        double disp = 0;

        const int orgX = group.x;
        const int orgY = group.y;

        Site* targetSite = database.getSite(groupsX[group.id], groupsY[group.id]);

        // scale to hpwl calc method
        if (group.GetSiteType() != SiteType::SLICE) {
            disp = abs(targetSite->x - database.getSite(orgX, orgY)->x) * 0.5 +
                   abs(targetSite->y - database.getSite(orgX, orgY)->y);
        } else {
            disp = abs(targetSite->x - orgX) * 0.5 + abs(targetSite->y - orgY);
        }

        dispPerSite[orgX][orgY] += disp;
        dispPerGroup[group.id] = disp;

        if (targetSite->type->name == SiteType::SLICE) {
            for (auto inst : group.instances) {
                if (inst->IsLUT()) {
                    nLut++;
                    lutDisp += disp;
                    lutMaxDisp = max(disp, lutMaxDisp);
                } else {
                    nFf++;
                    ffDisp += disp;
                    ffMaxDisp = max(disp, ffMaxDisp);
                }
            }
        } else if (targetSite->type->name == SiteType::DSP) {
            dspDisp += group.instances.size() * disp;
            dspMaxDisp = max(dspMaxDisp, disp);
            nDsp += group.instances.size();
        } else if (targetSite->type->name == SiteType::BRAM) {
            ramDisp += group.instances.size() * disp;
            ramMaxDisp = max(ramMaxDisp, disp);
            nRam += group.instances.size();
        }
    }

    if (lutDisp != 0)
        printlog(LOG_INFO, "lut: avg_disp=%.3f, max_disp=%.1f, tot_disp=%.1f", lutDisp / nLut, lutMaxDisp, lutDisp);
    if (ffDisp != 0)
        printlog(LOG_INFO, "ff: avg_disp=%.3f, max_disp=%.1f, tot_disp=%.1f", ffDisp / nFf, ffMaxDisp, ffDisp);
    if (ramDisp != 0) printlog(LOG_INFO, "ram: avg_disp=%.3f, max_disp=%.1f", ramDisp / nRam, ramMaxDisp);
    if (dspDisp != 0) printlog(LOG_INFO, "dsp: avg_disp=%.3f, max_disp=%.1f", dspDisp / nDsp, dspMaxDisp);
    if (invokeCount != 0)
        printlog(LOG_INFO,
                 "successful merge ratio: %.3f (%d/%d)",
                 successCount * 1.0 / invokeCount,
                 successCount,
                 invokeCount);
}

void LGData::GetPackStatics() {
    // get packing info
    int nSlice = 0, nDsp = 0, nRam = 0, nIo = 0;
    int nLutComp = 0, nFfComp = 0, nIoComp = 0;
    for (auto p : database.packs) {
        if (p->site->type->name == SiteType::SLICE) {
            nSlice++;
            for (auto i : p->instances) {
                if (i != NULL) {
                    if (i->IsLUT())
                        nLutComp++;
                    else
                        nFfComp++;
                }
            }
        } else {
            int curComp = 0;
            for (auto i : p->instances) {
                if (i != NULL) {
                    curComp++;
                }
            }
            if (p->site->type->name == SiteType::DSP) {
                nDsp++;
            } else if (p->site->type->name == SiteType::BRAM) {
                nRam++;
            } else {
                nIo++;
                nIoComp += curComp;
            }
        }
    }
    // printlog(LOG_INFO, "#pack = %d",database.packs.size());
    if (nSlice != 0) printlog(LOG_INFO, "#slice_pack = %d(%d/16,%d/16)", nSlice, nLutComp / nSlice, nFfComp / nSlice);
    printlog(LOG_INFO, "#dsp_pack = %d, #ram_pack = %d", nDsp, nRam);
    // printlog(LOG_INFO, "#io_pack = %d(%d/64)",nIo,nIoComp/nIo);
}

double LGData::GetHpwl() {
    double wl = 0;
    for (auto& nb : netBox) {
        wl += nb.uhp();
    }
    return wl;
}

void LGData::InitNetInfo() {
    netBox.clear();
    net2Gids.clear();

    nPinPerSite.assign(database.sitemap_nx, vector<int>(database.sitemap_ny, 0));
    nPinPerGroup.assign(groups.size(), 0);

    netBox.resize(database.nets.size());
    net2Gids.resize(database.nets.size());

    for (int n = 0; n < (int)database.nets.size(); n++) {
        Net* curNet = database.nets[n];

        for (auto pin : curNet->pins) {
            if (pin->instance != NULL) {
                Group& group = groups[inst2Gid[pin->instance->id]];
                // get pins distribution
                Site* site = database.getSite(group.x, group.y);
                nPinPerSite[site->x][site->y]++;
                nPinPerGroup[group.id]++;
                // get connected group
                net2Gids[n].push_back(group.id);
            }
        }
        sort(net2Gids[n].begin(), net2Gids[n].end());
        net2Gids[n].resize(distance(net2Gids[n].begin(), unique(net2Gids[n].begin(), net2Gids[n].end())));

        // get net xy & bound
        for (auto gid : net2Gids[n]) {
            Group& group = groups[gid];
            netBox[n].insert(group.x, group.y);
        }
    }
}

void LGData::InitGroupInfo() {
    group2Nets.assign(groups.size(), {});
    groupIds.assign(groups.size(), -1);
    inst2Gid.assign(database.instances.size(), -1);

    groupsX.assign(groups.size(), 0);
    groupsY.assign(groups.size(), 0);

    for (unsigned g = 0; g < groups.size(); g++) {
        // get the nets for each group
        for (int i = 0; i < (int)groups[g].instances.size(); i++) {
            if (groups[g].instances[i] != NULL) {
                for (int p = 0; p < (int)groups[g].instances[i]->pins.size(); p++) {
                    if (groups[g].instances[i]->pins[p]->net != NULL) {
                        group2Nets[g].push_back(groups[g].instances[i]->pins[p]->net);
                    }
                }
            }
        }
        sort(group2Nets[g].begin(), group2Nets[g].end());
        group2Nets[g].resize(distance(group2Nets[g].begin(), unique(group2Nets[g].begin(), group2Nets[g].end())));

        // instance to group mapping
        for (int i = 0; i < (int)groups[g].instances.size(); i++) {
            if (groups[g].instances[i] != NULL) {
                inst2Gid[groups[g].instances[i]->id] = groups[g].id;
            }
        }

        // get group id
        groupIds[g] = groups[g].id;
        groupsX[g] = groups[g].x;
        groupsY[g] = groups[g].y;
    }

    // put groups into bin
    groupMap.assign(database.sitemap_nx, vector<vector<int>>(database.sitemap_ny));
    for (unsigned g = 0; g < groups.size(); g++) {
        int gx = groupsX[g];
        int gy = groupsY[g];
        groupMap[gx][gy].push_back(groups[g].id);
    }
}

void LGData::InitLGStat() {
    invokeCount = 0;
    successCount = 0;

    placedGroupMap.assign(database.sitemap_nx, vector<vector<int>>(database.sitemap_ny));

    dispPerSite.assign(database.sitemap_nx, vector<double>(database.sitemap_ny, 0));
    dispPerGroup.assign(groups.size(), 0.0);
}

void LGData::Init(lgPackMethod packMethod) {
    database.unplaceAll();
    database.ClearEmptyPack();

    for (auto& g : groups) {
        if ((int)g.x >= database.sitemap_nx) {
            printlog(LOG_WARN, "group %d x out of boundary:%.2lf", g.id, g.x);
            g.x = database.sitemap_nx - 0.0001;
        } else if ((int)g.x < 0) {
            printlog(LOG_WARN, "group %d x out of boundary:%.2lf", g.id, g.x);
            g.x = 0;
        }

        if ((int)g.y >= database.sitemap_ny) {
            printlog(LOG_WARN, "group %d y out of boundary:%.2lf", g.id, g.y);
            g.y = database.sitemap_ny - 0.0001;
        } else if ((int)g.y < 0) {
            printlog(LOG_WARN, "group %d y out of boundary:%.2lf", g.id, g.y);
            g.y = 0;
        }
    }

    InitGroupInfo();
    InitNetInfo();
    InitLGStat();

    clbMap.assign(database.sitemap_nx, vector<CLBBase*>(database.sitemap_ny, NULL));

    // assign clb to slice
    for (int x = 0; x < database.sitemap_nx; x++) {
        for (int y = 0; y < database.sitemap_ny; y++) {
            if (database.getSite(x, y)->type->name == SiteType::SLICE) {
                if (packMethod == USE_CLB1)
                    clbMap[x][y] = new CLB1;
                else if (packMethod == USE_CLB2)
                    clbMap[x][y] = new CLB2;
                else
                    printlog(LOG_ERROR, "wrong pack method");
            }
        }
    }
}

LGData::LGData(vector<Group>& _groups) : groups(_groups) {}

void LGData::Group2Pack() {
    groups.clear();
    groups.resize(database.packs.size());
    for (int p = 0; p < (int)database.packs.size(); p++) {
        groups[p].x = database.packs[p]->site->cx();
        groups[p].y = database.packs[p]->site->cy();
        groups[p].id = p;
        groups[p].instances = database.packs[p]->instances;
    }
    groupMap.clear();
    groupMap.resize(database.sitemap_nx, vector<vector<int>>(database.sitemap_ny));
    for (const auto& g : groups) groupMap[g.x][g.y].push_back(g.id);
}

void LGData::UpdateGroupXY() {
    for (unsigned i = 0; i < groups.size(); ++i) {
        groups[i].x = groupsX[i];
        groups[i].y = groupsY[i];
    }
}

void LGData::UpdateGroupXYnOrder() {
    UpdateGroupXY();

    // only maintain the lg order of each site, result maybe different if lg is performed again
    groupIds.clear();
    for (int x = 0; x < database.sitemap_nx; x++) {
        for (int y = 0; y < database.sitemap_ny; y++) {
            for (auto gid : placedGroupMap[x][y]) {
                groupIds.push_back(gid);
            }
        }
    }

    vector<Group> tmpGroups;
    for (auto gid : groupIds) {
        tmpGroups.push_back(groups[gid]);
        tmpGroups[tmpGroups.size() - 1].id = tmpGroups.size() - 1;
    }
    groups = move(tmpGroups);

    groupMap.assign(database.sitemap_nx, vector<vector<int>>(database.sitemap_ny));
    for (auto group : groups) groupMap[group.x][group.y].push_back(group.id);
}

void LGData::GetResult(lgRetrunGroup retGroup) {
    // transform clb to pack
    for (int x = 0; x < database.sitemap_nx; x++) {
        for (int y = 0; y < database.sitemap_ny; y++) {
            if (clbMap[x][y] != NULL && !clbMap[x][y]->IsEmpty()) {
                Group tmpGroup;
                clbMap[x][y]->GetResult(tmpGroup);
                for (int i = 0; i < 32; i++) {
                    if (tmpGroup.instances[i] != NULL) {
                        database.place(tmpGroup.instances[i], database.getSite(x, y), i);
                    }
                }
            }
        }
    }

    database.ClearEmptyPack();
    if (retGroup == GET_SLICE)
        Group2Pack();
    else if (retGroup == UPDATE_XY_ORDER)
        UpdateGroupXYnOrder();
    else if (retGroup == UPDATE_XY)
        UpdateGroupXY();
    else if (retGroup == NO_UPDATE)
        ;
    else
        printlog(LOG_ERROR, "wrong usage in LGData::GetResult");

    // clean up
    for (int x = 0; x < database.sitemap_nx; x++) {
        for (int y = 0; y < database.sitemap_ny; y++) {
            delete clbMap[x][y];
        }
    }
}

void LGData::PartialUpdate(Group& group, Site* targetSite) {
    int gid = group.id;
    double tarX = targetSite->cx();
    double tarY = targetSite->cy();

    // update bounding box
    for (auto net : group2Nets[gid]) {
        netBox[net->id].erase(groupsX[gid], groupsY[gid]);
        netBox[net->id].insert(targetSite->cx(), targetSite->cy());
    }

    // update pins distribution
    nPinPerSite[groupsX[gid]][groupsY[gid]] -= nPinPerGroup[gid];
    nPinPerSite[tarX][tarY] += nPinPerGroup[gid];

    // update group location info
    groupsX[gid] = tarX;
    groupsY[gid] = tarY;
    placedGroupMap[targetSite->x][targetSite->y].push_back(gid);
}
