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

#include "searchmanager.h"

newsoul::SearchManager::SearchManager(Newsoul * newsoul) : m_Newsoul(newsoul)
{
    // Connect some events.
    newsoul->server()->loggedInStateChangedEvent.connect(this, &SearchManager::onServerLoggedInStateChanged);
    newsoul->server()->netInfoReceivedEvent.connect(this, &SearchManager::onNetInfoReceived);
    newsoul->server()->searchRequestedEvent.connect(this, &SearchManager::onSearchRequested);
    newsoul->server()->fileSearchRequestedEvent.connect(this, &SearchManager::onFileSearchRequested);
    newsoul->peers()->peerSocketReadyEvent.connect(this, &SearchManager::onPeerSocketReady);
    newsoul->peers()->peerSocketUnavailableEvent.connect(this, &SearchManager::onPeerSocketUnavailable);
    newsoul->server()->addUserReceivedEvent.connect(this, &SearchManager::onAddUserReceived);
    newsoul->server()->wishlistIntervalReceivedEvent.connect(this, &SearchManager::onWishlistIntervalReceived);
    //newsoul->config()->keySetEvent.connect(this, &SearchManager::onConfigKeySet);
    //newsoul->config()->keyRemovedEvent.connect(this, &SearchManager::onConfigKeyRemoved);

    m_Parent = 0;
    m_ParentIp = std::string();
    m_BranchRoot = std::string();
    m_BranchLevel = 0;
    m_TransferSpeed = 0;
    m_ChildrenMaxNumber = 3;
    m_WishlistInterval = 720; // Default wishlist interval
}

newsoul::SearchManager::~SearchManager()
{
    if (m_WishlistTimeout.isValid())
        newsoul()->reactor()->removeTimeout(m_WishlistTimeout);
    NNLOG("newsoul.peers.debug", "Search Manager destroyed");
}

/**
  * Returns the maximum depth of our tree of children
  */
uint newsoul::SearchManager::childDepth() {
    uint depth = 0;

    std::map<std::string, std::pair<NewNet::RefPtr<DistributedSocket>, uint> >::const_iterator it;
    for (it = m_Children.begin(); it != m_Children.end(); it++) {
        uint d = it->second.second + 1;
        if (d > depth)
            depth = d;
    }

    return depth;
}

/**
  * Register or update a child with the given socket and his child depth
  */
void newsoul::SearchManager::setChild(DistributedSocket * socket, uint depth) {
    if (socket) {
        bool isUpdate = m_Children.find(socket->user()) != m_Children.end();
        if ((parent() || isUpdate) && acceptChildren()) {
            NNLOG("newsoul.peers.debug", "Setting child: %s, depth %d", socket->user().c_str(), depth);

            uint oldDepth = childDepth();

            m_Children[socket->user()] = std::pair<NewNet::RefPtr<DistributedSocket>, uint>(socket, depth);
            uint newDepth = childDepth();

            if (oldDepth != newDepth) {
                NNLOG("newsoul.peers.debug", "Our child depth is now %d", newDepth);
                SChildDepth msg(newDepth);
                newsoul()->server()->sendMessage(msg.make_network_packet());

                if (parent()) {
                    DChildDepth msgP(newDepth);
                    parent()->sendMessage(msgP.make_network_packet());
                }
            }

            if (!isUpdate) {
                socket->disconnectedEvent.connect(this, &SearchManager::onChildDisconnected);
                socket->setPingTimeout(newsoul()->reactor()->addTimeout(60000, socket, &DistributedSocket::ping));

                // We have reached our maximum number of children
                if (m_Children.size() >= m_ChildrenMaxNumber) {
                    SAcceptChildren msgA(false);
                    newsoul()->server()->sendMessage(msgA.make_network_packet());
                }
            }
        }
        else {
            // We want to disconnect this socket. BUT we have to be sure that there isn't any remaining handshakesocket in the reactor
            // before doing this (If there is still a handshake socket, it will stay in the reactor even if it's a dead socket so there
            // will be problems. This happens if the peer sends us both HInitiate and DChildDepth message at once.).
            // To be sure of that, wait 1 second before disconnecting
            socket->addDisconnectNowTimeout();
        }
    }
}

/**
  * Removes a child from our tree
  */
