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

#include "ifacemanager.h"

#define SEND_MESSAGE(SOCKET, MESSAGE) (SOCKET)->sendMessage(MESSAGE.make_network_packet())
#define SEND_ALL(MESSAGE) \
  do { \
    NewNet::Buffer buffer(MESSAGE.make_network_packet()); \
    std::vector<NewNet::RefPtr<newsoul::IfaceSocket> >::iterator it, end = m_Ifaces.end(); \
    for(it = m_Ifaces.begin(); it != end; ++it) \
      if((*it)->authenticated()) \
        (*it)->sendMessage(buffer); \
  } while(0)
#define SEND_MASK(MASK, MESSAGE) \
  do { \
    NewNet::Buffer buffer(MESSAGE.make_network_packet()); \
    std::vector<NewNet::RefPtr<newsoul::IfaceSocket> >::iterator it, end = m_Ifaces.end(); \
    for(it = m_Ifaces.begin(); it != end; ++it) \
      if((*it)->authenticated() && ((*it)->mask() & MASK)) \
        (*it)->sendMessage(buffer); \
  } while(0)
#define SEND_C_MASK(MASK, MESSAGE) \
  do { \
    std::vector<NewNet::RefPtr<newsoul::IfaceSocket> >::iterator it, end = m_Ifaces.end(); \
    for(it = m_Ifaces.begin(); it != end; ++it) \
      if((*it)->authenticated() && ((*it)->mask() & MASK)) \
        (*it)->sendMessage(MESSAGE.make_network_packet()); \
  } while(0)

static char challengemap[] = "0123456789abcdef";
static std::string challenge()
{
  std::string r;
  for(int i = 0; i < 64; i++)
    r += challengemap[rand() % 16];
  return r;
}

newsoul::IfaceManager::IfaceManager(Newsoul * newsoul) : m_Newsoul(newsoul)
{
  m_AwayState = 0;
  m_ReceivedTimeDiff = false;

  NNLOG.logEvent.connect(this, &IfaceManager::onLog);

  newsoul->server()->loggedInStateChangedEvent.connect(this, &IfaceManager::onServerLoggedInStateChanged);
  newsoul->server()->receivedServerTimeDiff.connect(this, &IfaceManager::onServerTimeDiffReceived);
  newsoul->server()->loggedInEvent.connect(this, &IfaceManager::onServerLoggedIn);
  newsoul->server()->kickedEvent.connect(this, &IfaceManager::onServerKicked);
  newsoul->server()->peerAddressReceivedEvent.connect(this, &IfaceManager::onServerPeerAddressReceived);
  newsoul->server()->addUserReceivedEvent.connect(this, &IfaceManager::onServerAddUserReceived);
  newsoul->server()->userStatusReceivedEvent.connect(this, &IfaceManager::onServerUserStatusReceived);
  newsoul->server()->privateMessageReceivedEvent.connect(this, &IfaceManager::onServerPrivateMessageReceived);
  newsoul->server()->roomMessageReceivedEvent.connect(this, &IfaceManager::onServerRoomMessageReceived);
  newsoul->server()->roomJoinedEvent.connect(this, &IfaceManager::onServerRoomJoined);
  newsoul->server()->roomLeftEvent.connect(this, &IfaceManager::onServerRoomLeft);
  newsoul->server()->userJoinedRoomEvent.connect(this, &IfaceManager::onServerUserJoinedRoom);
  newsoul->server()->userLeftRoomEvent.connect(this, &IfaceManager::onServerUserLeftRoom);
  newsoul->server()->roomListReceivedEvent.connect(this, &IfaceManager::onServerRoomListReceived);
  newsoul->server()->privilegesReceivedEvent.connect(this, &IfaceManager::onServerPrivilegesReceived);
  newsoul->server()->roomTickersReceivedEvent.connect(this, &IfaceManager::onServerRoomTickersReceived);
  newsoul->server()->roomTickerAddedEvent.connect(this, &IfaceManager::onServerRoomTickerAdded);
  newsoul->server()->roomTickerRemovedEvent.connect(this, &IfaceManager::onServerRoomTickerRemoved);
  newsoul->server()->recommendationsReceivedEvent.connect(this, &IfaceManager::onServerRecommendationsReceived);
  newsoul->server()->globalRecommendationsReceivedEvent.connect(this, &IfaceManager::onServerGlobalRecommendationsReceived);
  newsoul->server()->similarUsersReceivedEvent.connect(this, &IfaceManager::onServerSimilarUsersReceived);
  newsoul->server()->itemRecommendationsReceivedEvent.connect(this, &IfaceManager::onServerItemRecommendationsReceived);
  newsoul->server()->itemSimilarUsersReceivedEvent.connect(this, &IfaceManager::onServerItemSimilarUsersReceived);
  newsoul->server()->userInterestsReceivedEvent.connect(this, &IfaceManager::onServerUserInterestsReceived);
  newsoul->server()->newPasswordReceivedEvent.connect(this, &IfaceManager::onServerNewPasswordSet);
  newsoul->server()->publicChatReceivedEvent.connect(this, &IfaceManager::onServerPublicChatReceived);

  newsoul->server()->privRoomToggleReceivedEvent.connect(this, &IfaceManager::onServerPrivRoomToggled);
  newsoul->server()->privRoomAlterableMembersReceivedEvent.connect(this, &IfaceManager::onServerPrivRoomAlterableMembers);
  newsoul->server()->privRoomAlterableOperatorsReceivedEvent.connect(this, &IfaceManager::onServerPrivRoomAlterableOperators);
  newsoul->server()->privRoomAddedUserEvent.connect(this, &IfaceManager::onServerPrivRoomAddedUser);
  newsoul->server()->privRoomRemovedUserEvent.connect(this, &IfaceManager::onServerPrivRoomRemovedUser);
  newsoul->server()->privRoomAddedOperatorEvent.connect(this, &IfaceManager::onServerPrivRoomAddedOperator);
  newsoul->server()->privRoomRemovedOperatorEvent.connect(this, &IfaceManager::onServerPrivRoomRemovedOperator);

  newsoul->downloads()->downloadAddedEvent.connect(this, &IfaceManager::onDownloadUpdated);
  newsoul->downloads()->downloadUpdatedEvent.connect(this, &IfaceManager::onDownloadUpdated);
  newsoul->downloads()->downloadRemovedEvent.connect(this, &IfaceManager::onDownloadRemoved);

  newsoul->uploads()->uploadAddedEvent.connect(this, &IfaceManager::onUploadUpdated);
  newsoul->uploads()->uploadUpdatedEvent.connect(this, &IfaceManager::onUploadUpdated);
  newsoul->uploads()->uploadRemovedEvent.connect(this, &IfaceManager::onUploadRemoved);

  newsoul->peers()->peerSocketUnavailableEvent.connect(this, &IfaceManager::onPeerSocketUnavailable);
  newsoul->peers()->peerSocketReadyEvent.connect(this, &IfaceManager::onPeerSocketReady);

  for(const std::string &listener : this->m_Newsoul->config()->getVec({"listeners", "paths"})) {
      if(m_Factories.find(listener) == m_Factories.end()) {
          this->addListener(listener);
          SEND_C_MASK(EM_CONFIG, IConfigSet((*it)->cipherContext(), "listeners", listener, ""));
      }
  }
}

