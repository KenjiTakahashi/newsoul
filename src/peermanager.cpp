/*  newsoul - A SoulSeek client written in C++
    Copyright (C) 2006-2007 Ingmar K. Steen (iksteen@gmail.com)
    Copyright 2008 little blue poney <lbponey@users.sourceforge.net>

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

#include "peermanager.h"
#include "ifacemanager.h"

newsoul::PeerManager::PeerManager(Newsoul * newsoul) : m_Newsoul(newsoul)
{
  newsoul->config()->keySetEvent.connect(this, &PeerManager::onConfigKeySet);
  newsoul->config()->keyRemovedEvent.connect(this, &PeerManager::onConfigKeyRemoved);
  newsoul->server()->loggedInStateChangedEvent.connect(this, &PeerManager::onServerLoggedInStateChanged);
  newsoul->server()->cannotConnectNotifyReceivedEvent.connect(this, &PeerManager::onCannotConnectNotify);
  newsoul->server()->connectToPeerRequestedEvent.connect(this, &PeerManager::onServerConnectToPeerRequested);
  newsoul->server()->userStatusReceivedEvent.connect(this, &PeerManager::onServerUserStatusReceived);
  newsoul->server()->addUserReceivedEvent.connect(this, &PeerManager::onServerAddUserReceived);

  firewallPiercedEvent.connect(this, &PeerManager::onFirewallPierced);

  listen();
}

newsoul::PeerManager::~PeerManager()
{
}

void
newsoul::PeerManager::unlisten()
{
    if(m_Factory.isValid()) {
        m_Factory->serverSocket()->disconnect();
        if (m_Factory->serverSocket()->reactor())
            m_Newsoul->reactor()->remove(m_Factory->serverSocket());
    }
    m_Factory = 0;
}

void
newsoul::PeerManager::listen()
{
  uint first = m_Newsoul->config()->getUint("clients.bind", "first"),
       last =  m_Newsoul->config()->getUint("clients.bind", "last");

  if((first == 0) || (first > last))
  {
    NNLOG("newsoul.peers.warn", "No valid client bind port set (range: %i - %i).", first, last);
    unlisten();
    return;
  }

  if(m_Factory.isValid())
  {
    unsigned int port = m_Factory->serverSocket()->listenPort();
    if((port >= first) && (port <= last))
      return;
    unlisten();
  }

  unsigned int port = first;
  while(port <= last)
  {
    NNLOG("newsoul.peers.debug", "Trying to bind to port %i...", port);
    m_Factory = new PeerFactory();
    m_Factory->clientAcceptedEvent.connect(this, &PeerManager::onClientAccepted);

    m_Newsoul->reactor()->add(m_Factory->serverSocket());
    m_Factory->serverSocket()->listen(port);
    if(m_Factory->serverSocket()->socketState() == NewNet::Socket::SocketListening)
    {
      onServerLoggedInStateChanged(m_Newsoul->server()->loggedIn());
      NNLOG("newsoul.peers.debug", "Listening for peers on port %i", port);
      return;
    }
    unlisten();
    ++port;
  }

  NNLOG("newsoul.peers.warn", "Couldn't find port to listen for peers on (range: %i - %i).", first, last);
}

/**
  * Returns a peersocket for the given user name.
  */
