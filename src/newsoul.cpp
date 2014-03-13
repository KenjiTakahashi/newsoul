/*  newsoul - A SoulSeek client written in C++
    Copyright (C) 2006-2007 Ingmar K. Steen (iksteen@gmail.com)
    Copyright 2008 little blue poney <lbponey@users.sourceforge.net>
    Karol 'Kenji Takahashi' Woźniak © 2013 - 2014

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 */

#include "newsoul.h"
#include "searchmanager.h"

newsoul::Newsoul *newsoul::Newsoul::_instance = 0; // Yeah, right

newsoul::Newsoul::Newsoul() {
    /* Seed the random generator and fabricate our starting token. */
    srand(time(NULL));
    m_Token = rand();

    this->_buddyShares = NULL;

    Newsoul::_instance = this;
}

newsoul::Newsoul::~Newsoul() {
    delete this->_globalShares;
    if(this->_buddyShares != NULL) {
        delete this->_buddyShares;
    }
    delete this->_config;
}

void newsoul::Newsoul::parsePSet(std::initializer_list<const std::string> keys, int *i, int argc, char *argv[]) {
    if(++*i >= argc) {
        std::cout << "Not enough arguments." << std::endl;
    } else {
        this->_config->set(keys, std::string(argv[*i]));
    }
}

void newsoul::Newsoul::parsePSetBool(std::initializer_list<const std::string> keys, int *i, int argc, char *argv[]) {
    if(++*i >= argc) {
        std::cout << "Not enough arguments." << std::endl;
    } else {
        std::string s(argv[*i]);
        if(s == "yes") {
            this->_config->set(keys, true);
        } else if(s == "no") {
            this->_config->set(keys, false);
        } else {
            std::cout << "Invalid argument [" << s << "]." << std::endl;
        }
    }
}

void newsoul::Newsoul::parsePAdd(std::initializer_list<const std::string> keys, int *i, int argc, char *argv[]) {
    if(++*i >= argc) {
        std::cout << "Not enough arguments." << std::endl;
    } else {
        this->_config->add(keys, std::string(argv[*i]));
    }
}

void newsoul::Newsoul::parsePDel(std::initializer_list<const std::string> keys, int *i, int argc, char *argv[]) {
    if(++*i >= argc) {
        std::cout << "Not enough arguments." << std::endl;
    } else {
        this->_config->del(keys, argv[*i]);
    }
}

void newsoul::Newsoul::parsePart(std::map<const std::string, std::function<void(const std::string &sarg)>> func, const std::string carg, int *i, int argc, char *argv[]) {
    if(++*i == argc) {
        std::cout << carg << ": Not enough arguments." << std::endl;
        return;
    }
    for(; *i < argc; ++*i) {
        std::string sarg(argv[*i]);
        auto f = func.find(sarg);
        if(f != func.end()) {
            if(f->second != nullptr) {
                f->second(sarg);
            } else {
                this->parsePSet({carg, sarg}, i, argc, argv);
            }
        } else {
            std::cout << carg <<  ":Unrecognized argument [" << sarg << "]." << std::endl;
        }
    }
}

bool newsoul::Newsoul::parseSet(int *i, int argc, char *argv[]) {
    if(++*i == argc) {
        std::cout << "set: Not enough arguments." << std::endl;
        return false;
    }
    for(; *i < argc; ++*i) {
        std::string carg(argv[*i]);
        if(carg == "server") {
            this->parsePart({
                {"host", nullptr}, {"port", nullptr},
                {"username", nullptr}, {"password", nullptr}
            }, carg, i, argc, argv);
        } else if(carg == "p2p") {
            this->parsePart({
                {"ports", [this, carg, i, argc, argv](const std::string &sarg){
                    this->parsePSet({carg, sarg, "first"}, i, argc, argv);
                    this->parsePSet({carg, sarg, "last"}, i, argc, argv);
                }},
                {"mode", nullptr}
            }, carg, i, argc, argv);
        } else if(carg == "encoding") {
            this->parsePart({
                {"network", nullptr},
                {"local", nullptr},
            }, carg, i, argc, argv);
        } else if(carg == "downloads") {
            this->parsePart({
                {"complete", nullptr},
                {"incomplete", nullptr}
            }, carg, i, argc, argv);
        } else if(carg == "help") {
            std::cout << "Usage: newsoul [--debug(-d)] set(s) [<args>], where <args> are:" << std::endl << std::endl;
            std::cout << "from <config file> \t\t\tReads values from file [default: ~/.config/newsoul/config.json]" << std::endl;
            std::cout << "server <arg>, where <arg> is one of:" << std::endl;
            std::cout << "  host <name>\t\t\t\tSets Soulseek's server host name [default: server.slsknet.org]" <<std::endl;
            std::cout << "  port <number>\t\t\t\tSets Soulseek's server port number [default: 2242]" << std::endl;
            std::cout << "  username <name>\t\t\tSets name of the user [default: anonymous]" << std::endl;
            std::cout << "  password <pass>\t\t\tSets user's password [default: <empty>]" << std::endl;
            std::cout << "p2p <arg>, where <arg> is one of:" << std::endl;
            std::cout << "  ports <first> <last>\t\t\tSets port range for peer connections [default: 2235-2236]" << std::endl;
            std::cout << "  mode <passive|active>\t\t\tSets connection mode [default: passive]" << std::endl;
            std::cout << "encoding <arg>, where <arg> is one of:" << std::endl;
            std::cout << "  network <encoding>\t\t\tSets encoding used in network messages [default: UTF-8]" << std::endl;
            std::cout << "  local <encoding>\t\t\tSets encoding used in filesystem [default: UTF-8]" << std::endl;
            std::cout << "downloads <arg>, where <arg> is one of:" << std::endl;
            std::cout << "  complete <dir>\t\t\tSets directory for complete downloads [default: ~/.config/newsoul/complete]" << std::endl;
            std::cout << "  incomplete <dir>\t\t\tSets directory for incomplete downloads [default: <empty>]" << std::endl;
        } else {
            std::cout << "set: Unrecognized argument [" << carg << "]." << std::endl;
        }
    }
    return false;
}

