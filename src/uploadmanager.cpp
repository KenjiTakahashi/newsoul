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

#include "uploadmanager.h"
#include "ifacemanager.h"

/**
  * Constructor
  * The given path should be encoded with FS encoding. Separator should be the FS one.
  */
newsoul::Upload::Upload(newsoul::Newsoul * newsoul, const std::string & user, const std::string & localPath)
{
    NNLOG("newsoul.up.debug", "Creating upload for %s, %s", user.c_str(), localPath.c_str());

    m_Newsoul = newsoul;
    m_User = user;
    m_Size = 0;
    m_Position = 0;
    m_Rate = 0;
    m_Ticket = 0;
    m_TicketValid = false;
    m_State = TS_Offline;
    m_Collected = 0;
    m_File = 0;

	m_CollectStart.tv_sec = m_CollectStart.tv_usec = 0;

    m_LocalPath = localPath;

    // We need to know the filesize. The only way is to open the file and look into it. But close it when we're done.
    openFile();
    closeFile();

    m_Newsoul->uploads()->uploadUpdatedEvent(this);
}

newsoul::Upload::~Upload()
{
    closeFile();

    NNLOG("newsoul.up.debug", "Upload destroyed.");
    m_Newsoul->uploads()->uploadRemovedEvent(this);
}

/**
  * Change the current state of the upload and consequences
  */
void
newsoul::Upload::setState(TrState state)
{
  	if(state == m_State)
		return;

	switch(state) {
        case TS_QueuedLocally:
        case TS_CannotConnect:
        case TS_Aborted:
        case TS_RemoteError:
        case TS_LocalError:
            closeFile();

            if(m_Socket.isValid()) {
                m_Socket->disconnect();
                m_Socket = 0;
            }
            break;

        case TS_Finished:
        case TS_ConnectionClosed:
            m_Socket = 0;
            closeFile();

            if(m_Position >= m_Size) {
                NNLOG("newsoul.up.debug", "transfer speed for %s was %u", m_User.c_str(), m_Rate);
                m_Newsoul->server()->sendMessage(SSendUploadSpeed(m_Rate).make_network_packet());
            }
            break;
        case TS_Offline:
            closeFile();
            break;

        default: ;
	}

    if (state == TS_Finished)
        setPosition(size());

	m_State = state;

    m_Newsoul->uploads()->uploadUpdatedEvent(this);

    if (    state != TS_Transferring
            && state != TS_Negotiating
            && state != TS_Waiting
            && state != TS_Establishing
            && state != TS_Initiating
            && state != TS_Connecting)
        m_Newsoul->uploads()->checkUploads();

    if (state == TS_Finished) {
        m_Newsoul->ifaces()->sendStatusMessage(true, std::string("Upload finished: '") + localPath() + std::string("' to ") + user());

        if (m_Newsoul->autoClearFinishedUploads())
            m_Newsoul->uploads()->remove(user(), localPath());
    }
    else if (state == TS_Transferring) {
        if (position() > 0)
            m_Newsoul->ifaces()->sendStatusMessage(true, std::string("Continuing upload: '") + localPath() + std::string("' to ") + user());
        else
            m_Newsoul->ifaces()->sendStatusMessage(true, std::string("Upload started: '") + localPath() + std::string("' to ") + user());
    }
    else if ((state == TS_RemoteError) || (state == TS_CannotConnect) || (state == TS_ConnectionClosed) || (state == TS_LocalError))
        m_Newsoul->ifaces()->sendStatusMessage(true, std::string("Upload failed: '") + localPath() + std::string("' to ") + user());
    else if (state == TS_Aborted)
        m_Newsoul->ifaces()->sendStatusMessage(true, std::string("Upload aborted: '") + localPath() + std::string("' to ") + user());
}

/**
  * An error has occured. The problem comes from the downloader.
  */
void
newsoul::Upload::setRemoteError(const std::string & error)
{
    setSocket(0);
    m_Error = error;
    setState(TS_RemoteError);
}

/**
  * An error has occured. The problem comes from our side.
  */
