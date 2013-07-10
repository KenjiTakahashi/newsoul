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

newsoul::Newsoul::Newsoul(const std::string &configPath, bool debug) {
    /* Seed the random generator and fabricate our starting token. */
    srand(time(NULL));
    m_Token = rand();

    m_Reactor = new NewNet::Reactor();

    /* Instantiate the various components. Order can be important here. */
    m_Config = new ConfigManager();
    m_Codeset = new CodesetManager(this);
    m_Server = new ServerManager(this);
    m_Peers = new PeerManager(this);
    m_Downloads = new DownloadManager(this);
    m_Uploads = new UploadManager(this);
    m_Ifaces = new IfaceManager(this);
    m_Searches = new SearchManager(this);

    if(!this->m_Config->load(configPath)) {
        NNLOG("newsoul.config.warn", "Failed to load configuration");
    }
    if(!this->m_Config->getBool("newsoul.debug", "ALL") && !debug) {
        NNLOG.disable("ALL");
    }

    this->LoadShares();
    this->LoadDownloads();
    _instance = this;
}

int newsoul::Newsoul::run(int argc, char *argv[]) {
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
    std::string shares = m_Config->get("shares", "database");
    if (!shares.empty()) {
        m_Shares = new SharesDB(shares, [this]{this->sendSharedNumber();});
    }
    std::string bshares = m_Config->get("buddy.shares", "database");
    if (!bshares.empty() && haveBuddyShares()) {
        m_BuddyShares = new SharesDB(bshares, [this]{this->sendSharedNumber();});
    }
}

void newsoul::Newsoul::LoadDownloads() {
    m_Downloads->loadDownloads();
}

newsoul::Newsoul::~Newsoul()
{
  /* This destructor doesn't do much. */
  NNLOG("newsoul.debug", "newsoul destroyed");
}

bool newsoul::Newsoul::isBanned(const std::string u) {
    return config()->hasKey("banned", u);
}

bool newsoul::Newsoul::isIgnored(const std::string u) {
    return config()->hasKey("ignored", u);
}

bool newsoul::Newsoul::isTrusted(const std::string u) {
    return config()->hasKey("trusted", u);
}

bool newsoul::Newsoul::isBuddied(const std::string u) {
    return config()->hasKey("buddies", u);
}

bool newsoul::Newsoul::isPrivileged(const std::string u) {
    return std::find(mPrivilegedUsers.begin(), mPrivilegedUsers.end(), u) != mPrivilegedUsers.end();
}

bool newsoul::Newsoul::toBuddiesOnly() {
    return config()->getBool("transfers", "only_buddies", false);
}

bool newsoul::Newsoul::haveBuddyShares() {
    return config()->getBool("transfers", "have_buddy_shares", false);
}

bool newsoul::Newsoul::trustingUploads() {
    return config()->getBool("transfers", "trusting_uploads", false);
}

bool newsoul::Newsoul::autoClearFinishedDownloads() {
    return config()->getBool("transfers", "autoclear_finished_downloads", false);
}

bool newsoul::Newsoul::autoRetryDownloads() {
    return config()->getBool("transfers", "autoretry_downloads", false);
}

bool newsoul::Newsoul::autoClearFinishedUploads() {
    return config()->getBool("transfers", "autoclear_finished_uploads", false);
}

bool newsoul::Newsoul::privilegeBuddies() {
    return config()->getBool("transfers", "privilege_buddies", false);
}

uint newsoul::Newsoul::upSlots() {
    return config()->getUint("transfers", "upload_slots", 0);
}

uint newsoul::Newsoul::downSlots() {
    return config()->getUint("transfers", "download_slots", 0);
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
        unsigned int numFiles = m_Shares->filesCount();
        unsigned int numFolders = m_Shares->dirsCount();
        if(haveBuddyShares()) {
            numFiles += m_BuddyShares->filesCount();
            numFolders += m_BuddyShares->dirsCount();
        }
	    SSharedFoldersFiles msg(numFolders, numFiles);
	    server()->sendMessage(msg.make_network_packet());
	}
}

bool newsoul::Newsoul::isEnabledPrivRoom() {
    return config()->getBool("priv_rooms", "enable_priv_room", false);
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