void newsoul::SearchManager::removeChild(const std::string & user) {
    NNLOG("newsoul.peers.debug", "Removing child: %s", user.c_str());

    uint oldDepth = childDepth();

    std::map<std::string, std::pair<NewNet::RefPtr<DistributedSocket>, uint> >::iterator it = m_Children.find(user);
    if(it != m_Children.end()) {
        m_Children.erase(it);

        // If we have a new child depth, inform the server and our parent
        uint newDepth = childDepth();
        if (oldDepth != newDepth) {
            NNLOG("newsoul.peers.debug", "Our child depth is now %d", newDepth);
            SChildDepth msg(newDepth);
            newsoul()->server()->sendMessage(msg.make_network_packet());

            if (parent()) {
                DChildDepth msgP(newDepth);
                parent()->sendMessage(msgP.make_network_packet());
            }
        }

        // We have one more place for a children
        if (m_Children.size() == m_ChildrenMaxNumber - 1) {
            SAcceptChildren msgA(true);
            newsoul()->server()->sendMessage(msgA.make_network_packet());
        }
    }
}

/**
  * Register a parent we're trying to connect to.
  */
void newsoul::SearchManager::addPotentialParent(const std::string & user, DistributedSocket * socket, const std::string & ip) {
    m_PotentialParents[user].first = socket;
    m_PotentialParents[user].second = ip;
}

/**
  * We have found a parent: stop connecting to others
  */
void newsoul::SearchManager::setParent(DistributedSocket * parentSocket) {
    if (parent() && parentSocket)
        parent()->stop();

    m_Parent = parentSocket;
    if (parentSocket) {
        std::string parentName = parentSocket->user();
        NNLOG("newsoul.peers.debug", "Found a parent : %s", parentName.c_str());
        std::map<std::string, std::pair<NewNet::RefPtr<DistributedSocket>, std::string> >::iterator it;

        std::map<std::string, std::pair<NewNet::RefPtr<DistributedSocket>, std::string> > parents = m_PotentialParents;

        for (it = parents.begin(); it != parents.end(); it++) {
            if (it->first != parentName) {
                if (it->second.first.isValid())
                    it->second.first->stop();
            }
            else
                m_ParentIp = it->second.second;
            m_PotentialParents.erase(it->first);
        }

        SAcceptChildren msgAccept(true);
        newsoul()->server()->sendMessage(msgAccept.make_network_packet());

        parentSocket->disconnectedEvent.connect(this, &SearchManager::onParentDisconnected);
        parentSocket->setPingTimeout(newsoul()->reactor()->addTimeout(60000, parentSocket, &DistributedSocket::ping));
    }
}

/**
  * We have a new branch root, store it and notify the server
  */
void newsoul::SearchManager::setBranchRoot(const std::string & root) {
    NNLOG("newsoul.peers.debug", "Our branch root is %s", root.c_str());
    m_BranchRoot = root;

    // Tell our new parent how many children we've got
    DChildDepth msgD(childDepth());
    parent()->sendMessage(msgD.make_network_packet());

    // Send out parent IP to the server
    SParentIP msgIp(m_ParentIp);
    newsoul()->server()->sendMessage(msgIp.make_network_packet());
    // Send our branch level to the server
    SBranchLevel msgL(branchLevel());
    newsoul()->server()->sendMessage(msgL.make_network_packet());
    // Send the new branch root to the server
    SBranchRoot msgR(root);
    newsoul()->server()->sendMessage(msgR.make_network_packet());

    // Tell our children what we've done
    std::map<std::string, std::pair<NewNet::RefPtr<DistributedSocket>, uint> >::iterator cit;
    for (cit = m_Children.begin(); cit != m_Children.end(); cit++) {
        if (cit->second.first.isValid())
            cit->second.first->sendPosition();
    }
}

/**
  * The server sends us a parent list.
  */
