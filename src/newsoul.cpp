/*  newsoul - A SoulSeek client written in C++
    Copyright (C) 2006-2007 Ingmar K. Steen (iksteen@gmail.com)
    Copyright 2008 little blue poney <lbponey@users.sourceforge.net>
    Karol 'Kenji Takahashi' Woźniak © 2013

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

    _instance = this;
}

newsoul::Newsoul::~Newsoul() { }

void newsoul::Newsoul::parsePiece(std::initializer_list<const std::string> keys, int *i, int argc, char *argv[]) {
    if(++(*i) > argc) {
        std::cout << "Not enough arguments" << std::endl;
    } else {
        this->_config->set(keys, argv[*i]);
    }
}

void newsoul::Newsoul::parsePart(std::map<const std::string, std::function<void(const std::string sarg)>> func, const std::string carg, int *i, int argc, char *argv[]) {
    if(++*i == argc) {
        std::cout << "Not enough arguments" << std::endl;
        return;
    }
    for(; *i < argc; ++*i) {
        std::string sarg(argv[*i]);
        auto f = func.find(sarg);
        if(f != func.end()) {
            if(f->second != nullptr) {
                f->second(sarg);
            } else {
                this->parsePiece({carg, sarg}, i, argc, argv);
            }
        } else {
            std::cout << "Ignored invalid argument [" << sarg << "]" << std::endl;
        }
    }
}

bool newsoul::Newsoul::parseSet(int *i, int argc, char *argv[]) {
    if(std::string(argv[*i + 1]) == "from") {
        if(++*i > argc) {
            std::cout << "Not enough arguments, loading default config" << std::endl;
        } else {
            this->_config = new Config(std::string(argv[*i]));
        }
        return true;
    } else {
        this->_config = new Config();
    }
    if(++*i == argc) {
        std::cout << "No config arguments supplied, command ignored" << std::endl;
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
                {"ports", [this, carg, i, argc, argv](const std::string sarg){
                    this->parsePiece({carg, sarg, "first"}, i, argc, argv);
                    this->parsePiece({carg, sarg, "last"}, i, argc, argv);
                }},
                {"mode", nullptr}
            }, carg, i, argc, argv);
        } else if(carg == "listeners") {
            this->parsePart({
                {"list", [](const std::string sarg){}},
                {"add", [this, carg, i, argc, argv](const std::string sarg){
                    if(++*i > argc) {
                        std::cout << "Not enough arguments" << std::endl;
                    } else {
                        this->parsePiece({carg, sarg, argv[*i]}, i, argc, argv);
                    }
                }},
                {"remove", [](const std::string sarg){}},
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
        } else {
            std::cout << "bailing out" << std::endl; //FIXME
        }
    }
    return false;
}

bool newsoul::Newsoul::parseDatabase(int *i, int argc, char *argv[]) {
    if(++*i == argc) {
        std::cout << "No config arguments supplied, command ignored" << std::endl;
        return false;
    }
    for(; *i < argc; ++*i) {
        std::string carg(argv[*i]);
        if(carg == "rescan") {
        } else if(carg == "global") {
        } else if(carg == "buddy") {
        }
    }
    return false;
}

bool newsoul::Newsoul::parseArgs(int argc, char *argv[]) {
    bool debug = false;
    for(int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if(arg == "--debug" || arg == "-d") {
            debug = true;
        }
        if(arg == "version" || arg == "v") {
            std::cout << "newsoul :: " << this->version << std::endl;
            return false;
        } else if(arg == "set" || arg == "s") {
            bool end = this->parseSet(&i, argc, argv);
            if(end) {
                return true;
            }
        } else if(arg == "database" || arg == "d") {
            this->parseDatabase(&i, argc, argv);
        } else if(arg == "help" || arg == "h") {
            if(++i == argc) {
                std::cout << "Usage: newsoul [--debug(-d)] [version(v)|help(h)|set(s)|database(d)] [<args>]" << std::endl << std::endl;
                std::cout << "Main options:" << std::endl;
                std::cout << "  version\t\tPrints version number and exists." << std::endl;
                std::cout << "  help [cmd]\t\tPrints detailed info about [cmd]. This screen if no cmd specified." << std::endl;
                std::cout << "  set\t\t\tChanges configuration values. See [newsoul help set]." << std::endl;
                std::cout << "  database\t\tManages shares database. See [newsoul help database]." << std::endl;
                std::cout << std::endl << "Additional options:" << std::endl;
                std::cout << "  --debug(-d) \t\tprints some more informations about what is going on" << std::endl;
                std::cout << std::endl << "Signals:" << std::endl;
                std::cout << "  HUP\t\t\treloads database(s)" << std::endl;
                std::cout << "  ALRM\t\t\treconnects to server" << std::endl;
            } else {
                std::string carg(argv[i]);
                if(carg == "set") {
                    std::cout << "Usage: newsoul [--debug(-d)] set(s) [<args>], where <args> are:" << std::endl << std::endl;
                    std::cout << "from <config file> \t\t\treads values from file [default: ~/.config/newsoul/config.json]" << std::endl;
                    std::cout << "server <arg>, where <arg> is one of:" << std::endl;
                    std::cout << "  host <name>\t\t\t\tsets Soulseek's server host name [default: server.slsknet.org]" <<std::endl;
                    std::cout << "  port <number>\t\t\t\tsets Soulseek's server port number [default: 2242]" << std::endl;
                    std::cout << "  username <name>\t\t\tsets name of the user [default: anonymous]" << std::endl;
                    std::cout << "  password <pass>\t\t\tsets user's password [default: <empty>]" << std::endl;
                    std::cout << "p2p <arg>, where <arg> is one of:" << std::endl;
                    std::cout << "  ports <first> <last>\t\t\tsets port range for peer connections [default: 2235-2236]" << std::endl;
                    std::cout << "  mode <passive(p)|active(a)>\t\tsets connection mode [default: passive]" << std::endl;
                    std::cout << "listeners <arg>, where <arg> is one of:" << std::endl;
                    std::cout << "  list\t\t\t\t\tlists all all existing listeners" << std::endl;
                    std::cout << "  add <tcp|unix> <addr:port|fn> [pass]\tadds new listener, optionally password protected" << std::endl;
                    std::cout << "  remove <index>\t\t\tremoves existing listener" << std::endl;
                    std::cout << "encoding <arg>, where <arg> is one of:" << std::endl;
                    std::cout << "  network <encoding>\t\t\tsets encoding used in network messages [default: UTF-8]" << std::endl;
                    std::cout << "  local <encoding>\t\t\tsets encoding used in filesystem [default: UTF-8]" << std::endl;
                    std::cout << "downloads <arg>, where <arg> is one of:" << std::endl;
                    std::cout << "  complete <dir>\t\t\tsets directory for complete downloads [default: ~/.newsoul/complete]" << std::endl;
                    std::cout << "  incomplete <dir>\t\t\tsets directory for incomplete downloads [default: <empty>]" << std::endl;
                } else if(carg == "database") {
                    std::cout << "Usage: newsoul [--debug(-d)] database(d) [<args>], where <args> are:" << std::endl << std::endl;
                    std::cout << "rescan\t\t\t\t\tupdates database NOW" << std::endl;
                    std::cout << "global <arg>, where <arg> is one of:" << std::endl;
                    std::cout << "  list\t\t\t\t\tlists shared directories" << std::endl;
                    std::cout << "  add <dir>\t\t\t\tadds new shared directory" << std::endl;
                    std::cout << "  remove <index>\t\t\tremoves a shared directory" << std::endl;
                    std::cout << "buddy <arg>, where <arg> is one of:" << std::endl;
                    std::cout << "  list\t\t\t\t\tlists shared directories" << std::endl;
                    std::cout << "  add <dir>\t\t\t\tadds new shared directory" << std::endl;
                    std::cout << "  remove <index>\t\t\tremoves a shared directory" << std::endl;
                    std::cout << "  enabled <yes|no>\t\t\tturns buddies only shares on or off [default: no]" << std::endl;
                } else {
                    std::cout << "help: Unrecognized command [" << carg << "], leaving." << std::endl;
                }
            }
            return false;
        } else {
            std::cout << "newsoul: Unrecognized command [" << arg << "], leaving" << std::endl;
            return false;
        }
    }

    return true;
}

int newsoul::Newsoul::run(int argc, char *argv[]) {
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

    this->LoadShares();
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
    const std::string shares = this->_config->getStr({"shares", "global", "dbfile"});
    if (!shares.empty()) {
        this->_globalShares = new SharesDB(shares, [this]{this->sendSharedNumber();});
    }
    const std::string bshares = this->_config->getStr({"shares", "buddy", "dbfile"});
    if (!bshares.empty() && haveBuddyShares()) {
        this->_buddyShares = new SharesDB(bshares, [this]{this->sendSharedNumber();});
    }
}

void newsoul::Newsoul::LoadDownloads() {
    m_Downloads->loadDownloads();
}

bool newsoul::Newsoul::isBanned(const std::string u) {
    return config()->contains({"banned"}, u);
}

bool newsoul::Newsoul::isIgnored(const std::string u) {
    return config()->contains({"ignored"}, u);
}

bool newsoul::Newsoul::isTrusted(const std::string u) {
    return config()->contains({"trusted"}, u);
}

bool newsoul::Newsoul::isBuddied(const std::string u) {
    return config()->contains({"buddies"}, u);
}

bool newsoul::Newsoul::isPrivileged(const std::string u) {
    return std::find(mPrivilegedUsers.begin(), mPrivilegedUsers.end(), u) != mPrivilegedUsers.end();
}

bool newsoul::Newsoul::toBuddiesOnly() {
    return config()->getBool({"transfers", "only_buddies"});
}

bool newsoul::Newsoul::haveBuddyShares() {
    return config()->getBool({"transfers", "have_buddy_shares"});
}

bool newsoul::Newsoul::trustingUploads() {
    return config()->getBool({"transfers", "trusting_uploads"});
}

bool newsoul::Newsoul::autoClearFinishedDownloads() {
    return config()->getBool({"transfers", "autoclear_finished_downloads"});
}

bool newsoul::Newsoul::autoRetryDownloads() {
    return config()->getBool({"transfers", "autoretry_downloads"});
}

bool newsoul::Newsoul::autoClearFinishedUploads() {
    return config()->getBool({"transfers", "autoclear_finished_uploads"});
}

bool newsoul::Newsoul::privilegeBuddies() {
    return config()->getBool({"transfers", "privilege_buddies"});
}

unsigned int newsoul::Newsoul::upSlots() {
    return (unsigned int)config()->getInt({"transfers", "upload_slots"});
}

unsigned int newsoul::Newsoul::downSlots() {
    return (unsigned int)config()->getInt({"transfers", "download_slots"});
}

// Add this user to the list of privileged ones
void newsoul::Newsoul::addPrivilegedUser(const std::string & user) {
    if (!isPrivileged(user)) {
        mPrivilegedUsers.push_back(user);
        NNLOG("newsoul.debug", "%u privileged users", mPrivilegedUsers.size());
    }
}

// Replace the privileged users list with this new one
void newsoul::Newsoul::setPrivilegedUsers(const std::vector<std::string> & users) {
    mPrivilegedUsers = users;
    NNLOG("newsoul.debug", "%u privileged users", mPrivilegedUsers.size());
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
    return config()->getBool({"priv_rooms", "enable_priv_room"});
}

void newsoul::Newsoul::handleSignals(int signal) {
    if(signal == SIGINT) {
        NNLOG("newsoul.debug", "Got %i, stopping the reactor.", signal);
        _instance->m_Reactor->stop();
#ifndef _WIN32
    } else if(signal == SIGHUP) {
        NNLOG("newsoul.debug", "Got %i, reloading shares.", signal);
        _instance->LoadShares();
    } else if(signal == SIGALRM) {
        NNLOG("newsoul.debug", "Got %i, reconnecting.", signal);
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