void
newsoul::PeerManager::peerSocket(const std::string & user, bool force) {
    NNLOG("newsoul.peers.debug", "Asking a peersocket for %s", user.c_str());

    // Check if this user is already registered.
    std::map<std::string, NewNet::WeakRefPtr<PeerSocket> >::iterator it;
    it = m_Peers.find(user);
    if(it == m_Peers.end() || !it->second) {
        // Nope, see if we can open a new peersocket
        int maxSocket = newsoul()->reactor()->maxSocketNo();
        int currentSockets = newsoul()->reactor()->currentSocketNo();

        if (!force && (maxSocket > 0) && (currentSockets > (maxSocket - static_cast<int>(maxSocket*0.5)))) {
            NNLOG("newsoul.peers.warn", "Too many opened peer socket, cannot open a new one with low priority");
            peerSocketUnavailableEvent(user);
            return;
        }

        // We can, register the user
        m_Peers[user] = 0;

        if(newsoul()->server()->loggedIn()) {
            std::map<std::string, uint32>::iterator it = m_UserStatus.find(user);
            if (it == m_UserStatus.end()) {
                NNLOG("newsoul.peers.debug", "No peer socket to %s, requesting status.", user.c_str());
                requestUserData(user);
            }
            else if (isUserConnected(user)) {
                // We know user is connected to server, create the socket
                createPeerSocket(user);
            }
            else {
                // User is offline, cannot create the socket
                NNLOG("newsoul.peers.debug", "User %s is offline", user.c_str());

                // Don't disconnect the peer socket if it exists: it will be disconnected if the user is completely offline.
                // If he's only disconnected from the server, we can still talk to him directly.
                peerOfflineEvent(user);
            }


        }
    }
    else
        peerSocketReadyEvent((*it).second);
}

/**
  * Ask the server to give us the given user existence, status and stats
  */
void newsoul::PeerManager::requestUserData(const std::string& user) {
    // Tell the server we want to watch this user
    // We don't need to send another SAddUser if we've just sent one
    struct timeval now;
    gettimeofday(&now, NULL);
    std::map<std::string, struct timeval >::iterator tit;
    tit = m_LastStatusTime.find(user);
    if(tit == m_LastStatusTime.end() || difftime(now, (*tit).second) > 10000.0) {
        NNLOG("newsoul.peers.debug", "Asking server for existence, status and stats of user %s", user.c_str());
        m_LastStatusTime[user] = now;

        // Let the server know we want to track the status of this user.
        SAddUser msg(user);
        newsoul()->server()->sendMessage(msg.make_network_packet());
    }
}

/**
  * Register a peer socket associated with the given user
  */
void newsoul::PeerManager::addPeerSocket(PeerSocket * socket) {
    if(socket->user() == std::string()) {
        NNLOG("newsoul.peers.warn", "Cannot add a peer socket with no user associated.");
        return;
    }

    NNLOG("newsoul.peers.debug", "Adding peer socket for %s", socket->user().c_str());

    bool isOurself = (socket->user() == newsoul()->server()->username());

    // Disconnect and remove any existing peer socket for this user except if we're connecting to ourself
    if (!isOurself)
        removePeerSocket(socket->user(), true);

    // There shouldn't be several peer sockets for the same user. If that happens, keep only the last one.
    m_Peers[socket->user()] = socket;
    if (!isOurself || (isOurself && !m_Peers[socket->user()])) {
        // Don't watch for disconnection twice if we're trying to connect to ourself
        socket->cannotConnectEvent.connect(this, &PeerManager::onPeerCannotConnect);
        socket->disconnectedEvent.connect(this, &PeerManager::onDisconnected);
    }
    if (socket->socketState() == NewNet::Socket::SocketConnected)
        peerSocketReadyEvent(socket);
    else
        socket->connectedEvent.connect(this, &PeerManager::onConnected);
}

/**
  * Remove a peer socket associated with the given user if found. If disconnect is set to true, the peer socket will be disconnected first.
  */
void newsoul::PeerManager::removePeerSocket(const std::string & user, bool disconnect) {
    std::map<std::string, NewNet::WeakRefPtr<PeerSocket> >::iterator it;
    it = m_Peers.find(user);
    if (it != m_Peers.end()) {
        if (disconnect && it->second)
            it->second->disconnect();
        else // disconnect will call removePeerSocket later with disconnect = false
            m_Peers.erase(it);
    }
}