void
newsoul::Upload::setLocalError(const std::string & error)
{
    setSocket(0);
    m_Error = error;
    setState(TS_LocalError);
}

/**
  * This upload will use this socket
  */
void
newsoul::Upload::setSocket(UploadSocket * socket)
{
    // If we already have an upload socket, first stop it and then replace it by the new one
    if(m_Socket.isValid() && (m_Socket != socket))
        m_Socket->stop();

    m_Socket = socket;

	m_CollectStart.tv_sec = m_CollectStart.tv_usec = 0;

    if (socket) {
        socket->setUpRateLimiter(newsoul()->uploads()->limiter());
        if (m_WaitingTimeout.isValid())
            newsoul()->reactor()->removeTimeout(m_WaitingTimeout);
    }
}

/**
  * Set the position in the uploaded file
  */
void
newsoul::Upload::setPosition(uint64 position)
{
    m_Position = position;
    m_Newsoul->uploads()->uploadUpdatedEvent(this);
}

/**
  * Close the file. the ifstream object can only be used once so delete it. We'll create a new one later if needed.
  */
void newsoul::Upload::closeFile() {
    if (m_File) {
        NNLOG("newsoul.up.debug", "Closing %s", m_LocalPath.c_str());
        delete m_File;
        m_File = 0;
    }
}

/**
  * Open the file -> ready to read it and send data
  */
bool newsoul::Upload::openFile()
{
    closeFile();

	m_File = new std::ifstream(m_LocalPath.c_str(), std::fstream::in | std::fstream::binary);

	if(m_File->fail() || !m_File->is_open()) {
	    NNLOG("newsoul.up.warn", "Error while opening %s", m_LocalPath.c_str());
		return false;
	}

    // go to the end of the file
    m_File->seekg( 0, std::ios_base::end );
    // the position = size of the file
    m_Size = m_File->tellg();
    // go back to previous position
    m_File->seekg( 0,  std::ios_base::beg ) ;

    NNLOG("newsoul.up.debug", "Opening file %s (size: %i)", m_LocalPath.c_str(), size());

	return true;
}

/**
  * Seek to the position 'pos' in the file
  */
bool newsoul::Upload::seek(uint64 pos) {
    if (pos < 0 || pos > m_Size) {
        NNLOG("newsoul.up.warn", "Wrong seeking position: %u (max size: %u)", pos, m_Size);
        return false;
    }

	NNLOG("newsoul.up.debug", "seeking to %u", pos);
	setState(TS_Transferring);

	m_File->seekg(pos, std::ios_base::beg);

	if (!m_File)
		return false;

	m_Position = pos;

	return true;
}

/**
  * Reads some data in the file and put it in the send buffer
  */
bool newsoul::Upload::read(NewNet::Buffer & buffer) {
    NNLOG("newsoul.up.debug", "Reading from file");

    if(!m_Socket)
        return false;

	char buf[1024 * 1024];

	m_File->read(buf, 1024 * 1024);
	int64_t count = m_File->gcount();
	if(count == -1)
		return false;

    NNLOG("newsoul.up.debug", "Appending %u bytes to the buffer", count);
    m_Socket->send((const unsigned char *) &buf, count);

	return true;
}

/**
  * Called when some data has been sent to the peer
  */
void newsoul::Upload::sent(uint count) {
	m_Position += count;
	collect(count);
}

/**
  * We have sent x bytes => update the stats
  */
void newsoul::Upload::collect(uint bytes) {
	struct timeval now;
	gettimeofday(&now, NULL);

	if(m_CollectStart.tv_sec == 0)
		m_CollectStart = now;

	m_Collected += bytes;
	double diff = difftime(now, m_CollectStart); // Returns diff in ms
	if(diff >= 1000.0) {
		m_RatePool.push_back((uint)((double)m_Collected * 1000 / diff));
		while(m_RatePool.size() > 10)
			m_RatePool.erase(m_RatePool.begin());

        m_Rate = 0;

		std::vector<uint>::iterator it, end = m_RatePool.end();
		for(it = m_RatePool.begin(); it != end; ++it)
			m_Rate += *it;
		m_Rate /= m_RatePool.size();

		if(m_Rate < 0)
			m_Rate = 0;
		m_Collected = 0;
		m_CollectStart = now;

        m_Newsoul->uploads()->uploadUpdatedEvent(this);
	}
}

