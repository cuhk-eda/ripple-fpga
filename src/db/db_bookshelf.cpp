#include "db.h"
using namespace db;

bool read_line_as_tokens(istream &is, vector<string> &tokens) {
    tokens.clear();
    string line;
    getline(is, line);
    while (is && tokens.empty()) {
        string token = "";
        for (unsigned i=0; i<line.size(); ++i) {
            char currChar = line[i];
            if (isspace(currChar)) {
                if (!token.empty()) {
                    // Add the current token to the list of tokens
                    tokens.push_back(token);
                    token.clear();
                }
            }else{
                // Add the char to the current token
                token.push_back(currChar);
            }
        }
        if(!token.empty()){
            tokens.push_back(token);
        }
        if(tokens.empty()){
            // Previous line read was empty. Read the next one.
            getline(is, line);    
        }
    }
    return !tokens.empty();
}

bool Database::readAux(string file){
    string directory;
    size_t found=file.find_last_of("/\\");
    if (found==file.npos) {
        directory="./";
    }else{
        directory=file.substr(0,found);
        directory+="/";
    }
    
    ifstream fs(file);
    if (!fs.good()){
        printlog(LOG_ERROR, "Cannot open %s to read", file.c_str());
        return false;
    }
    
    string buffer;
    while (fs>>buffer) {
        if (buffer=="#") {
            //ignore comments
            fs.ignore(std::numeric_limits<streamsize>::max(), '\n');
            continue;
        }
        size_t dot_pos = buffer.find_last_of(".");
        if ( dot_pos == string::npos) {
            //ignore words without "dot"
            continue;
        }
        string ext=buffer.substr(dot_pos);
        if (ext==".nodes") {
            setting.io_nodes = directory+buffer;
        }else if (ext==".nets") {
            setting.io_nets  = directory+buffer;
        }else if (ext==".pl") {
            setting.io_pl    = directory+buffer;
        }else if (ext==".wts") {
            setting.io_wts   = directory+buffer;
        }else if (ext==".scl") {
            setting.io_scl   = directory+buffer;
        }else if(ext==".lib"){
            setting.io_lib   = directory+buffer;
        }else {
            continue;
        }
    }

    printlog(LOG_VERBOSE, "nodes = %s", setting.io_nodes.c_str());
    printlog(LOG_VERBOSE, "nets  = %s", setting.io_nets.c_str());
    printlog(LOG_VERBOSE, "pl    = %s", setting.io_pl.c_str());
    printlog(LOG_VERBOSE, "wts   = %s", setting.io_wts.c_str());
    printlog(LOG_VERBOSE, "scl   = %s", setting.io_scl.c_str());
    printlog(LOG_VERBOSE, "lib   = %s", setting.io_lib.c_str());
    fs.close();

    return true;
}

bool Database::readNodes(string file){
    ifstream fs(file);
    if (!fs.good()) {
        cerr<<"Read nodes file: "<<file<<" fail"<<endl;
        return false;
    }

    vector<string> tokens;
    while(read_line_as_tokens(fs, tokens)){
        Instance *instance = database.getInstance(tokens[0]);
        if(instance != NULL){
            printlog(LOG_ERROR, "Instance duplicated: %s", tokens[0].c_str());
        }else{
            Master *master = database.getMaster(Master::NameString2Enum(tokens[1]));
            if(master == NULL){
                printlog(LOG_ERROR, "Master not found: %s", tokens[1].c_str());
            }else{
                Instance newinstance(tokens[0], master);
                instance = database.addInstance(newinstance);
            }
        }
    }

    fs.close();
    return true;
}