/*
    The status of a user has changed
*/
void
newsoul::PeerManager::onServerUserStatusReceived(const SGetStatus * message)
{
    setUserStatus(message->user, message->status); // Store status for future ifaces

    // Is the user online?
    if(message->status > 0) {
        NNLOG("newsoul.peers.debug", "User %s is now online", message->user.c_str());

        createPeerSocket(message->user);
    }
    else {
        NNLOG("newsoul.peers.debug", "User %s is now offline", message->user.c_str());

        peerOfflineEvent(message->user);
    }
}

/**
  *  Set the status of an user
  */
void
newsoul::PeerManager::setUserStatus(const std::string& user, uint32 status)
{
    m_UserStatus[user] = status; // Store status for future ifaces
}

/**
  * Create a peer socket for the given user. You shouldn't call this directly, call peerSocket() first.
  */
void
newsoul::PeerManager::createPeerSocket(const std::string& user) {
    // Only create a peer socket if we asked for it earlier
    std::map<std::string, NewNet::WeakRefPtr<PeerSocket> >::iterator it;
    it = m_Peers.find(user);
    if(it != m_Peers.end()) {
        // Check if we already have a socket for this user.
        PeerSocket * socket = m_Peers[user];
        if(! socket) {
            // Nope. Create a new one.
            socket = new PeerSocket(newsoul());
            socket->setUser(user);
            addPeerSocket(socket);
            newsoul()->reactor()->add(socket);
            if (user == newsoul()->server()->username())
                socket->initiateOurself();
            else
                socket->initiate(user);
        }
    }
}

/**
  * We have received stats of an user
  */
void
newsoul::PeerManager::onServerAddUserReceived(const SAddUser * message)
{
    if (!message->exists)
        return;

    m_UserStats[message->user] = message->userdata; // Store stats for future ifaces

    setUserStatus(message->user, message->userdata.status); // Store status for future ifaces

    // Is the user online?
    if(message->userdata.status > 0) {
        NNLOG("newsoul.peers.debug", "User %s is online", message->user.c_str());

        createPeerSocket(message->user);
    }
    else {
        NNLOG("newsoul.peers.debug", "User %s is offline", message->user.c_str());

        // Don't disconnect the peer socket if it exists: it will be disconnected if the user is completely offline.
        // If he's only disconnected from the server, we can still talk to him directly.
        peerOfflineEvent(message->user);
    }
}

/**
  * Returns true if the given user is known to be online by newsoul. If the user is offline or is not known by us, returns false.
  */
bool newsoul::PeerManager::isUserConnected(const std::string& user) {
    std::map<std::string, uint32>::iterator it = m_UserStatus.find(user);
    if (it == m_UserStatus.end())
        return false;
    else
        return (it->second > 0);
}

/*
    Called when the connection cannot be made with the peer.
*/
void
newsoul::PeerManager::onPeerCannotConnect(NewNet::ClientSocket * socket_)
{
	NNLOG("newsoul.peers.debug", "Cannot connect to the peer");
    // Cast the socket to a peer socket.
    PeerSocket * socket = (PeerSocket *)socket_;
    // Get the name of the user.
    std::string user = socket->user();

    socket->disconnect();

    peerSocketUnavailableEvent(user);
}

/*
    Called when the connection cannot be made with ourself (actively 127.0.0.1:listenport).
*/
void
newsoul::PeerManager::onCannotConnectOurself(NewNet::ClientSocket * socket_) {
    // Could't connect actively, try the standard way (which needs reverse NAT)
    PeerSocket * socket = (PeerSocket *)socket_;
    if (socket) {
        socket->stopConnectOurself();
        socket->initiate(newsoul()->server()->username());
    }
}

void
newsoul::PeerManager::onCannotConnectNotify(const SCannotConnect * msg) {
	NNLOG("newsoul.peers.debug", "Cannot connect to the peer %s", msg->user.c_str());

    peerSocketUnavailableEvent(msg->user);
}