void newsoul::SearchManager::onNetInfoReceived(const SNetInfo * msg) {
    if (parent())
        setParent(0);

    std::map<std::string, std::pair<std::string, uint32> >::const_iterator it;
    for (it = msg->users.begin(); it != msg->users.end(); it++) {
        NNLOG("newsoul.peers.debug", "Potential parent: %s (%s:%i)", it->first.c_str(), it->second.first.c_str(), it->second.second);

        DistributedSocket * socket = new DistributedSocket(newsoul());
        newsoul()->reactor()->add(socket);
        addPotentialParent(it->first, socket, it->second.first);
        socket->initiateActiveWithIP(it->first, it->second.first, it->second.second); // Don't ask for ip/port: we already know it
    }
}

/**
  * Called when some potential parent sends us his branch level
  */
void newsoul::SearchManager::branchLevelReceived(DistributedSocket * socket, uint level) {
    // The first potential parent who gives us his level will be our official parent.

    std::map<std::string, std::pair<NewNet::RefPtr<DistributedSocket>, std::string> >::iterator it;
    it = m_PotentialParents.find(socket->user());

    if (!parent() && it != m_PotentialParents.end()) {
        setParent(socket);
        setBranchLevel(level++);
        SHaveNoParents msg(false);
        newsoul()->server()->sendMessage(msg.make_network_packet());
    }
    else if (parent() == socket) {
        // This is an update from our parent
        setBranchLevel(level++);
    }
}

/**
  * The server sends us a search request directly
  */
void newsoul::SearchManager::onSearchRequested(const SSearchRequest * msg) {
    std::string query = newsoul()->codeset()->fromNet(msg->query);

    NNLOG("newsoul.peers.debug", "Received search request from server: %s for %s", query.c_str(), msg->username.c_str());

    transmitSearch(msg->unknown, msg->username, msg->token, msg->query);
    sendSearchResults(msg->username, query, msg->token);
}

/**
  * The server sends us a search request directly, outside the distributed system
  */
void newsoul::SearchManager::onFileSearchRequested(const SFileSearch * msg) {
    std::string query = newsoul()->codeset()->fromNet(msg->query);

    NNLOG("newsoul.peers.debug", "Received file search request from server: %s for %s", query.c_str(), msg->user.c_str());

    sendSearchResults(msg->user, query, msg->ticket);
}

/**
  * The server sends us a SAddUser response: see if there's something interesting
  */
void newsoul::SearchManager::onAddUserReceived(const SAddUser * msg) {
    if (msg->user == newsoul()->server()->username()) {
        NNLOG("newsoul.peers.debug", "Our transfer speed is %d", msg->userdata.avgspeed);
        setTransferSpeed(msg->userdata.avgspeed);
    }
}

/**
  * The server sends us the wishlist interval
  */
void newsoul::SearchManager::onWishlistIntervalReceived(const SWishlistInterval * msg) {
    NNLOG("newsoul.peers.debug", "New wishlist interval: %d", msg->value);
    m_WishlistInterval = msg->value;
    if (m_WishlistTimeout.isValid())
        newsoul()->reactor()->removeTimeout(m_WishlistTimeout);
    m_WishlistTimeout = newsoul()->reactor()->addTimeout(m_WishlistInterval*1000, this, &SearchManager::onWishlistTimeout);
}

/**
  * We can send a new wishlist request
  */
void newsoul::SearchManager::onWishlistTimeout(long) {
    if (!m_Wishlist.empty()) {
        // Which request should we do?
        std::map<std::string, time_t>::iterator oldest = m_Wishlist.begin(), it = m_Wishlist.begin();
        for (; it != m_Wishlist.end(); it++) {
            if (it->second < oldest->second)
                oldest = it;
        }
        NNLOG("newsoul.peers.debug", "Sending wishlist search for '%s'", oldest->first.c_str());
        uint token = newsoul()->token();

        newsoul()->ifaces()->sendNewSearchToAll(oldest->first, token);
        // Send query
        SWishlistSearch msg(token, newsoul()->codeset()->toNet(oldest->first));
        newsoul()->server()->sendMessage(msg.make_network_packet());
        // Update item's last query date
        newsoul()->config()->set({"wishlist", oldest->first}, static_cast<int>(time(NULL)));
    }

    // Prepare next wishlist timeout
    m_WishlistTimeout = newsoul()->reactor()->addTimeout(m_WishlistInterval*1000, this, &SearchManager::onWishlistTimeout);
}

/**
  * Send the given search request to our children
  * The query's encoding should be the network one
  */