bool
newsoul::IfaceManager::addListener(const std::string & path)
{
  if(path.empty())
    return false;

#ifndef WIN32 // No unix sockets on win32
  if(path[0] == '/')
  {
    NewNet::RefPtr<NewNet::UnixFactorySocket<IfaceSocket> > factory;
    factory = new NewNet::UnixFactorySocket<IfaceSocket>;
    factory->clientAcceptedEvent.connect(this, &IfaceManager::onIfaceAccepted);
    factory->serverSocket()->listen(path);
    if(factory->serverSocket()->socketState() != NewNet::Socket::SocketListening)
    {
      NNLOG("newsoul.iface.warn", "Couldn't listen on unix path '%s'.", path.c_str());
      return false;
    }
    m_Factories[path] = factory;
    m_ServerSockets[path] = factory->serverSocket();
    newsoul()->reactor()->add(factory->serverSocket());
  }
  else
#endif // WIN32
  {
    std::string::size_type ix = path.find(':');
    if(ix == std::string::npos)
    {
      NNLOG("newsoul.iface.warn", "Invalid TCP description: '%s'.", path.c_str());
      return false;
    }
    std::string host = path.substr(0, ix);
    unsigned int port = atol(path.substr(ix + 1).c_str());
    NewNet::RefPtr<NewNet::TcpFactorySocket<IfaceSocket> > factory;
    factory = new NewNet::TcpFactorySocket<IfaceSocket>;
    factory->clientAcceptedEvent.connect(this, &IfaceManager::onIfaceAccepted);
    factory->serverSocket()->listen(host, port);
    if(factory->serverSocket()->socketState() != NewNet::Socket::SocketListening)
    {
      NNLOG("newsoul.iface.warn", "Couldn't listen on '%s:%u'", host.c_str(), port);
      return false;
    }
    m_Factories[path] = factory;
    m_ServerSockets[path] = factory->serverSocket();
    newsoul()->reactor()->add(factory->serverSocket());
  }

  NNLOG("newsoul.iface.debug", "Listening on '%s'.", path.c_str());
  return true;
}

void
newsoul::IfaceManager::removeListener(const std::string & path)
{
  std::map<std::string, NewNet::RefPtr<NewNet::ServerSocket> >::iterator it;
  it = m_ServerSockets.find(path);
  if(it == m_ServerSockets.end())
    return;
  (*it).second->disconnect();
  newsoul()->reactor()->remove((*it).second);
  m_ServerSockets.erase(it);
  std::map<std::string, NewNet::RefPtr<NewNet::Object> >::iterator fit = m_Factories.find(path);
  if (fit != m_Factories.end())
    m_Factories.erase(fit);
}

void
newsoul::IfaceManager::onLog(const NewNet::Log::LogNotify * log)
{
  SEND_MASK(EM_DEBUG, IDebugMessage(log->domain, log->message));
}

//void
//newsoul::IfaceManager::onConfigKeySet(const ConfigManager::ChangeNotify * data)
//{
  //if(data->domain == "interfaces.bind")
  //{
    //if(m_Factories.find(data->key) != m_Factories.end())
      //return;
    //addListener(data->key);
  //}
  //SEND_C_MASK(EM_CONFIG, IConfigSet((*it)->cipherContext(), data->domain, data->key, data->value));
//}

//void
//newsoul::IfaceManager::onConfigKeyRemoved(const ConfigManager::RemoveNotify * data)
//{
  //if(data->domain == "interfaces.bind")
  //{
    //if(m_Factories.find(data->key) == m_Factories.end())
      //return;
    //removeListener(data->key);
  //}
  //SEND_C_MASK(EM_CONFIG, IConfigRemove((*it)->cipherContext(), data->domain, data->key));
//}

void
newsoul::IfaceManager::flushPrivateMessages()
{
  if(m_PrivateMessages.empty() || m_Ifaces.empty() || !m_ReceivedTimeDiff)
    return;

  bool sent = false;
  std::vector<PrivateMessage>::iterator it, end = m_PrivateMessages.end();

  std::vector<NewNet::RefPtr<IfaceSocket> >::iterator iit, iend = m_Ifaces.end();
  for(iit = m_Ifaces.begin(); iit != iend; ++iit)
  {
    if((*iit)->mask() & EM_PRIVATE)
    {
      sent = true;
      for(it = m_PrivateMessages.begin(); it != end; ++it)
        SEND_MESSAGE(*iit, IPrivateMessage(0, (*it).timestamp, (*it).user, (*it).message));
    }
  }

  if(! sent)
    return;

  for(it = m_PrivateMessages.begin(); it != end; ++it)
    SEND_MESSAGE(newsoul()->server(), SAckPrivateMessage((*it).ticket));

  m_PrivateMessages.clear();
}

void
newsoul::IfaceManager::onServerTimeDiffReceived(long diff) {
    if (!m_ReceivedTimeDiff) { // First time received
        std::vector<PrivateMessage>::iterator it, end = m_PrivateMessages.end();
        for(it = m_PrivateMessages.begin(); it != end; ++it) {
            it->timestamp = it->timestamp - newsoul()->server()->getServerTimeDiff();
        }
    }

    m_ReceivedTimeDiff = true;
    flushPrivateMessages();
}

void
newsoul::IfaceManager::sendNewSearchToAll(const std::string & query, uint token) {
    SEND_ALL(ISearch(query, token));
}

