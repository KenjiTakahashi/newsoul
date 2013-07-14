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

#ifndef NEWSOUL_PEERMANAGER_H
#define NEWSOUL_PEERMANAGER_H

#include "peersocket.h"
#include "NewNet/nnfactorysocket.h"
#include "NewNet/nnobject.h"
#include "NewNet/nnrefptr.h"
#include "NewNet/nntcpserversocket.h"
#include "NewNet/nnweakrefptr.h"

namespace newsoul
{
  class PeerManager : public NewNet::Object
  {
  public:
    typedef NewNet::FactorySocket<NewNet::TcpServerSocket, HandshakeSocket> PeerFactory;

    PeerManager(Newsoul * newsoul);
    ~PeerManager();

    /* Return pointer to newsoul instance. */
    Newsoul * newsoul() const { return m_Newsoul; }

    PeerFactory * peerFactory() const
    {
      return m_Factory;
    }

    NewNet::Event<HandshakeSocket *> firewallPiercedEvent;
    NewNet::Event<PeerSocket *> peerSocketReadyEvent;
    NewNet::Event<std::string> peerSocketUnavailableEvent;
    NewNet::Event<std::string> peerOfflineEvent;

    /* Find or make a peer socket for the specified user. */
    void peerSocket(const std::string & user, bool force = true);
    void addPeerSocket(PeerSocket * socket);
    void removePeerSocket(const std::string & user, bool disconnect = false);

    std::map<std::string, UserData> *userStats() {return &m_UserStats;};
    std::map<std::string, uint32> *userStatus() {return &m_UserStatus;};
    bool isUserConnected(const std::string&);

    void requestUserData(const std::string& user);
    void setUserStatus(const std::string& user, uint32 status);

    void waitingPassiveConnection(UserSocket * socket);
    void removePassiveConnectionWaiting(uint token);
    void onFirewallPierced(HandshakeSocket * socket);
    void onCannotConnectOurself(NewNet::ClientSocket * socket_);

  protected:
    void listen();
    void unlisten();

  private:
    void onClientAccepted(HandshakeSocket * socket);

    void onServerLoggedInStateChanged(bool loggedIn);
    void onCannotConnectNotify(const SCannotConnect * msg);
    void onServerConnectToPeerRequested(const SConnectToPeer * message);
    void onServerUserStatusReceived(const SGetStatus * message);
    void onServerAddUserReceived(const SAddUser * message);
    void onPeerCannotConnect(NewNet::ClientSocket * socket_);
    void onDisconnected(NewNet::ClientSocket * socket_);
    void onConnected(NewNet::ClientSocket * socket_);

    void createPeerSocket(const std::string&);

    NewNet::WeakRefPtr<Newsoul> m_Newsoul;
    NewNet::RefPtr<PeerFactory> m_Factory;

    std::map<std::string, struct timeval >                  m_LastStatusTime;   // When did we ask for status of each user?
    std::map<std::string, NewNet::WeakRefPtr<PeerSocket> >  m_Peers;            // List of all the peer sockets
    std::map<std::string, UserData>                         m_UserStats;        // User stats
    std::map<std::string, uint32>                           m_UserStatus;       // User status
    std::map<uint, NewNet::RefPtr<UserSocket> >             m_PassiveConnects;  // Sockets trying to establish a passive connection
  };
}

#endif // NEWSOUL_PEERMANAGER_H