/**
  * We want to start an upload from our side: send a PTransferRequest.
  * The downloader will answer with PTransferReply (see onPeerTransferReplyReceived).
  * If he allows us to upload, we'll create an upload socket, send the filesize on it, wait for the start position and send the data.
  */
void newsoul::Upload::initiate(PeerSocket * socket) {
    if (!socket) {
        setState(TS_LocalError);
        NNLOG("newsoul.up.warn", "Invalid PeerSocket in newsoul::Upload::initiate()");
        return;
    }

    if(m_State != TS_Initiating)
		return;

	if(! openFile()) {
		setLocalError("Local file error");
		return;
	}

    if (m_WaitingTimeout.isValid())
        newsoul()->reactor()->removeTimeout(m_WaitingTimeout);

    m_WaitingTimeout = newsoul()->reactor()->addTimeout(60000, this, &Upload::replyTimeout);

	setState(TS_Waiting);

	m_Ticket = m_Newsoul->token();
	m_TicketValid = true;

	NNLOG("newsoul.up.debug", "initiating upload sequence %u", m_Ticket);

    newsoul()->uploads()->setTransferReplyCallback(socket->transferReplyReceivedEvent.connect(newsoul()->uploads(), &UploadManager::onPeerTransferReplyReceived));

	std::string path = newsoul()->codeset()->fromFSToNet(m_LocalPath);
	if (m_CaseProblem)
        path = string::tolower(path);
	if(! path.empty()) {
		PTransferRequest msg(m_Ticket, path, m_Size);
        socket->sendMessage(msg.make_network_packet());
	}
}

/**
  * Called when we didn't get the transfer reply
  */
void
newsoul::Upload::replyTimeout(long) {
    NNLOG("newsoul.up.debug", "No transfer reply for uploading.");
    setSocket(0);
    setState(TS_CannotConnect);
}



newsoul::UploadManager::UploadManager(Newsoul * newsoul) : m_Newsoul(newsoul)
{
    // Connect some events.
    newsoul->server()->loggedInStateChangedEvent.connect(this, &UploadManager::onServerLoggedInStateChanged);
    newsoul->peers()->peerSocketReadyEvent.connect(this, &UploadManager::onPeerSocketReady);
    newsoul->peers()->peerSocketUnavailableEvent.connect(this, &UploadManager::onPeerSocketUnavailable);
    newsoul->peers()->peerOfflineEvent.connect(this, &UploadManager::onPeerOffline);
    uploadAddedEvent.connect(this, &UploadManager::onUploadAdded);
    uploadUpdatedEvent.connect(this, &UploadManager::onUploadUpdated);
    //newsoul->config()->keySetEvent.connect(this, &UploadManager::onConfigKeySet);
    //newsoul->config()->keyRemovedEvent.connect(this, &UploadManager::onConfigKeyRemoved);

    m_Limiter = new NewNet::RateLimiter();
    m_Limiter->setLimit(-1);
}

newsoul::UploadManager::~UploadManager()
{
    NNLOG("newsoul.up.debug", "Upload Manager destroyed");
}

/**
  * Called when an upload is added.
  * Add the user to the list of user we're uploading to
  */
void newsoul::UploadManager::onUploadAdded(Upload * upload) {
    if (upload->state() == TS_Transferring ||
            upload->state() == TS_Negotiating ||
            upload->state() == TS_Waiting ||
            upload->state() == TS_Establishing ||
            upload->state() == TS_Initiating ||
            upload->state() == TS_Connecting)
        addUploading(upload);

    if (upload->state() == TS_Negotiating ||
            upload->state() == TS_Waiting ||
            upload->state() == TS_Establishing ||
            upload->state() == TS_Initiating ||
            upload->state() == TS_Connecting)
        addInitiating(upload);
}