bool Database::readNets(string file){
    ifstream fs(file);
    if (!fs.good()) {
        cerr<<"Read net file: "<<file<<" fail"<<endl;
        return false;
    }

    vector<string> tokens;
    Net *net = NULL;
    while(read_line_as_tokens(fs, tokens)){
        if(tokens[0][0] == '#'){
            continue;
        }
        if(tokens[0] == "net"){
            net = database.getNet(tokens[1]);
            if(net != NULL){
                printlog(LOG_ERROR, "Net duplicated: %s", tokens[1].c_str());
            }else{
                Net newnet(tokens[1]);
                net = database.addNet(newnet);
            }
        }else if(tokens[0] == "endnet"){
            net = NULL;
        }else{
            Instance *instance = database.getInstance(tokens[0].c_str());
            Pin *pin = NULL;

            if(instance != NULL){
                pin = instance->getPin(tokens[1]);
            }else{
                printlog(LOG_ERROR, "Instance not found: %s", tokens[0].c_str());
            }

            if(pin == NULL){
                printlog(LOG_ERROR, "Pin not found: %s", tokens[1].c_str());
            }else{
                net->addPin(pin);
                if (pin->type->type=='o' && pin->instance->master->name==Master::BUFGCE){
                    net->isClk=true;
                }
            }
        }
    }

    fs.close();
    return false;
}

bool Database::readPl(string file){
    ifstream fs(file);
    if (!fs.good()) {
        cerr<<"Read pl file: "<<file<<" fail"<<endl;
        return false;
    }
    vector<string> tokens;
    while(read_line_as_tokens(fs, tokens)){
        Instance *instance = database.getInstance(tokens[0]);
        if(instance == NULL){
            printlog(LOG_ERROR, "Instance not found: %s", tokens[0].c_str());
            continue;
        }
        int x = atoi(tokens[1].c_str());
        int y = atoi(tokens[2].c_str());
        int slot = atoi(tokens[3].c_str());
        instance->inputFixed = (tokens.size() >= 5 && tokens[4] == "FIXED");
        instance->fixed = instance->inputFixed;
        if(instance->IsFF()) slot += 16;
        else if(instance->IsCARRY()) slot += 32;
        place(instance, x, y, slot);
    }
    fs.close();
    return true;
}

bool Database::readScl(string file){
    ifstream fs(file);
    if (!fs.good()) {
        printlog(LOG_ERROR, "Cannot open %s to read", file.c_str());
        return false;
    }
    vector<string> tokens;
    while(read_line_as_tokens(fs, tokens)){
        if(tokens[0] == "SITE"){
            SiteType *sitetype = database.getSiteType(SiteType::NameString2Enum(tokens[1]));
            if(sitetype == NULL){
                SiteType newsitetype(SiteType::NameString2Enum(tokens[1]));
                sitetype = database.addSiteType(newsitetype);
            }else{
                printlog(LOG_WARN, "Duplicated site type: %s", sitetype->name);
            }
            while(read_line_as_tokens(fs, tokens)){
                if(tokens[0] == "END" && tokens[1] == "SITE"){
                    break;
                }
                Resource *resource = database.getResource(Resource::NameString2Enum(tokens[0]));
                if(resource == NULL){
                    Resource newresource(Resource::NameString2Enum(tokens[0]));
                    resource = database.addResource(newresource);
                }
                sitetype->addResource(resource, atoi(tokens[1].c_str()));
            }
        }
        if(tokens[0] == "RESOURCES"){
            while(read_line_as_tokens(fs, tokens)){
                if(tokens[0] == "END" && tokens[1] == "RESOURCES"){
                    break;
                }
                Resource *resource = database.getResource(Resource::NameString2Enum(tokens[0]));
                if(resource == NULL){
                    Resource newresource(Resource::NameString2Enum(tokens[0]));
                    resource = database.addResource(newresource);
                }
                for(int i=1; i<(int)tokens.size(); i++){
                    Master *master = database.getMaster(Master::NameString2Enum(tokens[i]));
                    if(master == NULL){
                        printlog(LOG_ERROR, "Master not found: %s", tokens[i].c_str());
                    }else{
                        resource->addMaster(master);
                    }
                }
            }
        }
        if(tokens[0] == "SITEMAP"){
            int nx = atoi(tokens[1].c_str());
            int ny = atoi(tokens[2].c_str());
            database.setSiteMap(nx, ny);
            database.setSwitchBoxes(nx/2-1, ny);
            while(read_line_as_tokens(fs, tokens)){
                if(tokens[0] == "END" && tokens[1] == "SITEMAP"){
                    break;
                }
                int x = atoi(tokens[0].c_str());
                int y = atoi(tokens[1].c_str());
                SiteType *sitetype = database.getSiteType(SiteType::NameString2Enum(tokens[2]));
                if(sitetype == NULL){
                    printlog(LOG_ERROR, "Site type not found: %s", tokens[2].c_str());
                }else{
                    database.addSite(x, y, sitetype);
                }
            }
        }
        if(tokens[0] == "CLOCKREGIONS"){
            int nx = atoi(tokens[1].c_str());
            int ny = atoi(tokens[2].c_str());
            
            crmap_nx = nx;
            crmap_ny = ny;
            clkrgns.assign(nx, vector<ClkRgn*>(ny, NULL));
            hfcols.assign(sitemap_nx,vector<HfCol*>(2*ny,NULL));
            
            for (int x=0; x<nx; x++){
                for (int y=0; y<ny; y++){
                    read_line_as_tokens(fs, tokens);
                    string name = tokens[0]+tokens[1];
                    int lx = atoi(tokens[3].c_str());
                    int ly = atoi(tokens[4].c_str());
                    int hx = atoi(tokens[5].c_str());
                    int hy = atoi(tokens[6].c_str());
                    clkrgns[x][y]=new ClkRgn(name, lx, ly, hx, hy, x, y);
                }
            }
        }
    }

    fs.close();
    return true;
}