/*
    Called when a peer socket gets disconnected
*/
void
newsoul::PeerManager::onDisconnected(NewNet::ClientSocket * socket_)
{
	NNLOG("newsoul.peers.debug", "Peer socket disconnected");

    // Cast the socket to a peer socket.
    PeerSocket * socket = (PeerSocket *)socket_;

    // Get the name of the user.
    std::string user(socket->user());
    // Check if this is the most recently constructed socket.
    removePeerSocket(user);
}

void
newsoul::PeerManager::onConnected(NewNet::ClientSocket * socket_)
{
    // Cast the socket to a peer socket.
    PeerSocket * socket = (PeerSocket *)socket_;
    peerSocketReadyEvent(socket);
}

void
newsoul::PeerManager::onClientAccepted(newsoul::HandshakeSocket * socket)
{
  socket->setNewsoul(m_Newsoul);
}

void
newsoul::PeerManager::onConfigKeySet(const newsoul::ConfigManager::ChangeNotify * data)
{
  if(data->domain == "clients.bind")
  {
    listen();
  }
}

void
newsoul::PeerManager::onConfigKeyRemoved(const newsoul::ConfigManager::RemoveNotify * data)
{
  if(data->domain == "clients.bind")
  {
    listen();
  }
}

void
newsoul::PeerManager::onServerLoggedInStateChanged(bool loggedIn)
{
  if(loggedIn)
  {
    uint port = m_Factory ? m_Factory->serverSocket()->listenPort() : 0;
    m_Newsoul->server()->sendMessage(SSetListenPort(port).make_network_packet());
  }
}

void
newsoul::PeerManager::onServerConnectToPeerRequested(const SConnectToPeer * message)
{
    if (message->type == "P") {
        PeerSocket * socket = new PeerSocket(m_Newsoul);
        socket->setUser(message->user);
        addPeerSocket(socket);
        m_Newsoul->reactor()->add(socket);
        socket->reverseConnect(message->user, message->token, message->ip, message->port);
    }
    else if (message->type == "F") {
        TicketSocket * socket = new TicketSocket(m_Newsoul);
        m_Newsoul->reactor()->add(socket);
        socket->reverseConnect(message->user, message->token, message->ip, message->port);
        // There may be some data waiting in the buffer (sent at connection). We have to ask the ticketsocket to check it.
        socket->findTicket();
    }
    else if (message->type == "D") {
        // Create a new DistributedSocket which will copy our descriptor and state.
        DistributedSocket * socket = new DistributedSocket(m_Newsoul);
        // A potential parent doesn't care about our position
        if (!newsoul()->searches()->isPotentialParent(message->user))
            socket->sendPosition();
        m_Newsoul->reactor()->add(socket);
        socket->reverseConnect(message->user, message->token, message->ip, message->port);
    }
}

/**
  * Register a usersocket that is waiting for a response from a peer during a passive connection.
  */
void
newsoul::PeerManager::waitingPassiveConnection(UserSocket * socket) {
    if (socket && (socket->token() > 0)) {
        m_PassiveConnects[socket->token()] = socket;
    }
}

/**
  * Unregister a usersocket that is no longer waiting for a response from a peer during a passive connection.
  */
void
newsoul::PeerManager::removePassiveConnectionWaiting(uint token) {
    if (token > 0) {
        std::map<uint, NewNet::RefPtr<UserSocket> >::iterator it;
        it = m_PassiveConnects.find(token);
        if (it != m_PassiveConnects.end())
            m_PassiveConnects.erase(it);
    }
}

void
newsoul::PeerManager::onFirewallPierced(newsoul::HandshakeSocket * socket)
{
    std::map<uint, NewNet::RefPtr<UserSocket> >::iterator it;
    it = m_PassiveConnects.find(socket->token());
    if (it != m_PassiveConnects.end()) {
        it->second->firewallPierced(socket);
        m_PassiveConnects.erase(it);
    }
    else {
        NNLOG("newsoul.user.warn", "Received an unexpected HPierceFirewall message.");
        socket->disconnect(false);
    }
}