/**
  * Called when an upload is updated.
  * Add or remove the user to/from the list of user we're uploading to
  */
void newsoul::UploadManager::onUploadUpdated(Upload * upload) {
    if (upload->state() == TS_Transferring)
        addUploading(upload);
    else if (upload->state() == TS_Negotiating ||
                upload->state() == TS_Waiting ||
                upload->state() == TS_Establishing ||
                upload->state() == TS_Initiating ||
                upload->state() == TS_Connecting) {
        addUploading(upload);
        addInitiating(upload);
    }
    else {
        if (upload == isUploadingTo(upload->user()))
            removeUploading(upload->user());
        if (upload == isInitiatingTo(upload->user()))
            removeInitiating(upload->user());
    }
}

/**
  * We're uploading to this user
  */
void newsoul::UploadManager::addUploading(Upload * upload) {
    if (isUploadingTo(upload->user()) != upload) {
        NNLOG("newsoul.up.debug", "We're uploading to %s", upload->user().c_str());
        m_Uploading[upload->user()] = upload;
    }
}

/**
  * We're no longer uploading to this user
  */
void newsoul::UploadManager::removeUploading(const std::string & user) {
    std::map<std::string, NewNet::WeakRefPtr<Upload> >::iterator it = m_Uploading.find(user);
    if (it != m_Uploading.end()) {
        NNLOG("newsoul.up.debug", "Not uploading to %s", user.c_str());
        m_Uploading.erase(it);
        updateRates();
    }
}

/**
  * We're initiating an upload to this user
  */
void newsoul::UploadManager::addInitiating(Upload * upload) {
    if (isInitiatingTo(upload->user()) != upload) {
        NNLOG("newsoul.up.debug", "We're initiating the upload to %s", upload->user().c_str());
        m_Initiating[upload->user()] = upload;
    }
}

/**
  * We're no longer initiating an upload to this user
  */
void newsoul::UploadManager::removeInitiating(const std::string & user) {
    std::map<std::string, NewNet::WeakRefPtr<Upload> >::iterator it = m_Initiating.find(user);
    if (it != m_Initiating.end()) {
        NNLOG("newsoul.up.debug", "Not initiating to %s", user.c_str());
        m_Initiating.erase(it);
    }
}

/**
  * Are we already uploading to this user?
  */
newsoul::Upload * newsoul::UploadManager::isUploadingTo(const std::string & user) {
    if (m_Uploading.find(user) != m_Uploading.end())
        return m_Uploading[user];

    return 0;
}

/**
  * Returns the list of users in the upload queue (currently downloading or in not)
  */
std::vector<std::string> newsoul::UploadManager::getAllUsersWithUpload() {
    std::vector<std::string> res;
    std::vector<NewNet::RefPtr<Upload> >::const_iterator it = m_Uploads.begin();
    for (; it != m_Uploads.end(); it++) {
        if (std::find(res.begin(), res.end(), (*it)->user()) == res.end()) {
            res.push_back((*it)->user());
        }
    }

    return res;
}

/**
  * Are we already initiating an upload to this user?
  */
newsoul::Upload * newsoul::UploadManager::isInitiatingTo(const std::string & user) {
    if (m_Initiating.find(user) != m_Initiating.end())
        return m_Initiating[user];

    return 0;
}

/**
  * Look if there are some uploads to start
  */
void newsoul::UploadManager::checkUploads() {
	if(! hasFreeSlots()) {
        NNLOG("newsoul.up.debug", "No slot available for upload");
		return;
	}

    NNLOG("newsoul.up.debug", "Checking if there are some uploads to start");

	Upload* candidate = 0;
	std::vector<NewNet::RefPtr<Upload> >::iterator it = m_Uploads.begin();
	for(; it != m_Uploads.end(); ++it) {
	    if ((*it)->state() == TS_QueuedLocally && newsoul()->isBanned((*it)->user()))
	        (*it)->setLocalError("Banned");
		else if((*it)->state() == TS_QueuedLocally && !isUploadingTo((*it)->user())) {
			if(newsoul()->isPrivileged((*it)->user()) || (newsoul()->privilegeBuddies() && newsoul()->isBuddied((*it)->user()))) {
				candidate = *it;
				break;
			}
			if(! candidate)
				candidate = *it;
		}
	}
	if(candidate) {
	    NNLOG("newsoul.up.debug", "Can start upload of %s to %s", candidate->localPath().c_str(), candidate->user().c_str());
        candidate->setState(TS_Initiating);
	    newsoul()->peers()->peerSocket(candidate->user());
	    checkUploads();
	}
}