void
newsoul::IfaceManager::onIfaceAccepted(IfaceSocket * socket)
{
  NNLOG("newsoul.iface.debug", "Accepted new interface socket.");
  m_Ifaces.push_back(socket);

  // Connect the events
  socket->disconnectedEvent.connect(this, &IfaceManager::onIfaceDisconnected);
  socket->pingEvent.connect(this, &IfaceManager::onIfacePing);
  socket->loginEvent.connect(this, &IfaceManager::onIfaceLogin);
  socket->checkPrivilegesEvent.connect(this, &IfaceManager::onIfaceCheckPrivileges);
  socket->setStatusEvent.connect(this, &IfaceManager::onIfaceSetStatus);
  socket->newPasswordEvent.connect(this, &IfaceManager::onIfaceNewPassword);
  socket->setConfigEvent.connect(this, &IfaceManager::onIfaceSetConfig);
  socket->removeConfigEvent.connect(this, &IfaceManager::onIfaceRemoveConfig);
  socket->setUserImageEvent.connect(this, &IfaceManager::onIfaceSetUserImage);
  socket->getPeerExistsEvent.connect(this, &IfaceManager::onIfaceGetPeerExists);
  socket->getPeerStatusEvent.connect(this, &IfaceManager::onIfaceGetPeerStatus);
  socket->getPeerStatsEvent.connect(this, &IfaceManager::onIfaceGetPeerStats);
  socket->getUserInfoEvent.connect(this, &IfaceManager::onIfaceGetUserInfo);
  socket->getUserInterestsEvent.connect(this, &IfaceManager::onIfaceGetUserInterests);
  socket->getUserSharesEvent.connect(this, &IfaceManager::onIfaceGetUserShares);
  socket->getPeerAddressEvent.connect(this, &IfaceManager::onIfaceGetPeerAddress);
  socket->givePrivilegesEvent.connect(this, &IfaceManager::onIfaceGivePrivileges);
  socket->sendPrivateMessageEvent.connect(this, &IfaceManager::onIfaceSendPrivateMessage);
  socket->getRoomListEvent.connect(this, &IfaceManager::onIfaceGetRoomList);
  socket->joinRoomEvent.connect(this, &IfaceManager::onIfaceJoinRoom);
  socket->leaveRoomEvent.connect(this, &IfaceManager::onIfaceLeaveRoom);
  socket->sayRoomEvent.connect(this, &IfaceManager::onIfaceSayRoom);
  socket->setRoomTickerEvent.connect(this, &IfaceManager::onIfaceSetRoomTicker);
  socket->messageUsersEvent.connect(this, &IfaceManager::onIfaceMessageUsers);
  socket->messageBuddiesEvent.connect(this, &IfaceManager::onIfaceMessageBuddies);
  socket->messageDownloadingEvent.connect(this, &IfaceManager::onIfaceMessageDownloading);
  socket->askPublicChatEvent.connect(this, &IfaceManager::onIfaceAskPublicChat);
  socket->stopPublicChatEvent.connect(this, &IfaceManager::onIfaceStopPublicChat);
  socket->privRoomToggleEvent.connect(this, &IfaceManager::onIfacePrivRoomToggle);
  socket->privRoomAddUserEvent.connect(this, &IfaceManager::onIfacePrivRoomAddUser);
  socket->privRoomRemoveUserEvent.connect(this, &IfaceManager::onIfacePrivRoomRemoveUser);
  socket->privRoomAddOperatorEvent.connect(this, &IfaceManager::onIfacePrivRoomAddOperator);
  socket->privRoomRemoveOperatorEvent.connect(this, &IfaceManager::onIfacePrivRoomRemoveOperator);
  socket->privRoomDismemberEvent.connect(this, &IfaceManager::onIfacePrivRoomDismember);
  socket->privRoomDisownEvent.connect(this, &IfaceManager::onIfacePrivRoomDisown);

  socket->startGlobalSearchEvent.connect(this, &IfaceManager::onIfaceStartSearch);
  socket->startUserSearchEvent.connect(this, &IfaceManager::onIfaceStartUserSearch);
  socket->startWishListSearchEvent.connect(this, &IfaceManager::onIfaceStartWishListSearch);
  socket->getRecommendationsEvent.connect(this, &IfaceManager::onIfaceGetRecommendations);
  socket->getGlobalRecommendationsEvent.connect(this, &IfaceManager::onIfaceGetGlobalRecommendations);
  socket->getSimilarUsersEvent.connect(this, &IfaceManager::onIfaceGetSimilarUsers);
  socket->getItemRecommendationsEvent.connect(this, &IfaceManager::onIfaceGetItemRecommendations);
  socket->getItemSimilarUsersEvent.connect(this, &IfaceManager::onIfaceGetItemSimilarUsers);
  socket->addInterestEvent.connect(this, &IfaceManager::onIfaceAddInterest);
  socket->removeInterestEvent.connect(this, &IfaceManager::onIfaceRemoveInterest);
  socket->addHatedInterestEvent.connect(this, &IfaceManager::onIfaceAddHatedInterest);
  socket->removeHatedInterestEvent.connect(this, &IfaceManager::onIfaceRemoveHatedInterest);
  socket->addWishItemEvent.connect(this, &IfaceManager::onIfaceAddWishItem);
  socket->removeWishItemEvent.connect(this, &IfaceManager::onIfaceRemoveWishItem);
  socket->connectToServerEvent.connect(this, &IfaceManager::onIfaceConnectToServer);
  socket->disconnectFromServerEvent.connect(this, &IfaceManager::onIfaceDisconnectFromServer);
  socket->reloadSharesEvent.connect(this, &IfaceManager::onIfaceReloadShares);
  socket->downloadFileEvent.connect(this, &IfaceManager::onIfaceDownloadFile);
  socket->downloadFileToEvent.connect(this, &IfaceManager::onIfaceDownloadFileTo);
  socket->downloadFolderEvent.connect(this, &IfaceManager::onIfaceDownloadFolder);
  socket->downloadFolderToEvent.connect(this, &IfaceManager::onIfaceDownloadFolderTo);
  socket->updateTransferEvent.connect(this, &IfaceManager::onIfaceUpdateTransfer);
  socket->removeTransferEvent.connect(this, &IfaceManager::onIfaceRemoveTransfer);
  socket->abortTransferEvent.connect(this, &IfaceManager::onIfaceAbortTransfer);
  socket->uploadFolderEvent.connect(this, &IfaceManager::onIfaceUploadFolder);
  socket->uploadFileEvent.connect(this, &IfaceManager::onIfaceUploadFile);

  // Send the login challenge
  socket->setChallenge(challenge());
  SEND_MESSAGE(socket, IChallenge(4, socket->challenge()));
}

void
newsoul::IfaceManager::onIfaceDisconnected(NewNet::ClientSocket * socket)
{
    std::vector<NewNet::RefPtr<IfaceSocket> >::iterator it;
    it = std::find(m_Ifaces.begin(), m_Ifaces.end(), static_cast<IfaceSocket *>(socket));
    if (it != m_Ifaces.end())
        m_Ifaces.erase(it);
    if(socket->reactor())
        socket->reactor()->remove(socket);
}

void
newsoul::IfaceManager::onIfacePing(const IPing * message)
{
  SEND_MESSAGE(message->ifaceSocket(), IPing(message->id));
}

void
newsoul::IfaceManager::onIfaceLogin(const ILogin * message)
{
  std::string password = newsoul()->config()->getStr({"listeners", "password"});
  if(password.empty())
  {
    NNLOG("newsoul.iface.warn", "Rejecting login attempt because of empty password.");
    message->ifaceSocket()->setChallenge(challenge());
    SEND_MESSAGE(message->ifaceSocket(), ILogin(false, "INVPASS", message->ifaceSocket()->challenge()));
    return;
  }

  std::string ch = message->ifaceSocket()->challenge() + password;

  unsigned char digest[32];
  uint digest_len = 0;

  if(message->algorithm == "SHA1")
  {
    shaBlock((unsigned char *)ch.data(), ch.size(), digest);
    digest_len = 20;
  }
  else if(message->algorithm == "SHA256")
  {
    sha256Block((unsigned char *)ch.data(), ch.size(), digest);
    digest_len = 32;
  }
  else if(message->algorithm == "MD5")
  {
    md5Block((unsigned char *)ch.data(), ch.size(), digest);
    digest_len = 16;
  }
  else
  {
    NNLOG("newsoul.iface.warn", "Rejected login attempt because of unknown hash algorithm.");
    message->ifaceSocket()->setChallenge(challenge());
    SEND_MESSAGE(message->ifaceSocket(), ILogin(false, "INVHASH", message->ifaceSocket()->challenge()));
    return;
  }

  char hexdigest[65];
  hexDigest(digest, digest_len, hexdigest);
  if(message->chresponse != hexdigest)
  {
    NNLOG("newsoul.iface.warn", "Rejected login attempt because of incorrect password.");
    message->ifaceSocket()->setChallenge(challenge());
    SEND_MESSAGE(message->ifaceSocket(), ILogin(false, "INVPASS", message->ifaceSocket()->challenge()));
  }
  else
  {
    NNLOG("newsoul.iface.debug", "Interface successfully logged in.");
    IfaceSocket * socket = message->ifaceSocket();
    socket->setAuthenticated(true);
    socket->setMask(message->mask);
    socket->setCipherKey(password);
    SEND_MESSAGE(socket, ILogin(true, std::string(), std::string()));
    SEND_MESSAGE(socket, IServerState(newsoul()->server()->loggedIn(), newsoul()->server()->username()));
    if(socket->mask() & EM_CHAT) {
      SEND_MESSAGE(socket, IRoomStateCompat(m_RoomList, m_RoomData, m_TickerData)); // For compatibility with old clients (deprecated since 0.3)
      SEND_MESSAGE(socket, IRoomList(m_RoomList));
      SEND_MESSAGE(socket, IPrivRoomList(m_PrivRoomList));
      SEND_MESSAGE(socket, IRoomMembers(m_RoomData, m_PrivRoomOperators, m_PrivRoomOwners));
      SEND_MESSAGE(socket, IRoomsTickers(m_TickerData));
      SEND_MESSAGE(socket, IPrivRoomToggle(newsoul()->isEnabledPrivRoom()));

      std::map<std::string, std::vector<std::string> >::iterator altMembIt = m_PrivRoomAlterableMembers.begin();
      std::map<std::string, std::vector<std::string> >::iterator altOpIt = m_PrivRoomAlterableOperators.begin();
      for (; altMembIt != m_PrivRoomAlterableMembers.end(); altMembIt++) {
        SEND_MESSAGE(socket, IPrivRoomAlterableMembers(altMembIt->first, altMembIt->second));
      }
      for (; altOpIt != m_PrivRoomAlterableOperators.end(); altOpIt++) {
        SEND_MESSAGE(socket, IPrivRoomAlterableOperators(altOpIt->first, altOpIt->second));
      }
    }
    if(socket->mask() & EM_TRANSFERS) {
      SEND_MESSAGE(socket, ITransferState(&newsoul()->downloads()->downloads()));
      SEND_MESSAGE(socket, ITransferState(&newsoul()->uploads()->uploads()));
    }
    if(socket->mask() & EM_PRIVATE)
      flushPrivateMessages();
    if(socket->mask() & EM_CONFIG)
        //SEND_MESSAGE(socket, IConfigState(socket->cipherContext(), newsoul()->config())); //FIXME
    if(newsoul()->server()->loggedIn())
      SEND_MESSAGE(message->ifaceSocket(), ISetStatus(m_AwayState));

    if (socket->mask() & EM_USERINFO) {
        // send peers stats
        // copy the maps to avoid invalid iterator when they are modified elsewhere
        std::map<std::string, UserData> userStats(*(newsoul()->peers()->userStats()));
        std::map<std::string, UserData>::const_iterator it = userStats.begin();
        for(; it != userStats.end(); it++) {
            SEND_MESSAGE(socket, IPeerStats(it->first, it->second));
        }

        std::map<std::string, uint32> userStatus(*(newsoul()->peers()->userStatus()));
        std::map<std::string, uint32>::const_iterator sit = userStatus.begin();
        for(; sit != userStatus.end(); sit++) {
            SEND_MESSAGE(socket, IPeerStatus(sit->first, sit->second));
        }
    }
  }
}

