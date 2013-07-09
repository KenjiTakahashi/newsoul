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

#include "usersocket.h"
#include "servermanager.h"

newsoul::UserSocket::UserSocket(newsoul::Newsoul * newsoul, const std::string & type) : NewNet::TcpClientSocket(), m_Newsoul(newsoul), m_Type(type)
{
  disconnectedEvent.connect(this, &UserSocket::onDisconnected);
  m_Newsoul->server()->cannotConnectNotifyReceivedEvent.connect(this, &UserSocket::onCannotConnectNotify);
}

newsoul::UserSocket::UserSocket(newsoul::HandshakeSocket * that, const std::string & type) : NewNet::TcpClientSocket(), m_Newsoul(that->newsoul()), m_Type(type)
{
  disconnectedEvent.connect(this, &UserSocket::onDisconnected);
  m_Newsoul->server()->cannotConnectNotifyReceivedEvent.connect(this, &UserSocket::onCannotConnectNotify);

  m_Token = that->token();
  m_User = that->user();

  setDescriptor(that->descriptor());
  setSocketState(SocketConnected);
  receiveBuffer() = that->receiveBuffer();
}

newsoul::UserSocket::~UserSocket()
{
    if(m_PassiveConnectTimeout.isValid())
      m_Newsoul->reactor()->removeTimeout(m_PassiveConnectTimeout);
}

void
newsoul::UserSocket::onDisconnected(NewNet::ClientSocket *)
{
  // Just in case this socket is still registered
  m_Newsoul->peers()->removePassiveConnectionWaiting(m_Token);

  if(reactor())
    reactor()->remove(this);
}

void
newsoul::UserSocket::initiate(const std::string & user)
{
  m_User = user;
  m_Token = m_Newsoul->token();

  if(m_Newsoul->config()->get("clients", "connectmode", "active") == "passive")
    initiatePassive();
  else
    initiateActive();
}

void
newsoul::UserSocket::initiateActive()
{
  NNLOG("newsoul.user.debug", "Initiating active user connection to %s (type %s).", m_User.c_str(), m_Type.c_str());

  HInitiate handshake(m_Newsoul->server()->username(), m_Type, m_Token);
  sendMessage(handshake.make_network_packet());

  m_Newsoul->server()->peerAddressReceivedEvent.connect(this, &UserSocket::onServerPeerAddressReceived);
  SGetPeerAddress msg(m_User);
  m_Newsoul->server()->sendMessage(msg.make_network_packet());
}

void
newsoul::UserSocket::initiatePassive()
{
  NNLOG("newsoul.user.debug", "Initiating passive user connection to %s (type %s).", m_User.c_str(), m_Type.c_str());

  m_PassiveConnectTimeout = m_Newsoul->reactor()->addTimeout(60000, this, &UserSocket::onFirewallPierceTimedOut);
  m_Newsoul->peers()->waitingPassiveConnection(this);

  SConnectToPeer msg(m_Token, m_User, m_Type);
  m_Newsoul->server()->sendMessage(msg.make_network_packet());
}

void
newsoul::UserSocket::firewallPierced(newsoul::HandshakeSocket * socket)
{
    NNLOG("newsoul.user.debug", "%s's firewall successfully pierced.", m_User.c_str());
    if(m_PassiveConnectTimeout.isValid())
      m_Newsoul->reactor()->removeTimeout(m_PassiveConnectTimeout);

    setSocketState(SocketConnected);
    setDescriptor(socket->descriptor());
    receiveBuffer() = socket->receiveBuffer();
    if(! receiveBuffer().empty())
        dataReceivedEvent(this);
    socket->receiveBuffer().clear();
    connectedEvent(this);
}

void
newsoul::UserSocket::onFirewallPierceTimedOut(long)
{
  m_Newsoul->peers()->removePassiveConnectionWaiting(m_Token);

  if(socketState() == SocketUninitialized)
  {
    NNLOG("newsoul.user.debug", "Passive connection failed: pierce firewall timed out. Trying an active connection.");
    initiateActive();
  }
}

void
newsoul::UserSocket::onCannotConnectNotify(const SCannotConnect * msg)
{
    m_Newsoul->peers()->removePassiveConnectionWaiting(m_Token);

    if (msg->token == token()) {
        NNLOG("newsoul.user.debug", "Cannot connect to the peer.");
        disconnect();
    }
}

void
newsoul::UserSocket::onServerPeerAddressReceived(const SGetPeerAddress * message)
{
  if((message->user != m_User) || (socketState() != SocketUninitialized))
    return;
  NNLOG("newsoul.user.debug", "Received address of user %s: %s:%u", m_User.c_str(), message->ip.c_str(), message->port);
  if((message->ip == "0.0.0.0") || (message->port == 0))
  {
    cannotConnectEvent(this);
    return;
  }
  connect(message->ip, message->port);
}

/**
  * This is called when a peer asks us to connect to him.
  */
void
newsoul::UserSocket::reverseConnect(const std::string & user, uint token, const std::string & ip, uint port)
{
  NNLOG("newsoul.user.debug", "Trying to reverse connect to %s at %s:%u, token=%u", user.c_str(), ip.c_str(), port, token);

  cannotConnectEvent.connect(this, &UserSocket::onCannotReverseConnect);

  m_User = user;
  m_Token = token;

  HPierceFirewall handshake(m_Token);
  sendMessage(handshake.make_network_packet());

  connect(ip, port);
}

void
newsoul::UserSocket::onCannotReverseConnect(NewNet::ClientSocket *)
{
  NNLOG("newsoul.user.debug", "Could not fulfill %s's connection request.", m_User.c_str());
  if (m_Newsoul->server()->loggedIn()) {
    SCannotConnect msg(m_User, m_Token);
    m_Newsoul->server()->sendMessage(msg.make_network_packet());
  }
  // Try to disconnect even if it's probably not necessary
  // then remove from reactor as this will probably not be done by disconnect()
  disconnect();
}

void
newsoul::UserSocket::sendMessage(const NewNet::Buffer & buffer)
{
  unsigned char buf[4];
  buf[0] = buffer.count() & 0xff;
  buf[1] = (buffer.count() >> 8) & 0xff;
  buf[2] = (buffer.count() >> 16) & 0xff;
  buf[3] = (buffer.count() >> 24) & 0xff;
  send(buf, 4);
  send(buffer.data(), buffer.count());
}