/**
  * Update the rate limiter for every uploads
  */
void newsoul::UploadManager::updateRates() {
    std::map<std::string, NewNet::WeakRefPtr<Upload> >::iterator it;
    int globalRate = newsoul()->config()->getInt({"transfers", "upload_rate"});
    if (globalRate > 0) {
        // There's a limit, update the rate limiter
        m_Limiter->setLimit(globalRate*1000);
    }
    else {
        // No limit
        m_Limiter->setLimit(-1);
    }
}

/**
  * Register the uploading of file localPath to the given user
  * The given path should be encoded with FS encoding. Separator should be the FS one.
  */
void
newsoul::UploadManager::add(const std::string & user, const std::string & localPath, const uint & ticket, const bool caseProblem, const bool forceEnqueue)
{
    // Check if this upload already exits.
    Upload * upload = findUpload(user, localPath);
    if(! upload) {
        // Create new upload object.
        upload = new Upload(newsoul(), user, localPath);
        upload->setCaseProblem(caseProblem);
        if (ticket <= 0)
            upload->setTicket(newsoul()->token());
        else
            upload->setTicket(ticket);
        upload->validateTicket();
        m_Uploads.push_back(upload);
        NNLOG("newsoul.up.debug", "Created new upload entry, user=%s, localpath=%s, ticket=%u.", user.c_str(), localPath.c_str(), upload->ticket());
        uploadAddedEvent(upload);

        upload->setState(TS_QueuedLocally);
    }
    else if (forceEnqueue || ((upload->state() != TS_Aborted) && (upload->state() != TS_LocalError)))
        upload->setState(TS_QueuedLocally);
}

/**
  * Register the uploading of folder localPath to the given user
  * The given path should be encoded with utf8 encoding. Separator should be the network one (backslash).
  */
void
newsoul::UploadManager::addFolder(const std::string & user, const std::string & localPath)
{
    NNLOG("newsoul.up.debug", "Uploading folder %s to %s.", localPath.c_str(), user.c_str());
    std::string dir = newsoul()->codeset()->toNet(localPath);
    std::string error;
    if (! newsoul()->isBanned(user)) {
        Shares content;
        if (newsoul()->haveBuddyShares() && newsoul()->isBuddied(user))
            content = newsoul()->buddyshares()->contents(dir);
        else
            content = newsoul()->shares()->contents(dir);

        Shares::const_iterator it;
        Folder::const_iterator fit;
        for (it = content.begin(); it != content.end(); it++) {
            for (fit = it->second.begin(); fit != it->second.end(); fit++) {
                std::string pathFile = dir + '\\' + fit->first;
                NNLOG("newsoul.up.debug", "Uploading the folder means uploading file %s", pathFile.c_str());
                if (newsoul()->uploads()->isUploadable(user, pathFile, &error))
                    newsoul()->uploads()->add(user, newsoul()->codeset()->fromNetToFS(pathFile), 0, false, true);
            }
        }
    }


}

/**
  * Returns the Upload object for the given user and the given path
  * The given path should be encoded with FS encoding. Separator should be the FS one.
  */
newsoul::Upload *
newsoul::UploadManager::findUpload(const std::string & user, const std::string & path)
{
    // Iterate over m_Uploads until we find a match.
    std::vector<NewNet::RefPtr<Upload> >::iterator it, end = m_Uploads.end();
    for(it = m_Uploads.begin(); it != end; ++it) {
        if(((*it)->user() == user) && ((*it)->localPath() == path))
            return *it;
    }

    NNLOG("newsoul.up.debug", "Upload %s not found", path.c_str());
    return 0;
}