void
newsoul::IfaceManager::onIfaceCheckPrivileges(const ICheckPrivileges * message)
{
  SEND_MESSAGE(newsoul()->server(), SCheckPrivileges());
}

void
newsoul::IfaceManager::onIfaceSetStatus(const ISetStatus * message)
{
  SEND_MESSAGE(newsoul()->server(), SSetStatus(message->status ? 1 : 2));
  // Send it without waiting for a response from the server.
  // Before 157, server used to send us SGetStatus after changing our status.
  // Starting from 157, it doesn't anymore when switching back to online.
  m_AwayState = message->status;
  SEND_ALL(ISetStatus(m_AwayState));
  newsoul()->peers()->setUserStatus(newsoul()->server()->username(), message->status ? 1 : 2);
  SEND_ALL(IPeerStatus(newsoul()->server()->username(), message->status ? 1 : 2));
}

void
newsoul::IfaceManager::onIfaceNewPassword(const INewPassword * message)
{
  SEND_MESSAGE(newsoul()->server(), SNewPassword(message->newPass));
}

void
newsoul::IfaceManager::onIfaceSetConfig(const IConfigSet * message)
{
  newsoul()->config()->set({message->domain, message->key}, message->value);
}

void
newsoul::IfaceManager::onIfaceRemoveConfig(const IConfigRemove * message)
{
  newsoul()->config()->del({message->domain}, message->key);
}

void
newsoul::IfaceManager::onIfaceSetUserImage(const IConfigSetUserImage * message) {
    std::string path = newsoul()->config()->getStr({"info", "image"});

    std::ofstream ofs(path.c_str(), std::ofstream::binary | std::ofstream::trunc);
    if (!ofs.good())
        return;

    for (uint i =0; i < message->mData.size(); i++) {
        ofs.put(static_cast<const char>(message->mData[i]));
    }

    ofs.close();
}

void
newsoul::IfaceManager::onIfaceGetPeerExists(const IPeerExists * message)
{
    std::map<std::string, uint32> userStatus(*(newsoul()->peers()->userStatus()));
    std::map<std::string, uint32>::iterator it = userStatus.find(message->user);
    if (it == userStatus.end())
        newsoul()->peers()->requestUserData(message->user); // Maybe the user doesn't exist, maybe we haven't seen him yet. Ask the server
    else
        SEND_ALL(IPeerExists(message->user, true)); // We're sure he exists
}

void
newsoul::IfaceManager::onIfaceGetPeerStatus(const IPeerStatus * message)
{
    std::map<std::string, uint32> userStatus(*(newsoul()->peers()->userStatus()));
    std::map<std::string, uint32>::iterator it = userStatus.find(message->user);
    if (it == userStatus.end())
        newsoul()->peers()->requestUserData(message->user);
    else
        SEND_ALL(IPeerStatus(message->user, it->second));
}

void
newsoul::IfaceManager::onIfaceGetPeerStats(const IPeerStats * message)
{
    std::map<std::string, UserData> userStats(*(newsoul()->peers()->userStats()));
    std::map<std::string, UserData>::iterator it = userStats.find(message->user);
    if (it == userStats.end())
        newsoul()->peers()->requestUserData(message->user);
    else
        SEND_ALL(IPeerStats(message->user, it->second));
}

void
newsoul::IfaceManager::onIfaceGetUserInfo(const IUserInfo * message)
{
  m_PendingInfo[message->user].push_back(message->ifaceSocket());
  if (std::find(m_PendingInfoWaiting.begin(), m_PendingInfoWaiting.end(), message->user) == m_PendingInfoWaiting.end())
    m_PendingInfoWaiting.push_back(message->user);

  newsoul()->peers()->peerSocket(message->user);
}

void
newsoul::IfaceManager::onIfaceGetUserInterests(const IUserInterests * message)
{
  // Check the user's Interests
  m_Newsoul->server()->sendMessage(SUserInterests(message->user).make_network_packet());
}

void
newsoul::IfaceManager::onIfaceGetUserShares(const IUserShares * message)
{
  m_PendingShares[message->user].push_back(message->ifaceSocket());
  if (std::find(m_PendingSharesWaiting.begin(), m_PendingSharesWaiting.end(), message->user) == m_PendingSharesWaiting.end())
    m_PendingSharesWaiting.push_back(message->user);

  newsoul()->peers()->peerSocket(message->user);
}

void
newsoul::IfaceManager::onIfaceGetPeerAddress(const IPeerAddress * message)
{
  SEND_MESSAGE(newsoul()->server(), SGetPeerAddress(message->user));
}

void
newsoul::IfaceManager::onIfaceGivePrivileges(const IGivePrivileges * message)
{
  SEND_MESSAGE(newsoul()->server(), SGivePrivileges(message->user, message->days));
}

void
newsoul::IfaceManager::onIfaceSendPrivateMessage(const IPrivateMessage * message)
{
  std::string line = newsoul()->codeset()->toPeer(message->user, message->msg);

  // send one message per line
  std::vector<std::string> lines = string::split(line, "\n");

  std::vector<std::string>::const_iterator it;
  for (it = lines.begin(); it != lines.end(); ++it) {
    SEND_MESSAGE(newsoul()->server(), SPrivateMessage(message->user, *it));
  }

  // send one message per line
  std::vector<std::string> ilines = string::split(message->msg, "\n");

  std::vector<std::string>::const_iterator iit;
  for (iit = ilines.begin(); iit != ilines.end(); ++iit) {
    IPrivateMessage msg(1, time(NULL), message->user, *iit);

    const NewNet::Buffer & buffer = msg.make_network_packet();
    std::vector<NewNet::RefPtr<newsoul::IfaceSocket> >::iterator fit;
    for(fit = m_Ifaces.begin(); fit != m_Ifaces.end(); ++fit) {
    if((*fit)->authenticated() && ((*fit)->mask() & EM_PRIVATE) && ((*fit) != message->ifaceSocket()))
      (*fit)->sendMessage(buffer);
    }
  }
}