bool newsoul::Newsoul::parseDatabase(int *i, int argc, char *argv[]) {
    if(++*i == argc) {
        std::cout << "database: Not enough arguments." << std::endl;
        return false;
    }
    for(; *i < argc; ++*i) {
        std::string carg(argv[*i]);
        if(carg == "rescan") {
            for(const std::string &dir : this->_config->getVec({"database", "global", "paths"})) {
                this->_globalShares->add({dir});
            }
            for(const std::string &dir : this->_config->getVec({"database", "local", "paths"})) {
                this->_buddyShares->add({dir});
            }
        } else if(carg == "global") {
            this->parsePart({
                {"list", [this](const std::string &sarg){
                    std::vector<std::string> items = this->_config->getVec({"database", "global", "paths"});
                    for(unsigned int i = 0; i < items.size(); ++i) {
                        std::cout << i << ": " << items[i] << std::endl;
                    }
                }},
                {"add", [this, i, argc, argv](const std::string &sarg){
                    this->parsePAdd({"database", "global", "paths"}, i, argc, argv);
                    this->_globalShares->add({argv[*i]});
                }},
                {"remove", [this, i, argc, argv](const std::string &sarg){
                    if(++*i >= argc) {
                        std::cout << "remove: Not enough arguments." << std::endl;
                        return;
                    }
                    std::vector<std::string> items = this->_config->getVec({"database", "global", "paths"});
                    int index = atoi(argv[*i]);
                    if(index >= 0 && index < (int)items.size()) {
                        this->_globalShares->remove({items[index]});
                        this->_config->del({"database", "global", "paths"}, items[index]);
                    } else {
                        std::cout << "remove: Invalid index [" << index << "]." << std::endl;
                    }
                }}
            }, carg, i, argc, argv);
        } else if(carg == "buddy") {
            this->parsePart({
                {"list", [this](const std::string &sarg){
                }},
                {"add", [this, i, argc, argv](const std::string &sarg){
                    this->parsePAdd({"database", "buddy", "paths"}, i, argc, argv);
                    this->_buddyShares->add({argv[*i]});
                }},
                {"remove", [this, i, argc, argv](const std::string &sarg){
                    this->parsePDel({"database", "buddy", "paths"}, i, argc, argv);
                    this->_buddyShares->remove({argv[*i]});
                }},
                {"enabled", [this, i, argc, argv](const std::string &sarg){
                    this->parsePSetBool({"database", "buddy", "enabled"}, i, argc, argv);
                }}
            }, carg, i, argc, argv);
        } else if(carg == "help") {
            std::cout << "Usage: newsoul [--debug(-d)] database(d) [<args>], where <args> are:" << std::endl << std::endl;
            std::cout << "rescan\t\t\t\t\tUpdates database NOW" << std::endl;
            std::cout << "global <arg>, where <arg> is one of:" << std::endl;
            std::cout << "  list\t\t\t\t\tLists shared directories" << std::endl;
            std::cout << "  add <dir>\t\t\t\tAdds new shared directory" << std::endl;
            std::cout << "  remove <index>\t\t\tRemoves a shared directory" << std::endl;
            std::cout << "buddy <arg>, where <arg> is one of:" << std::endl;
            std::cout << "  list\t\t\t\t\tLists shared directories" << std::endl;
            std::cout << "  add <dir>\t\t\t\tAdds new shared directory" << std::endl;
            std::cout << "  remove <index>\t\t\tRemoves a shared directory" << std::endl;
            std::cout << "  enabled <yes|no>\t\t\tTurns buddies only shares on or off [default: no]" << std::endl;
        } else {
            std::cout << "database: Unrecognized argument [" << carg << "]." << std::endl;
        }
    }
    return false;
}

