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

#include "servermanager.h"

newsoul::ServerManager::ServerManager(Newsoul * newsoul) : m_Newsoul(newsoul), m_LoggedIn(false)
{
    // Connect config event handlers
    newsoul->config()->keySetEvent.connect(this, &ServerManager::onConfigKeySet);
    newsoul->config()->keyRemovedEvent.connect(this, &ServerManager::onConfigKeyRemoved);

    // Connect local event handlers
    loggedInEvent.connect(this, &ServerManager::onLoggedIn);
    roomJoinedEvent.connect(this, &ServerManager::onRoomJoined);
    roomLeftEvent.connect(this, &ServerManager::onRoomLeft);
    privilegedUsersReceivedEvent.connect(this, & ServerManager::onPrivilegedUsersReceived);
    privilegedUserAddedEvent.connect(this, & ServerManager::onPrivilegedUserAddedReceived);
    kickedEvent.connect(this, & ServerManager::onKicked);
    privateMessageReceivedEvent.connect(this, &ServerManager::onServerPrivateMessageReceived);

    m_ConnectionTries = 0;
    m_AutoConnect = true;
    mServerTimeDiff = 0.0L;
    mTestingServerTime = false;
}

newsoul::ServerManager::~ServerManager()
{
    if (m_ReconnectTimeout.isValid())
        newsoul()->reactor()->removeTimeout(m_ReconnectTimeout);
}

void
newsoul::ServerManager::connect()
{
  if(m_Socket.isValid())
  {
    NNLOG("newsoul.server.warn", "Already connected to server.");
    return;
  }

  m_LoggedIn = false;

  std::string host = newsoul()->config()->get("server", "host", "server.slsknet.org");
  unsigned int port = newsoul()->config()->getUint("server", "port", 2242);

  if(host.empty())
  {
    NNLOG("newsoul.server.warn", "No hostname for server specified.");
    return;
  }

  if(port == 0)
  {
    NNLOG("newsoul.server.warn", "No port for server specified.");
    return;
  }

  m_Username = newsoul()->config()->get("server", "username");
  m_Password = newsoul()->config()->get("server", "password");
  if(m_Username.empty())
  {
    NNLOG("newsoul.server.warn", "No username for server specified.");
    return;
  }

  // Set up auto-join
  m_JoinedRooms = newsoul()->config()->keys("autojoin");

  // Create the TcpMessageSocket
  m_Socket = new TcpMessageSocket();

  // Connect the event handlers
  m_Socket->cannotConnectEvent.connect(this, &ServerManager::onCannotConnect);
  m_Socket->connectedEvent.connect(this, &ServerManager::onConnected);
  m_Socket->disconnectedEvent.connect(this, &ServerManager::onDisconnected);
  m_Socket->messageReceivedEvent.connect(this, &ServerManager::onMessageReceived);

  // Add the socket to the reactor
  newsoul()->reactor()->add(m_Socket);

  // Reconnect automatically if problems
  m_AutoConnect = true;

  // Connect to the soulseek server
  m_Socket->connect(host, port);
}

void
newsoul::ServerManager::disconnect()
{
  // Don't try to reconnect automatically as we explicitly tell we want to stay disconnected
  m_AutoConnect = false;

  if(m_Socket.isValid())
    m_Socket->disconnect();
}

/**
  * Reconnect to the server
  */
void
newsoul::ServerManager::reconnect(long) {
    if (m_AutoConnect)
        connect();
}

void
newsoul::ServerManager::sendMessage(const NewNet::Buffer & buffer)
{
  if(! m_Socket)
  {
    NNLOG("newsoul.server.warn", "Trying to send message over closed socket...");
    return;
  }

  gettimeofday(&mLastSentMessage, 0);

  unsigned char buf[4];
  buf[0] = buffer.count() & 0xff;
  buf[1] = (buffer.count() >> 8) & 0xff;
  buf[2] = (buffer.count() >> 16) & 0xff;
  buf[3] = (buffer.count() >> 24) & 0xff;
  m_Socket->send(buf, 4);
  m_Socket->send(buffer.data(), buffer.count());
}

#define SEND_MESSAGE(m) sendMessage(m.make_network_packet())

void
newsoul::ServerManager::onCannotConnect(NewNet::ClientSocket * socket)
{
  // We'll try to reconnect in a few seconds
  bool wasAuto = m_AutoConnect;
  if (m_AutoConnect) {
      int length = 2000;
      if (m_ConnectionTries >= 2)
        length = 30000;
      if (m_ReconnectTimeout.isValid())
        newsoul()->reactor()->removeTimeout(m_ReconnectTimeout);
      m_ReconnectTimeout = newsoul()->reactor()->addTimeout(length, this, &ServerManager::reconnect);
      m_ConnectionTries++;
      NNLOG("newsoul.server.warn", "Cannot connect to server... Will reconnect in %d ms.", length);
  }
  else
      NNLOG("newsoul.server.warn", "Cannot connect to server... Will not try to reconnect.");

  setLoggedIn(false);
  disconnect();
  m_AutoConnect = wasAuto; // Restore m_AutoConnect as disconnect() will always set it to false
}