void
newsoul::IfaceManager::onIfaceGetRoomList(const IRoomList * message)
{
  SEND_MESSAGE(newsoul()->server(), SRoomList());
}

void
newsoul::IfaceManager::onIfaceJoinRoom(const IJoinRoom * message)
{
  SEND_MESSAGE(newsoul()->server(), SJoinRoom(message->room, message->priv));
}

void
newsoul::IfaceManager::onIfaceLeaveRoom(const ILeaveRoom * message)
{
  SEND_MESSAGE(newsoul()->server(), SLeaveRoom(message->room));
}

void
newsoul::IfaceManager::onIfaceSayRoom(const ISayRoom * message)
{
  std::string line = newsoul()->codeset()->toRoom(message->room, message->line);

  // send one message per line
  std::vector<std::string> lines = string::split(line, "\n");

  std::vector<std::string>::const_iterator it;
  for (it = lines.begin(); it != lines.end(); ++it) {
    SEND_MESSAGE(newsoul()->server(), SSayRoom(message->room, *it));
  }
}

void
newsoul::IfaceManager::onIfaceSetRoomTicker(const IRoomTickerSet * message)
{
  std::string ticker = newsoul()->codeset()->toRoom(message->room, message->message);
  ticker = string::replace(ticker, '\n', ' ');
  SEND_MESSAGE(newsoul()->server(), SSetRoomTicker(message->room, ticker));
}

void
newsoul::IfaceManager::onIfaceMessageUsers(const IMessageUsers * message)
{
    SEND_MESSAGE(newsoul()->server(), SMessageUsers(message->users, message->msg));
    std::string userList = "";
    std::vector<std::string>::const_iterator it = message->users.begin();
    for (; it != message->users.end(); it++) {
        if (it != message->users.begin())
            userList = userList + std::string(", ");
        userList = userList + *it;
    }
    sendStatusMessage(true, std::string("Sent message '") + message->msg + std::string("' to users: ") + userList);
}

void
newsoul::IfaceManager::onIfaceMessageBuddies(const IMessageBuddies * message)
{
    std::vector<std::string> buddies = newsoul()->config()->getVec({"users", "buddies"});
    SEND_MESSAGE(newsoul()->server(), SMessageUsers(buddies, message->msg));
    sendStatusMessage(true, std::string("Sent message '") + message->msg + std::string("' to buddies."));
}

void
newsoul::IfaceManager::onIfaceMessageDownloading(const IMessageDownloading * message)
{
    std::vector<std::string> users = newsoul()->uploads()->getAllUsersWithUpload();
    SEND_MESSAGE(newsoul()->server(), SMessageUsers(users, message->msg));
    sendStatusMessage(true, std::string("Sent message '") + message->msg + std::string("' to downloading users."));
}

void
newsoul::IfaceManager::onIfaceAskPublicChat(const IAskPublicChat * message)
{
  SEND_MESSAGE(newsoul()->server(), SAskPublicChat());
  SEND_MASK(EM_CHAT, IAskPublicChat());
}

void
newsoul::IfaceManager::onIfaceStopPublicChat(const IStopPublicChat * message)
{
  SEND_MESSAGE(newsoul()->server(), SStopPublicChat());
  SEND_MASK(EM_CHAT, IStopPublicChat());
}

void
newsoul::IfaceManager::onIfacePrivRoomToggle(const IPrivRoomToggle * message)
{
    bool oldConfig = newsoul()->isEnabledPrivRoom();
    if (oldConfig == message->enabled)
        return;

    newsoul()->config()->set({"privateRooms", "enabled"}, message->enabled);
    SEND_MASK(EM_CHAT, IPrivRoomToggle(message->enabled)); // Needed as the server doesn't always confirm
    SEND_MESSAGE(newsoul()->server(), SPrivRoomToggle(message->enabled));
}

void
newsoul::IfaceManager::onIfacePrivRoomAddUser(const IPrivRoomAddUser * message)
{
  SEND_MESSAGE(newsoul()->server(), SPrivRoomAddUser(message->room, message->user));
}

void
newsoul::IfaceManager::onIfacePrivRoomRemoveUser(const IPrivRoomRemoveUser * message)
{
  SEND_MESSAGE(newsoul()->server(), SPrivRoomRemoveUser(message->room, message->user));
}

void
newsoul::IfaceManager::onIfacePrivRoomAddOperator(const IPrivRoomAddOperator * message)
{
  SEND_MESSAGE(newsoul()->server(), SPrivRoomAddOperator(message->room, message->user));
}

void
newsoul::IfaceManager::onIfacePrivRoomRemoveOperator(const IPrivRoomRemoveOperator * message)
{
  SEND_MESSAGE(newsoul()->server(), SPrivRoomRemoveOperator(message->room, message->user));
}

void
newsoul::IfaceManager::onIfacePrivRoomDismember(const IPrivRoomDismember * message) {
    SEND_MESSAGE(newsoul()->server(), SPrivRoomDismember(message->room));
}

void
newsoul::IfaceManager::onIfacePrivRoomDisown(const IPrivRoomDisown * message) {
    SEND_MESSAGE(newsoul()->server(), SPrivRoomDisown(message->room));
}

void
newsoul::IfaceManager::onIfaceStartSearch(const ISearch * message)
{
    uint token = newsoul()->token();

    if (message->type == 0) // Global search
        SEND_MESSAGE(newsoul()->server(), SFileSearch(token, newsoul()->codeset()->toNet(message->query)));
    else if (message->type == 1) // Buddies search
        newsoul()->searches()->buddySearch(token, message->query);
    else if (message->type == 2) // Room search
        newsoul()->searches()->roomsSearch(token, message->query);

    sendNewSearchToAll(message->query, token);
}

void
newsoul::IfaceManager::onIfaceStartUserSearch(const IUserSearch * message)
{
    uint token = newsoul()->token();

    SEND_MESSAGE(newsoul()->server(), SUserSearch(message->user, token, newsoul()->codeset()->toNet(message->query)));

    sendNewSearchToAll(message->query, token);
}

void
newsoul::IfaceManager::onIfaceStartWishListSearch(const IWishListSearch * message) {
    newsoul()->searches()->wishlistAdd(message->query);
}

void
newsoul::IfaceManager::onIfaceGetRecommendations(const IGetRecommendations *)
{
  SEND_MESSAGE(newsoul()->server(), SGetRecommendations());
}

void
newsoul::IfaceManager::onIfaceGetGlobalRecommendations(const IGetGlobalRecommendations *)
{
  SEND_MESSAGE(newsoul()->server(), SGetGlobalRecommendations());
}

void
newsoul::IfaceManager::onIfaceGetSimilarUsers(const IGetSimilarUsers *)
{
  SEND_MESSAGE(newsoul()->server(), SGetSimilarUsers());
}

void
newsoul::IfaceManager::onIfaceGetItemRecommendations(const IGetItemRecommendations * message)
{
  SEND_MESSAGE(newsoul()->server(), SGetItemRecommendations(message->item));
}

void
newsoul::IfaceManager::onIfaceGetItemSimilarUsers(const IGetItemSimilarUsers * message)
{
  SEND_MESSAGE(newsoul()->server(), SGetItemSimilarUsers(message->item));
}

void
newsoul::IfaceManager::onIfaceAddInterest(const IAddInterest * message)
{
  newsoul()->config()->add({"info", "interests", "like"}, message->interest);
}