/**
  * Returns the Upload object for the given user and the given ticket
  */
newsoul::Upload *
newsoul::UploadManager::findUpload(const std::string & user, uint ticket)
{
    // Iterate over m_Uploads until we find a match.
    std::vector<NewNet::RefPtr<Upload> >::iterator it, end = m_Uploads.end();
    for(it = m_Uploads.begin(); it != end; ++it) {
        if(((*it)->user() == user) && ((*it)->ticket() == ticket))
            return *it;
    }

    NNLOG("newsoul.up.debug", "Upload with ticket %d not found", ticket);
    return 0;
}

/**
  * Abort an upload
  * The given path should be encoded with FS encoding. Separator should be the FS one.
  */
void
newsoul::UploadManager::abort(const std::string & user, const std::string & path)
{
    Upload * upload = findUpload(user, path);
    if(! upload)
        return;

    upload->setSocket(0);
    if(upload->state() != TS_Finished)
        upload->setState(TS_Aborted);
}

/**
  * Abort the upload and removes it from the manager
  * The given path should be encoded with FS encoding. Separator should be the FS one.
  */
void
newsoul::UploadManager::remove(const std::string & user, const std::string & path)
{
    Upload * upload = findUpload(user, path);
    if(! upload)
        return;

    abort(user, path);

    std::vector<NewNet::RefPtr<Upload> >::iterator it;
    it = std::find(m_Uploads.begin(), m_Uploads.end(), upload);
    if (it != m_Uploads.end())
        m_Uploads.erase(it);
}

/**
  * Updates the given transfer if we found it
  * The given path should be encoded with FS encoding. Separator should be the FS one.
  */
void
newsoul::UploadManager::update(const std::string & user, const std::string & path)
{
    Upload * upload = findUpload(user, path);
    if(! upload)
        return;

    uploadUpdatedEvent(upload);
}

/**
  * Get the queue length
  * The given path should be encoded with FS encoding. Separator should be the FS one.
  */
uint newsoul::UploadManager::queueLength(const std::string& user, const std::string& stopAt) {
	bool priv = newsoul()->isPrivileged(user) || (newsoul()->privilegeBuddies() && newsoul()->isBuddied(user));

	std::vector<NewNet::RefPtr<Upload> >::const_iterator it, end = m_Uploads.end();
	uint uploads = 0;
	bool found = false;

	for(it = m_Uploads.begin(); it != end; ++it) {
	    // Count every upload that are before this one in the queue
		if((*it)->state() == TS_QueuedLocally && (! priv || newsoul()->isPrivileged((*it)->user()) || (newsoul()->privilegeBuddies() && newsoul()->isBuddied((*it)->user()))) && !found)
			uploads++;

        // There might be some privileged uploads after this one. Count them if we're not privileged
        if (found && (*it)->state() == TS_QueuedLocally && !priv && (newsoul()->isPrivileged((*it)->user()) || (newsoul()->privilegeBuddies() && newsoul()->isBuddied((*it)->user()))))
			uploads++;

		if(((*it)->localPath() == stopAt) || ((*it)->hasCaseProblem() && (string::tolower((*it)->localPath()) == stopAt)))
			found = true;
	}

	if(it == end && !found)
		return 0;

	return uploads;
}

/**
  * Get the total queue length
  */
uint newsoul::UploadManager::queueTotalLength() {
    std::vector<NewNet::RefPtr<Upload> >::const_iterator it, end = m_Uploads.end();
	uint uploads = 0;

	for(it = m_Uploads.begin(); it != end; ++it) {
	    // Count every upload queued
		if((*it)->state() == TS_QueuedLocally)
			uploads++;
	}

	return uploads;
}

/**
  * Called when the connection to the server changes (connected/disconnected)
  */