void
newsoul::ServerManager::onConnected(NewNet::ClientSocket *)
{
  NNLOG("newsoul.server.debug", "Connected to server.");
  NNLOG("newsoul.server.debug", "Sending login message.");
  SEND_MESSAGE(SLogin(m_Username, m_Password));
}

void
newsoul::ServerManager::onDisconnected(NewNet::ClientSocket * socket)
{
  // We'll try to reconnect in a few seconds
  if (m_AutoConnect) {
      int length = 2000;
      if (m_ConnectionTries >= 2)
        length = 30000;
      if (m_ReconnectTimeout.isValid())
        newsoul()->reactor()->removeTimeout(m_ReconnectTimeout);
      m_ReconnectTimeout = newsoul()->reactor()->addTimeout(length, this, &ServerManager::reconnect);
      m_ConnectionTries++;
    NNLOG("newsoul.server.debug", "Disconnected from server. Will reconnect in %d ms.", length);
  }
  else
    NNLOG("newsoul.server.debug", "Disconnected from server. Will not try to reconnect.");


  setLoggedIn(false);
  newsoul()->reactor()->remove(socket);
  newsoul()->reactor()->removeTimeout(m_PingTimeout);
}

void
newsoul::ServerManager::onMessageReceived(const TcpMessageSocket::MessageData * data)
{
  switch(data->type)
  {

    #define MAP_MESSAGE(ID, TYPE, EVENT) \
      case ID: \
      { \
        NNLOG("newsoul.messages.server", "Received server message " #TYPE "."); \
        TYPE msg; \
        msg.parse_network_packet(data->data, data->length); \
        EVENT(&msg); \
        break; \
      }
    #include "servereventtable.h"
    #undef MAP_MESSAGE

    default:
        NNLOG("newsoul.server.warn", "Received unknown server message, type: %u, length: %u", data->type, data->length);
        NetworkMessage msg;
        msg.parse_network_packet(data->data, data->length);
  }
}

void
newsoul::ServerManager::onConfigKeySet(const ConfigManager::ChangeNotify * data)
{
  if(loggedIn())
  {
    if(data->domain == "interests.like")
      SEND_MESSAGE(SInterestAdd(data->key));
    else if(data->domain == "interests.hate")
      SEND_MESSAGE(SInterestHatedAdd(data->key));
    else if(data->domain == "buddies" || data->domain == "trusted" || data->domain == "ignored" || data->domain == "banned")
      newsoul()->peers()->requestUserData(data->key);
  }
}

void
newsoul::ServerManager::onConfigKeyRemoved(const ConfigManager::RemoveNotify * data)
{
  if(loggedIn())
  {
    if(data->domain == "interests.like")
      SEND_MESSAGE(SInterestRemove(data->key));
    else if(data->domain == "interests.hate")
      SEND_MESSAGE(SInterestHatedRemove(data->key));
  }
}

void
newsoul::ServerManager::onLoggedIn(const SLogin * message)
{
  m_ConnectionTries = 0;
  if (m_ReconnectTimeout.isValid())
    newsoul()->reactor()->removeTimeout(m_ReconnectTimeout);

  m_PingTimeout = newsoul()->reactor()->addTimeout(60000, this, &ServerManager::pingServer);

  launchServerTimeTest(0.0L);
  newsoul()->reactor()->addTimeout(10000, this, &ServerManager::serverTimeTestFailed); // Server time test needs to work in the first 10 seconds.

  setLoggedIn(message->success);
  if(message->success)
  {
    newsoul()->peers()->requestUserData(username());
    std::vector<std::string>::const_iterator it, end = m_JoinedRooms.end();
    for(it = m_JoinedRooms.begin(); it != end; ++it)
      SEND_MESSAGE(SJoinRoom(*it));
    m_JoinedRooms.clear();

    std::vector<std::string> interests;
    interests = newsoul()->config()->keys("interests.like");
    end = interests.end();
    for(it = interests.begin(); it != end; ++it)
      SEND_MESSAGE(SInterestAdd(*it));
    interests = newsoul()->config()->keys("interests.hate");
    end = interests.end();
    for(it = interests.begin(); it != end; ++it)
      SEND_MESSAGE(SInterestHatedAdd(*it));

    // Get status and stats for every users we have in our lists
    std::vector<std::string> users, trusted, banned, ignored;
    users = newsoul()->config()->keys("buddies");
    trusted = newsoul()->config()->keys("trusted");
    banned = newsoul()->config()->keys("banned");
    ignored = newsoul()->config()->keys("ignored");
    users.insert(users.begin(), trusted.begin(), trusted.end());
    users.insert(users.begin(), banned.begin(), banned.end());
    users.insert(users.begin(), ignored.begin(), ignored.end());
    std::sort(users.begin(), users.end());
    std::unique(users.begin(), users.end());
    for(it = users.begin(); it != users.end(); ++it) {
      newsoul()->peers()->requestUserData(*it);
    }

    newsoul()->sendSharedNumber();

    SEND_MESSAGE(SPrivRoomToggle(newsoul()->isEnabledPrivRoom()));
  }
}

void
newsoul::ServerManager::launchServerTimeTest(long) {
    gettimeofday(&mLastServerTimeTestTime, 0);
    std::ostringstream oss;
    oss << "Testing server time " << mLastServerTimeTestTime.tv_sec << " " << mLastServerTimeTestTime.tv_usec;
    mLastServerTimeTestString = oss.str();
    SEND_MESSAGE(SPrivateMessage(username(), mLastServerTimeTestString));
    mTestingServerTime = true;
}

void
newsoul::ServerManager::serverTimeTestFailed(long) {
    if (mServerTimeDiff <= 0)
        mServerTimeDiff = 0;
    NNLOG("newsoul.server.debug", "The server time diff is considered as %ld.", mServerTimeDiff);
    //mTestingServerTime = true; // Keep it to true to avoid showing the test message if the server finally answers after a moment
    newsoul()->reactor()->addTimeout(21600000, this, &ServerManager::launchServerTimeTest); // New test in 6 hours
    receivedServerTimeDiff(mServerTimeDiff);
}

bool
newsoul::ServerManager::isServerTimeTestMessage(const std::string& user, const std::string& message) {
    return ((user == username()) && (mLastServerTimeTestString == message));
}

void
newsoul::ServerManager::onServerPrivateMessageReceived(const SPrivateMessage * message)
{
    if (mTestingServerTime && isServerTimeTestMessage(message->user, message->message)) {
        mServerTimeDiff = message->timestamp - mLastServerTimeTestTime.tv_sec;
        NNLOG("newsoul.server.debug", "The server time diff is %ld.", mServerTimeDiff);
        mTestingServerTime = false;
        SEND_MESSAGE(SAckPrivateMessage(message->ticket));
        newsoul()->reactor()->addTimeout(21600000, this, &ServerManager::launchServerTimeTest); // New test in 6 hours
        receivedServerTimeDiff(mServerTimeDiff);
    }
}

/**
  * Ping the server and launch a timer for the next ping
  */
void
newsoul::ServerManager::pingServer(long diff) {
    struct timeval now;
    gettimeofday(&now, 0);

    if (difftime(now, mLastSentMessage) > 59000) {
        // No data sent to server since 60 seconds. Ping the server
        NNLOG("newsoul.server.debug", "Pinging the server (%dms delay)", diff);
        SPing msg;
        sendMessage(msg.make_network_packet());
        m_PingTimeout = newsoul()->reactor()->addTimeout(60000, this, &ServerManager::pingServer);
    }
    else {
        // We've sent someting to the server recently. Wait 60 seconds vefore pinging.
        NNLOG("newsoul.server.debug", "Delaying server ping");
        m_PingTimeout = newsoul()->reactor()->addTimeout(60000, this, &ServerManager::pingServer);
    }
}

void
newsoul::ServerManager::onRoomJoined(const SJoinRoom * message)
{
  m_JoinedRooms.push_back(message->room);

  std::string ticker(newsoul()->config()->get("tickers", message->room));
  if(ticker.empty())
    ticker = newsoul()->config()->get("default-ticker", "ticker");
  if(! ticker.empty())
  {
    ticker = newsoul()->codeset()->toRoom(message->room, ticker);
    SEND_MESSAGE(SSetRoomTicker(message->room, ticker));
  }
}

void
newsoul::ServerManager::onRoomLeft(const SLeaveRoom * message)
{
    std::vector<std::string>::iterator it;
    it = std::find(m_JoinedRooms.begin(), m_JoinedRooms.end(), message->value);
    if (it != m_JoinedRooms.end())
        m_JoinedRooms.erase(it);
}

void
newsoul::ServerManager::onPrivilegedUsersReceived(const SPrivilegedUsers * message) {
    NNLOG("newsoul.server.debug", "Received privileged users");
    m_Newsoul->setPrivilegedUsers(message->values);
}

void
newsoul::ServerManager::onPrivilegedUserAddedReceived(const SAddPrivileged * message) {
    NNLOG("newsoul.server.debug", "Received a new privileged user");
    m_Newsoul->addPrivilegedUser(message->value);
}

/**
  * Called when newsoul is kicked from the server.
  */
void
newsoul::ServerManager::onKicked(const SKicked * message) {
    // Don't try to reconnect
    m_AutoConnect = false;
}
