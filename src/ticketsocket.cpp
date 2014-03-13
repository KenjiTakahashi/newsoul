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

#include "ticketsocket.h"

newsoul::TicketSocket::TicketSocket(newsoul::HandshakeSocket * that) : UserSocket(that, "F")
{
    // Sometimes, the ticket is sent at the connection of the socket so the data received event is not called.
    // You should call findTicket when you create a new TicketSocket, after adding it to the reactor
    dataReceivedEvent.connect(this, &TicketSocket::onDataReceived);
}

newsoul::TicketSocket::TicketSocket(newsoul::Newsoul * newsoul) : UserSocket(newsoul, "F")
{
  dataReceivedEvent.connect(this, &TicketSocket::onDataReceived);
}

newsoul::TicketSocket::~TicketSocket()
{
  //NNLOG("newsoul.ticket.debug", "TicketSocket destroyed");
}

void
newsoul::TicketSocket::onDataReceived(NewNet::ClientSocket *) {
    findTicket();
}

void
newsoul::TicketSocket::findTicket() {
    if(receiveBuffer().count() < 4)
        return;

    //NNLOG("newsoul.ticket.debug", "TicketSocket got %u bytes", receiveBuffer().count());
    // Unpack the ticket
    if (receiveBuffer().count() >= 4 ) {
        unsigned char * data = receiveBuffer().data();
        m_Ticket = data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24);
        receiveBuffer().seek(4);
    }

    // Notify our waiting downloadsockets
    //NNLOG("newsoul.ticket.debug", "Yay! We received ticket %u.. Now what..", m_Ticket);
    newsoul()->downloads()->transferTicketReceivedEvent(this);
    newsoul()->uploads()->transferTicketReceivedEvent(this);

    // Self-terminate
    receiveBuffer().clear();
    newsoul()->reactor()->remove(this);
}