void newsoul::SearchManager::transmitSearch(uint unknown, const std::string & username, uint ticket, const std::string & query) {
    std::map<std::string, std::pair<NewNet::RefPtr<DistributedSocket>, uint> >::const_iterator it;
    for (it = m_Children.begin(); it != m_Children.end(); it++) {
        DistributedSocket * socket = it->second.first;
        if (socket) {
            DSearchRequest msgD(unknown, username, ticket, query);
            socket->sendMessage(msgD.make_network_packet());
        }
    }
}

/**
  * Send the results from a query to the asker
  * The query's encoding should be UTF-8
  */
void newsoul::SearchManager::sendSearchResults(const std::string & username, const std::string & query, uint token) {
	if(! newsoul()->isBanned(username) && (username != newsoul()->server()->username())) {
        SharesDB *db;

        if (newsoul()->isBuddied(username))
            db = newsoul()->buddyshares();
        else
            db = newsoul()->shares();

        Folder results = db->query(query);

        if (!results.empty()) {
            m_PendingResults[username][token] = results;
            newsoul()->peers()->peerSocket(username, false);
        }
	}
}

/**
  * Initiate a search in our buddy list
  */
void newsoul::SearchManager::buddySearch(uint token, const std::string & query) {
    std::vector<std::string> buddies = newsoul()->config()->getVec({"buddies"});
    std::vector<std::string>::const_iterator it;
    std::string q = newsoul()->codeset()->toNet(query);
    for(it = buddies.begin(); it != buddies.end(); ++it) {
        SUserSearch msg(*it, token, q);
        newsoul()->server()->sendMessage(msg.make_network_packet());
    }
}

/**
  * Initiate a search in the room we joined
  */
void newsoul::SearchManager::roomsSearch(uint token, const std::string & query) {
    std::vector<std::string> rooms = newsoul()->server()->joinedRooms();
    std::vector<std::string>::const_iterator it;
    std::string q = newsoul()->codeset()->toNet(query);
    for(it = rooms.begin(); it != rooms.end(); ++it) {
        SRoomSearch msg(*it, token, q);
        newsoul()->server()->sendMessage(msg.make_network_packet());
    }
}

/**
  * Add an item to wishlist and search for it
  */
void newsoul::SearchManager::wishlistAdd(const std::string & query) {
    uint token = newsoul()->token();
    newsoul()->ifaces()->sendNewSearchToAll(query, token);

    if (m_Wishlist.find(query) != m_Wishlist.end()) {
        NNLOG("newsoul.peers.debug", "'%s' is already in the wishlist", query.c_str());
        // This item is already in wishlist, just do a normal search
        SFileSearch msg(token, newsoul()->codeset()->toNet(query));
        newsoul()->server()->sendMessage(msg.make_network_packet());
    }
    else {
        NNLOG("newsoul.peers.debug", "Adding '%s' in the wishlist", query.c_str());
        // launch a wishlist search
        SWishlistSearch msg(token, newsoul()->codeset()->toNet(query));
        newsoul()->server()->sendMessage(msg.make_network_packet());
    }
    // Update item's last query date
    newsoul()->config()->set({"wishlist", query}, static_cast<int>(time(NULL)));
}

/**
  * There's a peer socket available: see if we have something to send
  */
void newsoul::SearchManager::onPeerSocketReady(PeerSocket * socket) {
    std::string username = socket->user();

    std::map<std::string, std::map<uint, Folder> >::iterator pending = m_PendingResults.find(username);
    if (pending != m_PendingResults.end() && m_PendingResults[username].size()) {
        NNLOG("newsoul.peers.debug", "Sending search results to %s", username.c_str());

        std::map<uint, Folder>::const_iterator it;
        for (it = m_PendingResults[username].begin(); it != m_PendingResults[username].end(); it++) {
            PSearchReply msg(it->first, username, it->second, transferSpeed(), (uint64) newsoul()->uploads()->queueTotalLength(), newsoul()->uploads()->hasFreeSlots());
            socket->sendMessage(msg.make_network_packet());
        }

        m_PendingResults.erase(pending);

        // Disconnect the peer socket as it is probably no longer needed and we have a limit for opened socket
        socket->addSearchResultsOnlyTimeout(2000);
    }
}

/**
  * Our connection with our parent has been disconnected
  */