void
newsoul::IfaceManager::onIfaceRemoveInterest(const IRemoveInterest * message)
{
  newsoul()->config()->del({"info", "interests", "like"}, message->interest);
}

void
newsoul::IfaceManager::onIfaceAddHatedInterest(const IAddHatedInterest * message)
{
  newsoul()->config()->add({"info", "interests", "hate"}, message->interest);
}

void
newsoul::IfaceManager::onIfaceRemoveHatedInterest(const IRemoveHatedInterest * message)
{
  newsoul()->config()->del({"infO", "interests", "hate"}, message->interest);
}

void
newsoul::IfaceManager::onIfaceAddWishItem(const IAddWishItem * message)
{
  newsoul()->config()->set({"downloads", "wishes", message->query}, 0);
}

void
newsoul::IfaceManager::onIfaceRemoveWishItem(const IRemoveWishItem * message)
{
  newsoul()->config()->del({"downloads", "wishes"}, message->query);
}

void
newsoul::IfaceManager::onIfaceConnectToServer(const IConnectServer * message)
{
  newsoul()->server()->connect();
}

void
newsoul::IfaceManager::onIfaceDisconnectFromServer(const IDisconnectServer * message)
{
  newsoul()->server()->disconnect();
}


void
newsoul::IfaceManager::onIfaceReloadShares(const IReloadShares * message)
{
  newsoul()->LoadShares();
}

void
newsoul::IfaceManager::onIfaceDownloadFile(const IDownloadFile * message)
{
    newsoul()->downloads()->add(message->user, message->path);
    Download * newDownload = newsoul()->downloads()->findDownload(message->user, message->path);
    if (newDownload)
        newDownload->setSize(message->size);
}

void
newsoul::IfaceManager::onIfaceDownloadFileTo(const IDownloadFileTo * message)
{
    newsoul()->downloads()->add(message->user, message->path, newsoul()->codeset()->fromUtf8ToFS(message->localpath));
    Download * newDownload = newsoul()->downloads()->findDownload(message->user, message->path);
    if (newDownload)
        newDownload->setSize(message->size);
}

void
newsoul::IfaceManager::onIfaceDownloadFolder(const IDownloadFolder * message)
{
    newsoul()->downloads()->addFolder(message->user, message->folder);
}

void
newsoul::IfaceManager::onIfaceDownloadFolderTo(const IDownloadFolderTo * message)
{
    newsoul()->downloads()->addFolder(message->user, message->folder, newsoul()->codeset()->fromUtf8ToFS(message->localpath));
}

void
newsoul::IfaceManager::onIfaceUpdateTransfer(const ITransferUpdate * message)
{
    newsoul()->downloads()->update(message->user, message->path);
    newsoul()->uploads()->update(message->user, newsoul()->codeset()->fromUtf8ToFS(message->path));
}

void
newsoul::IfaceManager::onIfaceRemoveTransfer(const ITransferRemove * message)
{
    if(! message->upload)
        newsoul()->downloads()->remove(message->user, message->path);
    else {
        newsoul()->uploads()->remove(message->user, newsoul()->codeset()->fromUtf8ToFS(message->path));
    }
}

void
newsoul::IfaceManager::onIfaceAbortTransfer(const ITransferAbort * message)
{
    if(! message->upload)
        newsoul()->downloads()->abort(message->user, message->path);
    else
        newsoul()->uploads()->abort(message->user, newsoul()->codeset()->fromUtf8ToFS(message->path));
}

void
newsoul::IfaceManager::onIfaceUploadFolder(const IUploadFolder * message) {
    std::string user = message->user;
    std::string path = message->path;
    newsoul()->uploads()->addFolder(user, path);
}

void
newsoul::IfaceManager::onIfaceUploadFile(const IUploadFile * message)
{
    std::string user = message->user;
    std::string path = newsoul()->codeset()->fromUtf8ToFS(message->path);
    std::string pathInDb = newsoul()->codeset()->toNet(message->path);
    std::string error;

    if (newsoul()->uploads()->isUploadable(user, pathInDb, &error))
        newsoul()->uploads()->add(user, path, 0, false, true);
}

void
newsoul::IfaceManager::onServerLoggedInStateChanged(bool loggedIn)
{
  m_AwayState = 0;
  SEND_ALL(IServerState(loggedIn, newsoul()->server()->username()));
  if(loggedIn) {
    SEND_ALL(ISetStatus(m_AwayState));
    sendStatusMessage(true, std::string("Connected to the server"));
  }
  else
  {
    m_AwayState = 0;
    m_RoomList.clear();
    m_PrivRoomList.clear();
    m_RoomData.clear();
    m_PrivRoomOwners.clear();
    m_PrivRoomOperators.clear();
    m_TickerData.clear();
    m_PrivateMessages.clear();
    sendStatusMessage(true, std::string("Disconnected from the server"));
  }
}

void
newsoul::IfaceManager::onServerKicked(const SKicked* message) {
    sendStatusMessage(true, std::string("Kicked from soulseek server"));

    PrivateMessage msg;
    msg.ticket = 0;
    msg.timestamp = time(NULL);
    msg.user = "Server";
    msg.message = "You've been kicked from soulseek server. Maybe you tried to launch several newsoul instances. Or you did something *wrong*. Try reconnecting in a moment (usually >30min).";
    m_PrivateMessages.push_back(msg);
    flushPrivateMessages();
}

void
newsoul::IfaceManager::onServerPeerAddressReceived(const SGetPeerAddress * message)
{
  SEND_ALL(IPeerAddress(message->user, message->ip, message->port));
}

void
newsoul::IfaceManager::onServerAddUserReceived(const SAddUser * message)
{
  SEND_ALL(IPeerExists(message->user, message->exists));

  if (!message->exists)
    return;

  std::map<std::string, RoomData>::iterator it, end = m_RoomData.end();
  for(it = m_RoomData.begin(); it != end; ++it)
  {
    RoomData::iterator u_it = (*it).second.find(message->user);
    if(u_it == (*it).second.end())
      continue;
    (*u_it).second = message->userdata;
  }
  SEND_ALL(IPeerStats(message->user, message->userdata));
  SEND_ALL(IPeerStatus(message->user, message->userdata.status));
}

void
newsoul::IfaceManager::onServerUserStatusReceived(const SGetStatus * message)
{
  std::map<std::string, RoomData>::iterator it, end = m_RoomData.end();
  for(it = m_RoomData.begin(); it != end; ++it)
  {
    RoomData::iterator u_it = (*it).second.find(message->user);
    if(u_it == (*it).second.end())
      continue;
    (*u_it).second.status = message->status;
  }
  SEND_ALL(IPeerStatus(message->user, message->status));
  if(message->user == newsoul()->server()->username())
  {
    m_AwayState = message->status & 1;
    SEND_ALL(ISetStatus(m_AwayState));
  }
}

void
newsoul::IfaceManager::onServerPrivateMessageReceived(const SPrivateMessage * message)
{
    if (!newsoul()->isIgnored(message->user) && !newsoul()->server()->isServerTimeTestMessage(message->user, message->message)) {
        PrivateMessage msg;
        msg.ticket = message->ticket;
        msg.timestamp = message->timestamp - newsoul()->server()->getServerTimeDiff(); // Server's timestamps are wrong
        msg.user = message->user;
        msg.message = newsoul()->codeset()->fromPeer(msg.user, message->message);
        m_PrivateMessages.push_back(msg);
        flushPrivateMessages();
    }
}