bool newsoul::Newsoul::parseListeners(int *i, int argc, char *argv[]) {
    this->parsePart({
        {"help", [this](const std::string &sarg){
            std::cout << "Usage: newsoul [--debug(-d)] listeners(l) [<args>], where <args> are:" << std::endl << std::endl;
            std::cout << "list\t\t\tLists all existing listeners" << std::endl;
            std::cout << "add <addr:port|fn>\tAdds new listener" << std::endl;
            std::cout << "remove <addr:port|fn>\tRemoves existing listener" << std::endl;
            std::cout << "password <pass>\t\tSets listeners password [default: p]" << std::endl;
        }},
        {"list", [this](const std::string &sarg){
            std::vector<std::string> items = this->_config->getVec({"listeners", "paths"});
            for(unsigned int i = 0; i < items.size(); ++i) {
                std::cout << i << ": " << items[i] << std::endl;
            }
        }},
        {"add", [this, i, argc, argv](const std::string &sarg){
            this->parsePAdd({"listeners", "paths"}, i, argc, argv);
        }},
        {"remove", [this, i, argc, argv](const std::string &sarg){
            this->parsePDel({"listeners", "paths"}, i, argc, argv);
        }},
        {"password", nullptr}
    }, "listeners", i, argc, argv);
    return false;
}

bool newsoul::Newsoul::parseArgs(int argc, char *argv[]) {
    bool debug = false;

    if(2 < argc && std::string(argv[1]) == "set" && std::string(argv[2]) == "from") {
        if(3 < argc) {
            this->_config = new Config(std::string(argv[3]));
        } else {
            std::cout << "Not enough arguments, loading default config." << std::endl;
            this->_config = new Config();
        }
        return true;
    } else {
        this->_config = new Config();
    }

    this->LoadShares();  //FIXME

    for(int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if(arg == "--debug" || arg == "-d") {
            debug = true;
            continue;
        }
        if(arg == "version" || arg == "v") {
            std::cout << "newsoul :: " << this->version << std::endl;
            return false;
        } else if(arg == "set" || arg == "s") {
            return this->parseSet(&i, argc, argv);
        } else if(arg == "database" || arg == "d") {
            return this->parseDatabase(&i, argc, argv);
        } else if(arg == "listeners" || arg == "l") {
            return this->parseListeners(&i, argc, argv);
        } else if(arg == "help" || arg == "h") {
            if(++i != argc) {
                std::cout << "Ignoring unrecognized argument: [" << argv[i] << std::endl;
            }
            std::cout << "Usage: newsoul [--debug(-d)] [version(v)|help(h)|set(s)|database(d)] [<args>]" << std::endl << std::endl;
            std::cout << "Main options:" << std::endl;
            std::cout << "  version\t\tPrints version number and exists." << std::endl;
            std::cout << "  [cmd] help\t\tPrints detailed info about [cmd]. This screen if no cmd specified." << std::endl;
            std::cout << "  set\t\t\tChanges configuration values. See [newsoul set help]." << std::endl;
            std::cout << "  database\t\tManages shares database. See [newsoul database help]." << std::endl;
            std::cout << "  listeners\t\tManages listening interfaces. See [newsoul listeners help]." << std::endl;
            std::cout << std::endl << "Additional options:" << std::endl;
            std::cout << "  --debug(-d) \t\tPrints some more informations about what is going on" << std::endl;
            std::cout << std::endl << "Signals:" << std::endl;
            std::cout << "  HUP\t\t\tReloads database(s)" << std::endl;
            std::cout << "  ALRM\t\t\tReconnects to server" << std::endl;
            return false;
        } else {
            std::cout << "newsoul: Unrecognized command [" << arg << "]." << std::endl;
            return false;
        }
    }

    if(!debug) {
    }

    return true;
}

int newsoul::Newsoul::run(int argc, char *argv[]) {
    //google::InstallFailureFunction(&handleSignals); //FIXME: Make better func
    bool run = this->parseArgs(argc, argv);
    if(!run) {
        return 0;
    }

    m_Reactor = new NewNet::Reactor();
    /* Instantiate the various components. Order can be important here. */
    m_Codeset = new CodesetManager(this);
    m_Server = new ServerManager(this);
    m_Peers = new PeerManager(this);
    m_Downloads = new DownloadManager(this);
    m_Uploads = new UploadManager(this);
    m_Ifaces = new IfaceManager(this);
    m_Searches = new SearchManager(this);

    this->LoadDownloads();

#ifndef _WIN32
    signal(SIGHUP, &handleSignals);
    signal(SIGALRM, &handleSignals);
#endif
    signal(SIGINT, &handleSignals);

    this->m_Server->connect();
    this->m_Reactor->run();
    this->m_Downloads->saveDownloads();
    return 0;
}