void
newsoul::UploadManager::onServerLoggedInStateChanged(bool loggedIn)
{
    if(loggedIn) {
        // Look if there's some uploads to restart
        checkUploads();
    }
    else {
        // Server connection was severed. As are our chances to connect to a peer.
        std::vector<NewNet::RefPtr<Upload> >::iterator it, end = m_Uploads.end();
        for(it = m_Uploads.begin(); it != end; ++it) {
            if (((*it) != isUploadingTo((*it)->user()) && ((*it)->state() != TS_Finished) && ((*it)->state() != TS_Aborted) && ((*it)->state() != TS_Offline)))
                (*it)->setState(TS_Offline);
        }
    }
}

/**
  * Receives the PTransferReply after we have asked to initiate an upload sending a PTransferRequest
  */
void
newsoul::UploadManager::onPeerTransferReplyReceived(const PTransferReply * message)
{
    // Find the upload this concerns.
    const std::string & user = message->peerSocket()->user();
    Upload * upload = findUpload(user, message->ticket);
    if(! upload)
        return; // No such upload, bail out.

    if(upload->state() != TS_Waiting)
        return; // No longer needing this upload, bail out.

    if (m_TransferReplyCallback.isValid()) {
        message->peerSocket()->transferReplyReceivedEvent.disconnect(m_TransferReplyCallback);
        m_TransferReplyCallback = 0;
    }

    if(message->allowed) {
        // Transfer can start immediately, no queue at remote end.
        NNLOG("newsoul.up.debug", "Got transfer reply: user=%s,path=%s,ticket=%u,allowed=yes. Initiating upload.", user.c_str(), upload->localPath().c_str(), upload->ticket());
        UploadSocket * uploadSocket = new UploadSocket(newsoul(), upload);
        upload->setSocket(uploadSocket);
        newsoul()->reactor()->add(uploadSocket);
        uploadSocket->initiate(user.c_str());
        uploadSocket->sendTicket();
    }
    else {
        // Transfer (currently) not possible.
        NNLOG("newsoul.up.debug", "Got transfer reply: user=%s,path=%s,ticket=%u,allowed=no,reason=%s", user.c_str(), upload->localPath().c_str(), upload->ticket(), message->reason.c_str());
        upload->setRemoteError(message->reason);
    }
}

/**
  * There's a peer socket available: see if we have something to send
  */
void newsoul::UploadManager::onPeerSocketReady(PeerSocket * socket) {
    std::string username = socket->user();

    // Check if we have any uploads with status user offline for this user.
    std::vector<NewNet::RefPtr<Upload> >::iterator dit, dend = m_Uploads.end();
    for(dit = m_Uploads.begin(); dit != dend; ++dit) {
        if (((*dit)->user() == username) && ((*dit)->state() == TS_Offline))
            (*dit)->setState(TS_QueuedLocally);
    }

    Upload * userUpload = isUploadingTo(socket->user());

    // We cannot start an upload for this user
    if(!userUpload || userUpload->state() != TS_Initiating)
		return;

    NNLOG("newsoul.up.debug", "Sending upload request to %s for file %s", userUpload->user().c_str(), userUpload->localPath().c_str());
    // If we have a socket, we can initiate the upload. Otherwise, peerSocket will create a socket and call checkUploads
    userUpload->initiate(socket);
    checkUploads();
}

/**
  *  Called when the connection cannot be made with the peer.
  */
void
newsoul::UploadManager::onPeerSocketUnavailable(std::string user)
{
    Upload * current = isUploadingTo(user);
	if (current) {
        current->setSocket(0);
        current->setState(TS_CannotConnect);
	}
}

/**
  * One of our peer got offline: clean all is stuff
  */
void newsoul::UploadManager::onPeerOffline(std::string user) {
    std::vector<NewNet::RefPtr<Upload> >::iterator it, end = m_Uploads.end();
    for(it = m_Uploads.begin(); it != end; ++it) {
        if((*it)->user() == user
            && (*it)->state() != TS_Finished
            && (*it)->state() != TS_RemoteError
            && (*it)->state() != TS_LocalError
            && (*it)->state() != TS_Transferring
            && (*it)->state() != TS_ConnectionClosed
            && (*it)->state() != TS_CannotConnect
            && (*it)->state() != TS_Aborted
            && (*it)->state() != TS_Offline)
            (*it)->setState(TS_Offline);
    }
}