void
newsoul::IfaceManager::onServerRoomMessageReceived(const SSayRoom * message)
{
    if (!newsoul()->isIgnored(message->user)) {
        std::string line = newsoul()->codeset()->fromRoom(message->room, message->line);
        SEND_MASK(EM_CHAT, ISayRoom(message->room, message->user, line));
    }
}

void
newsoul::IfaceManager::onServerRoomJoined(const SJoinRoom * message)
{
    if(m_RoomData.find(message->room) != m_RoomData.end())
        return;

    m_RoomData[message->room] = message->users;
    m_TickerData[message->room];
    if (message->isPrivate) {
        m_PrivRoomOwners[message->room] = message->owner;
        m_PrivRoomOperators[message->room] = message->ops;
    }

    SEND_MASK(EM_CHAT, IJoinRoom(message->room, message->users, message->owner, message->ops));
}

void
newsoul::IfaceManager::onServerRoomLeft(const SLeaveRoom * message)
{
    std::map<std::string, RoomData>::iterator it = m_RoomData.find(message->value);
    if(it != m_RoomData.end())
        m_RoomData.erase(it);

    PrivRoomOperators::iterator opit = m_PrivRoomOperators.find(message->value);
    if(opit != m_PrivRoomOperators.end())
        m_PrivRoomOperators.erase(opit);

    PrivRoomOwners::iterator wit = m_PrivRoomOwners.find(message->value);
    if(wit != m_PrivRoomOwners.end())
        m_PrivRoomOwners.erase(wit);

    std::map<std::string, Tickers>::iterator tit = m_TickerData.find(message->value);
    if (tit != m_TickerData.end())
        m_TickerData.erase(tit);

    SEND_MASK(EM_CHAT, ILeaveRoom(message->value));
}

void
newsoul::IfaceManager::onServerUserJoinedRoom(const SUserJoinedRoom * message)
{
  std::map<std::string, RoomData>::iterator it = m_RoomData.find(message->room);
  if(it == m_RoomData.end())
    return;
  RoomData::iterator it2 = (*it).second.find(message->user);
  if(it2 != (*it).second.end())
    return;
  (*it).second[message->user] = message->userdata;
  SEND_MASK(EM_CHAT, IUserJoinedRoom(message->room, message->user, message->userdata));
}

void
newsoul::IfaceManager::onServerUserLeftRoom(const SUserLeftRoom * message)
{
  std::map<std::string, RoomData>::iterator it = m_RoomData.find(message->room);
  if(it == m_RoomData.end())
    return;
  RoomData::iterator it2 = (*it).second.find(message->user);
  if(it2 == (*it).second.end())
    return;
  (*it).second.erase(it2);
  SEND_MASK(EM_CHAT, IUserLeftRoom(message->room, message->user));
}

void
newsoul::IfaceManager::onServerRoomListReceived(const SRoomList * message)
{
  m_RoomList = message->roomlist;
  m_PrivRoomList = message->privroomlist;
  SEND_MASK(EM_CHAT, IRoomList(message->roomlist));
  SEND_MASK(EM_CHAT, IPrivRoomList(message->privroomlist));
}

void
newsoul::IfaceManager::onServerPrivilegesReceived(const SCheckPrivileges * message)
{
  SEND_ALL(ICheckPrivileges(message->time_left));
}

void
newsoul::IfaceManager::onServerRoomTickersReceived(const SRoomTickers * message)
{
  if(m_TickerData.find(message->room) == m_TickerData.end())
    return;
  m_TickerData[message->room] = newsoul()->codeset()->fromRoomMap(message->room, message->tickers);
  SEND_MASK(EM_CHAT, IRoomTickers(message->room, m_TickerData[message->room]));
}

void
newsoul::IfaceManager::onServerRoomTickerAdded(const SRoomTickerAdd * message)
{
  if(m_TickerData.find(message->room) == m_TickerData.end())
    return;
  std::string ticker = newsoul()->codeset()->fromRoom(message->room, message->ticker);
  m_TickerData[message->room][message->user] = ticker;
  SEND_MASK(EM_CHAT, IRoomTickerSet(message->room, message->user, ticker));
}

void
newsoul::IfaceManager::onServerNewPasswordSet(const SNewPassword * message) {
    SEND_C_MASK(EM_CONFIG, INewPassword((*it)->cipherContext(), message->newPassword));
}

void
newsoul::IfaceManager::onServerPublicChatReceived(const SPublicChat * message) {
    SEND_MASK(EM_CHAT, IPublicChat(message->room, message->user, message->message));
}

void
newsoul::IfaceManager::onServerPrivRoomToggled(const SPrivRoomToggle * message)
{
    SEND_MASK(EM_CHAT, IPrivRoomToggle(message->enabled));

    bool oldConfig = newsoul()->isEnabledPrivRoom();
    if (oldConfig == message->enabled)
        return;

    newsoul()->config()->set({"privateRooms", "enabled"}, message->enabled);
}

void
newsoul::IfaceManager::onServerPrivRoomAlterableMembers(const SPrivRoomAlterableMembers * message)
{
    m_PrivRoomAlterableMembers[message->room] = message->users;

    SEND_MASK(EM_CHAT, IPrivRoomAlterableMembers(message->room, message->users));
}

void
newsoul::IfaceManager::onServerPrivRoomAddedUser(const SPrivRoomAddUser * message)
{
    m_PrivRoomAlterableMembers[message->room].push_back(message->user);

    SEND_MASK(EM_CHAT, IPrivRoomAddUser(message->room, message->user));
}

void
newsoul::IfaceManager::onServerPrivRoomRemovedUser(const SPrivRoomRemoveUser * message)
{

    std::vector<std::string>::iterator it = std::find(m_PrivRoomAlterableMembers[message->room].begin(), m_PrivRoomAlterableMembers[message->room].end(), message->user);
    if (it != m_PrivRoomAlterableMembers[message->room].end()) {
        m_PrivRoomAlterableMembers[message->room].erase(it);

        SEND_MASK(EM_CHAT, IPrivRoomRemoveUser(message->room, message->user));
    }
}

void
newsoul::IfaceManager::onServerPrivRoomAddedOperator(const SPrivRoomAddOperator * message)
{
    m_PrivRoomAlterableOperators[message->room].push_back(message->op);

    SEND_MASK(EM_CHAT, IPrivRoomAddOperator(message->room, message->op));
}

void
newsoul::IfaceManager::onServerPrivRoomRemovedOperator(const SPrivRoomRemoveOperator * message)
{

    std::vector<std::string>::iterator it = std::find(m_PrivRoomAlterableOperators[message->room].begin(), m_PrivRoomAlterableOperators[message->room].end(), message->op);
    if (it != m_PrivRoomAlterableOperators[message->room].end()) {
        m_PrivRoomAlterableOperators[message->room].erase(it);

        SEND_MASK(EM_CHAT, IPrivRoomRemoveOperator(message->room, message->op));
    }
}

void
newsoul::IfaceManager::onServerPrivRoomAlterableOperators(const SPrivRoomAlterableOperators * message)
{
    m_PrivRoomAlterableOperators[message->room] = message->ops;

    SEND_MASK(EM_CHAT, IPrivRoomAlterableOperators(message->room, message->ops));
}

void
newsoul::IfaceManager::onServerRoomTickerRemoved(const SRoomTickerRemove * message)
{
  std::map<std::string, Tickers>::iterator it = m_TickerData.find(message->room);
  if(it == m_TickerData.end())
    return;
  Tickers::iterator it2 = (*it).second.find(message->user);
  if(it2 == (*it).second.end())
    return;
  (*it).second.erase(it2);
  SEND_MASK(EM_CHAT, IRoomTickerSet(message->room, message->user, std::string()));
}

void
newsoul::IfaceManager::onServerRecommendationsReceived(const SGetRecommendations * message)
{
  SEND_MASK(EM_INTERESTS, IGetRecommendations(message->recommendations));
}