void newsoul::Newsoul::LoadShares() {
    const std::string shares = this->_config->getStr({"database", "global", "dbpath"});
    if (!shares.empty()) {
        this->_globalShares = new SharesDB(shares, [this]{this->sendSharedNumber();});
    }
    const std::string bshares = this->_config->getStr({"database", "buddy", "dbpath"});
    if (!bshares.empty() && haveBuddyShares()) {
        this->_buddyShares = new SharesDB(bshares, [this]{this->sendSharedNumber();});
    }
}

void newsoul::Newsoul::LoadDownloads() {
    m_Downloads->loadDownloads();
}

bool newsoul::Newsoul::isBanned(const std::string u) {
    return config()->contains({"users", "banned"}, u);
}

bool newsoul::Newsoul::isIgnored(const std::string u) {
    return config()->contains({"users", "ignored"}, u);
}

bool newsoul::Newsoul::isTrusted(const std::string u) {
    return config()->contains({"users", "trusted"}, u);
}

bool newsoul::Newsoul::isBuddied(const std::string u) {
    return config()->contains({"users", "buddies"}, u);
}

bool newsoul::Newsoul::isPrivileged(const std::string u) {
    return std::find(mPrivilegedUsers.begin(), mPrivilegedUsers.end(), u) != mPrivilegedUsers.end();
}

bool newsoul::Newsoul::toBuddiesOnly() {
    return config()->getBool({"uploads", "buddiesOnly"});
}

bool newsoul::Newsoul::haveBuddyShares() {
    return config()->getBool({"database", "buddy", "enabled"});
}

bool newsoul::Newsoul::trustingUploads() {
    return config()->getBool({"uploads", "allowTrusted"});
}

bool newsoul::Newsoul::autoClearFinishedDownloads() {
    return config()->getBool({"downloads", "autoclear"});
}

bool newsoul::Newsoul::autoRetryDownloads() {
    return config()->getBool({"downloads", "autoretry"});
}

bool newsoul::Newsoul::autoClearFinishedUploads() {
    return config()->getBool({"uploads", "autoclear"});
}

bool newsoul::Newsoul::privilegeBuddies() {
    return config()->getBool({"uploads", "buddiesFirst"});
}

unsigned int newsoul::Newsoul::upSlots() {
    return (unsigned int)config()->getInt({"uploads", "slots"});
}

unsigned int newsoul::Newsoul::downSlots() {
    return (unsigned int)config()->getInt({"downloads", "slots"});
}

// Add this user to the list of privileged ones
void newsoul::Newsoul::addPrivilegedUser(const std::string & user) {
    if (!isPrivileged(user)) {
        mPrivilegedUsers.push_back(user);
        VLOG(1) << "Privileged users no: " << mPrivilegedUsers.size();
    }
}

// Replace the privileged users list with this new one
void newsoul::Newsoul::setPrivilegedUsers(const std::vector<std::string> & users) {
    mPrivilegedUsers = users;
    VLOG(1) << "Privileged users no:" << mPrivilegedUsers.size();
}

void newsoul::Newsoul::sendSharedNumber() {
	if(server()->loggedIn()) {
        unsigned int numFiles = this->_globalShares->filesCount();
        unsigned int numFolders = this->_globalShares->dirsCount();
        if(haveBuddyShares()) {
            numFiles += this->_buddyShares->filesCount();
            numFolders += this->_buddyShares->dirsCount();
        }
	    SSharedFoldersFiles msg(numFolders, numFiles);
	    server()->sendMessage(msg.make_network_packet());
	}
}

bool newsoul::Newsoul::isEnabledPrivRoom() {
    return config()->getBool({"privateRooms", "enabled"});
}

void newsoul::Newsoul::handleSignals(int signal) {
    if(signal == SIGINT) {
        LOG(INFO) << "Got SIGINT, stopping the reactor";
        _instance->m_Reactor->stop();
#ifndef _WIN32
    } else if(signal == SIGHUP) {
        LOG(INFO) << "Got SIGHUP, reloading shares";
        _instance->LoadShares();
    } else if(signal == SIGALRM) {
        LOG(INFO) << "Got SIGALRM, reconnecting";
        if(!_instance->m_Server->loggedIn()) {
            _instance->m_Server->connect();
        }
#endif
    }

#ifndef _WIN32
    ::signal(SIGHUP, &handleSignals);
    ::signal(SIGALRM, &handleSignals);
#endif
    ::signal(SIGINT, &handleSignals);
}