bool Database::readLib(string file){
    ifstream fs(file);
    if (!fs.good()) {
        printlog(LOG_ERROR, "Cannot open %s to read", file.c_str());
        return false;
    }
    
    vector<string> tokens;
    Master *master = NULL;
    while(read_line_as_tokens(fs, tokens)){
        if (tokens[0]=="CELL") {
            master = database.getMaster(Master::NameString2Enum(tokens[1]));
            if(master == NULL){
                Master newmaster(Master::NameString2Enum(tokens[1]));
                master = database.addMaster(newmaster);
            }else{
                printlog(LOG_WARN, "Duplicated master: %s", tokens[1].c_str());
            }
        } else if (tokens[0]=="PIN") {
            char type = 'x';
            if(tokens.size() >= 4){
                if(tokens[3] == "CLOCK"){
                    type = 'c';
                }else if(tokens[3] == "CTRL"){
                    type = 'e';
                }
            }else if(tokens.size() >= 3){
                if(tokens[2] == "OUTPUT"){
                    type = 'o';
                }else if(tokens[2] == "INPUT"){
                    type = 'i';
                }
            }
            
            PinType pintype(tokens[1], type);
            if(master != NULL){
                master->addPin(pintype);
            }
        } else if (tokens[0]=="END") {
        }
    }

    fs.close();
    return true;
}

bool Database::readWts(string file){
    return false;
}

bool Database::writePl(string file){
    ofstream fs(file);
    if(!fs.good()){
        printlog(LOG_ERROR, "Cannot open %s to write", file.c_str());
        return false;
    }

    vector<Instance*>::iterator ii = database.instances.begin();
    vector<Instance*>::iterator ie = database.instances.end();
    int nErrorLimit = 10;
    for(; ii!=ie; ++ii){
        Instance *instance = *ii;
        if(instance->pack == NULL || instance->pack->site == NULL){
            if(nErrorLimit > 0){
                printlog(LOG_ERROR, "Instance not placed: %s", instance->name.c_str());
                nErrorLimit--;
            }else if(nErrorLimit == 0){
                printlog(LOG_ERROR, "(Remaining same errors are not shown)");
                nErrorLimit--;
            }
            continue;
        }
        Site *site = instance->pack->site;
        int slot = instance->slot;
        if(instance->IsFF()) slot -= 16;
        else if(instance->IsCARRY()) slot -= 32;
        fs<<instance->name<<" "<<site->x<<" "<<site->y<<" "<<slot;
        if(instance->inputFixed) fs<<" FIXED";
        fs<<endl;
    }

    return true;
}