void
newsoul::IfaceManager::onServerGlobalRecommendationsReceived(const SGetGlobalRecommendations * message)
{
  SEND_MASK(EM_INTERESTS, IGetGlobalRecommendations(message->recommendations));
}

void
newsoul::IfaceManager::onServerSimilarUsersReceived(const SGetSimilarUsers * message)
{
    SEND_MASK(EM_INTERESTS, IGetSimilarUsers(message->users));
    SimilarUsers::const_iterator it = message->users.begin();
    for(; it != message->users.end(); ++it) {
        SEND_MESSAGE(newsoul()->server(), SAddUser(it->first));
    }
}

void
newsoul::IfaceManager::onServerItemRecommendationsReceived(const SGetItemRecommendations * message)
{
  SEND_MASK(EM_INTERESTS, IGetItemRecommendations(message->item, message->recommendations));
}

void
newsoul::IfaceManager::onServerItemSimilarUsersReceived(const SGetItemSimilarUsers * message)
{
  SEND_MASK(EM_INTERESTS, IGetItemSimilarUsers(message->item, message->users));
}


void
newsoul::IfaceManager::onServerUserInterestsReceived(const SUserInterests * message)
{
  NNLOG("newsoul.iface.debug", "%s has %d likes and %d hates", message->user.c_str(), message->likes.size(), message->hates.size());
  SEND_MASK(EM_USERINFO, IUserInterests(message->user, message->likes, message->hates));
}

void
newsoul::IfaceManager::onPeerSocketUnavailable(std::string user)
{
  std::map<std::string, std::vector<NewNet::WeakRefPtr<IfaceSocket> > >::iterator it;
  it = m_PendingInfo.find(user);
  std::vector<std::string>::iterator wit = std::find(m_PendingInfoWaiting.begin(), m_PendingInfoWaiting.end(), user);
  if(it != m_PendingInfo.end())
    m_PendingInfo.erase(it);
  if (wit != m_PendingInfoWaiting.end())
    m_PendingInfoWaiting.erase(wit);

  it = m_PendingShares.find(user);
  wit = std::find(m_PendingSharesWaiting.begin(), m_PendingSharesWaiting.end(), user);
  if(it != m_PendingShares.end())
    m_PendingShares.erase(it);
  if (wit != m_PendingSharesWaiting.end())
    m_PendingSharesWaiting.erase(wit);
}

void
newsoul::IfaceManager::onPeerSocketReady(PeerSocket * socket)
{
    std::vector<std::string>::iterator iit = std::find(m_PendingInfoWaiting.begin(), m_PendingInfoWaiting.end(), socket->user());
    if (iit != m_PendingInfoWaiting.end()) {
        socket->infoReceivedEvent.connect(this, &IfaceManager::onPeerInfoReceived);
        PInfoRequest msgI;
        socket->sendMessage(msgI.make_network_packet());
        m_PendingInfoWaiting.erase(iit);
    }

    std::vector<std::string>::iterator sit = std::find(m_PendingSharesWaiting.begin(), m_PendingSharesWaiting.end(), socket->user());
    if (sit != m_PendingSharesWaiting.end()) {
        socket->sharesReceivedEvent.connect(this, &IfaceManager::onPeerSharesReceived);
        PSharesRequest msgS;
        socket->sendMessage(msgS.make_network_packet());
        m_PendingSharesWaiting.erase(sit);
    }
}

void
newsoul::IfaceManager::onPeerInfoReceived(const PInfoReply * message)
{
    PeerSocket * socket = message->peerSocket();
    std::map<std::string, std::vector<NewNet::WeakRefPtr<IfaceSocket> > >::iterator it;
    std::vector<NewNet::WeakRefPtr<IfaceSocket> >::iterator fit;
    it = m_PendingInfo.find(socket->user());
    if (it != m_PendingInfo.end()) {
        for (fit = it->second.begin(); fit != it->second.end(); fit++) {
            if (fit->isValid()) {
                IUserInfo msg(socket->user(), newsoul()->codeset()->fromPeer(socket->user(), message->description), message->picture, message->totalupl, message->queuesize, message->slotfree);
                (*fit)->sendMessage(msg.make_network_packet());
            }
        }
    }

    if (it != m_PendingInfo.end())
        m_PendingInfo.erase(it);

    // Delete also from waiting (in case it isn't already done)
    std::vector<std::string>::iterator sit = std::find(m_PendingInfoWaiting.begin(), m_PendingInfoWaiting.end(), socket->user());
    if (sit != m_PendingInfoWaiting.end())
        m_PendingInfoWaiting.erase(sit);
}

void
newsoul::IfaceManager::onPeerSharesReceived(const PSharesReply * message)
{
    PeerSocket * socket = message->peerSocket();

    // We need to convert the shares with correct encoding
    Shares oriShares = message->shares;
    Shares encShares;
    Shares::iterator itFold;
    Folder::iterator itFile;
    for(itFold = oriShares.begin(); itFold != oriShares.end(); ++itFold) {
        Folder newFold;
        for(itFile = itFold->second.begin(); itFile != itFold->second.end(); ++itFile) {
            newFold[newsoul()->codeset()->fromPeer(socket->user(), itFile->first)] = itFile->second;
        }
        encShares[newsoul()->codeset()->fromPeer(socket->user(), itFold->first)] = newFold;
    }

    std::map<std::string, std::vector<NewNet::WeakRefPtr<IfaceSocket> > >::iterator it;
    std::vector<NewNet::WeakRefPtr<IfaceSocket> >::iterator fit;
    it = m_PendingShares.find(socket->user());
    if (it != m_PendingShares.end()) {
        for (fit = it->second.begin(); fit != it->second.end(); fit++) {
            if (fit->isValid()) {
                IUserShares msg(socket->user(), encShares);
                (*fit)->sendMessage(msg.make_network_packet());
            }
        }
    }

    if (it != m_PendingShares.end())
        m_PendingShares.erase(it);

    // Delete also from waiting (in case it isn't already done)
    std::vector<std::string>::iterator sit = std::find(m_PendingSharesWaiting.begin(), m_PendingSharesWaiting.end(), socket->user());
    if (sit != m_PendingSharesWaiting.end())
        m_PendingSharesWaiting.erase(sit);
}

void
newsoul::IfaceManager::onServerLoggedIn(const SLogin * message)
{

  if(message->success)
  {
    sendStatusMessage(false, message->greet);
  }
}

void
newsoul::IfaceManager::sendStatusMessage(bool type, std::string message)
{
  SEND_ALL(IStatusMessage(type, message));
}

void
newsoul::IfaceManager::onDownloadUpdated(Download * download)
{
  SEND_MASK(EM_TRANSFERS, ITransferUpdate(download));
}

void
newsoul::IfaceManager::onDownloadRemoved(Download * download)
{
  SEND_MASK(EM_TRANSFERS, ITransferRemove(false, download->user(), download->remotePath()));
}

void
newsoul::IfaceManager::onUploadUpdated(Upload * upload)
{
  SEND_MASK(EM_TRANSFERS, ITransferUpdate(upload));
}

void
newsoul::IfaceManager::onUploadRemoved(Upload * upload)
{
    SEND_MASK(EM_TRANSFERS, ITransferRemove(true, upload->user(), newsoul()->codeset()->fromFsToUtf8(upload->localPath())));
}

void
newsoul::IfaceManager::onSearchReply(uint ticket, const std::string & user, bool slotfree, uint avgspeed, uint queuelen, const Folder & folders)
{
  SEND_ALL(ISearchReply(ticket, user, slotfree, avgspeed, queuelen, folders));
}