void newsoul::SearchManager::onParentDisconnected(NewNet::ClientSocket * socket_) {
    m_BranchRoot = newsoul()->server()->username();
    setBranchLevel(0);

    setParent(0);

    SAcceptChildren msgAccept(false);
    newsoul()->server()->sendMessage(msgAccept.make_network_packet());

    SChildDepth msgD(childDepth());
    newsoul()->server()->sendMessage(msgD.make_network_packet());

    SHaveNoParents msgH(true);
    newsoul()->server()->sendMessage(msgH.make_network_packet());

    SBranchLevel msgL(branchLevel());
    newsoul()->server()->sendMessage(msgL.make_network_packet());

    SBranchRoot msgR(branchRoot());
    newsoul()->server()->sendMessage(msgR.make_network_packet());
}

/**
  * Our connection with our child has been disconnected
  */
void newsoul::SearchManager::onChildDisconnected(NewNet::ClientSocket * socket_) {
    DistributedSocket * socket = (DistributedSocket *) socket_;

    NNLOG("newsoul.peers.debug", "Child %s is gone", socket->user().c_str());

    removeChild(socket->user());
}

/*
    Called when the connection to the server changes (connected/disconnected)
*/
void
newsoul::SearchManager::onServerLoggedInStateChanged(bool loggedIn)
{
    if(loggedIn) {
        uint level = branchLevel();
        uint depth = branchLevel();
        DistributedSocket * parentSocket = parent();
        std::string parentName = newsoul()->server()->username();
        if (parentSocket)
            parentName = parentSocket->user();

        NNLOG("newsoul.peers.debug", "Asking parents (level: %d, root: %s, child depth:%d)", level, parentName.c_str(), depth);

        SHaveNoParents msgNoParents(true);
        newsoul()->server()->sendMessage(msgNoParents.make_network_packet());

        SBranchLevel msgLevel(level);
        newsoul()->server()->sendMessage(msgLevel.make_network_packet());

        SBranchRoot msgRoot(parentName);
        newsoul()->server()->sendMessage(msgRoot.make_network_packet());

        SChildDepth msgDepth(depth);
        newsoul()->server()->sendMessage(msgDepth.make_network_packet());

        SAcceptChildren msgAccept(true);
        newsoul()->server()->sendMessage(msgAccept.make_network_packet());

        // We want to know our own transfer speed. Ask the server for it.
        newsoul()->peers()->requestUserData(newsoul()->server()->username());
    }
    else if(!parent()) {
        // If we're the root of our branch and we get disconnected, we should disconnect from our children to let them go on a living branch
        std::map<std::string, std::pair<NewNet::RefPtr<DistributedSocket>, uint> >::iterator it;
        for (it = m_Children.begin(); it != m_Children.end(); it++) {
            if (it->second.first)
                it->second.first->stop();
        }
        m_Children.clear();
    }
}

/*
    Called when the connection cannot be made with the peer.
*/
void
newsoul::SearchManager::onPeerSocketUnavailable(std::string user)
{
    // Could not connect to the peer or disconnected: delete the pending search results
    std::map<std::string, std::map<uint, Folder> >::iterator it;
    it = m_PendingResults.find(user);
    if (it != m_PendingResults.end())
        m_PendingResults.erase(it);
}

void
newsoul::SearchManager::searchReplyReceived(uint ticket, const std::string & user, bool slotfree, uint avgspeed, uint64 queuelen, const Folder & folders) {
    newsoul()->ifaces()->onSearchReply(ticket, user, slotfree, avgspeed, (uint) queuelen, folders);
}

//void
//newsoul::SearchManager::onConfigKeySet(const ConfigManager::ChangeNotify * data) {
    //if ((data->domain == "wishlist") && (!data->key.empty()))
        //m_Wishlist[data->key] = newsoul()->config()->getInt({data->domain, data->key});
//}

//void
//newsoul::SearchManager::onConfigKeyRemoved(const ConfigManager::RemoveNotify * data) {
    //if (data->domain == "wishlist") {
        //std::map<std::string, time_t>::iterator it = m_Wishlist.find(data->key);
        //if (it != m_Wishlist.end())
            //m_Wishlist.erase(it);
    //}
//}