/**
  *   Return true if there is some free slots, false otherwise.
  */
bool newsoul::UploadManager::hasFreeSlots() {
    return (m_Uploading.size() < newsoul()->upSlots()) || (newsoul()->upSlots() == 0);
}

/**
  * Called when some key of the config has been changed
  */
//void
//newsoul::UploadManager::onConfigKeySet(const newsoul::ConfigManager::ChangeNotify * data)
//{
    //if(data->domain == "transfers" && data->key == "upload_slots")
        //checkUploads();
    //if(data->domain == "transfers" && data->key == "upload_rate")
        //updateRates();
    //if(data->domain == "banned") {
        //Upload * current = isUploadingTo(data->key);
        //if (current)
            //current->setLocalError("Banned");
        //checkUploads();
    //}
//}

/**
  * Called when some key of the config has been deleted
  */
//void
//newsoul::UploadManager::onConfigKeyRemoved(const newsoul::ConfigManager::RemoveNotify * data)
//{
    //if(data->domain == "transfers" && data->key == "upload_slots")
        //checkUploads();
    //if(data->domain == "transfers" && data->key == "upload_rate")
        //updateRates();
//}

/**
  * Return true if the given user can download the given file. Otherwise, returns false and put the error message in 'error'.
  * The given path should be encoded with net encoding. Separator should be the network one (backslash).
  */
bool newsoul::UploadManager::isUploadable(const std::string & user, const std::string & path, std::string * error)
{
    if (!error)
        return false;

    *error = std::string();

    bool buddyShared = newsoul()->buddyshares()->isShared(path);
    bool normalShared = newsoul()->shares()->isShared(path);

    if ( user == newsoul()->server()->username() )
        *error = "Cannot Transfer to yourself";
    else if(newsoul()->isBanned(user)) {
        if (newsoul()->toBuddiesOnly())
            *error = "Sharing Only to List";
        else
            *error = "Banned";
    }
    else if(!newsoul()->isBuddied(user) && newsoul()->toBuddiesOnly())
        *error = "Sharing Only to List";
    else if(!normalShared && !buddyShared )
        *error = "File not shared";
    else if(newsoul()->haveBuddyShares() && buddyShared && !normalShared && ! newsoul()->isBuddied(user))
        *error = "File not shared";
    else if( ! newsoul()->haveBuddyShares()  && buddyShared  && !normalShared )
        *error = "File not shared";


    if (!error->empty()) {
        NNLOG("newsoul.up.debug", "File %s is not uploadable to %s because : %s", path.c_str(), user.c_str(), error->c_str());
        return false;
    }
    else
        return true;
}

/**
  * Return true if the given user can download a file corresponding to the given path (case insensitive).
  * If true, put the correct path in 'goodPath'.
  * The given path should be encoded with net encoding. Separator should be the network one (backslash).
  */
bool newsoul::UploadManager::findUploadableNoCase(const std::string & user, const std::string & path, std::string * goodPath)
{
    if (!goodPath)
        return false;

    *goodPath = std::string();

    std::string normalShared = newsoul()->shares()->toProperCase(path);

    if (!normalShared.empty()) {
        *goodPath = normalShared;
        NNLOG("newsoul.up.debug", "Found an uploadable file for %s to user %s: %s", path.c_str(), user.c_str(), goodPath->c_str());
        return true;
    }
    else if (newsoul()->haveBuddyShares() && newsoul()->isBuddied(user)) {
        std::string buddyShared = newsoul()->buddyshares()->toProperCase(path);
        if (!buddyShared.empty()) {
            *goodPath = buddyShared;
            NNLOG("newsoul.up.debug", "Found an uploadable file for %s to user %s: %s", path.c_str(), user.c_str(), goodPath->c_str());
            return true;
        }
    }

    NNLOG("newsoul.up.debug", "Couldn't find an uploadable file for %s to user %s", path.c_str(), user.c_str());
    return false;
}
