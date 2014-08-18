/*  newsoul - A SoulSeek client written in C++
    Copyright (C) 2006-2007 Ingmar K. Steen (iksteen@gmail.com)
    Copyright 2008 little blue poney <lbponey@users.sourceforge.net>
    Karol 'Kenji Takahashi' Woźniak © 2013 - 2014

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

#include "downloadmanager.h"
#include "ifacemanager.h"

/**
  * Constructor
  * The remote path should be encoded with utf8 encoding. Separator should be the network one (backslash).
  * The localDir should be encoded with FS encoding. Separator should be the FS one.
  */
newsoul::Download::Download(newsoul::Newsoul * newsoul, const std::string & user, const std::string & remotePath, const std::string & localDir)
{
    //NNLOG("newsoul.down.debug", "Creating download from %s, %s", user.c_str(), remotePath.c_str());

    m_Newsoul = newsoul;
    m_User = user;
    m_Enqueued = false;
    m_RemotePath = remotePath;
    m_LocalDir = localDir;
    m_Size = 0;
    m_Position = 0;

    m_Rate = 0;
    m_Ticket = 0;
    m_State = TS_Offline;
    m_Collected = 0;

    m_Place = 0;

	m_CollectStart.tv_sec = m_CollectStart.tv_usec = 0;

    newsoul->downloads()->downloadUpdatedEvent(this);
}

newsoul::Download::~Download()
{
  //NNLOG("newsoul.down.debug", "Download destroyed.");
  newsoul()->downloads()->downloadRemovedEvent(this);
}

/**
  * Returns the name of the file we're downloading.
  * Encoded with utf8 encoding. Separator is the network one (backslash).
  */
std::string
newsoul::Download::filename() const
{
    if(m_Filename != std::string())
        return m_Filename;

    std::string::size_type ix = m_RemotePath.find_last_of('\\');
    if(ix == std::string::npos)
        m_Filename = m_RemotePath;
    else
        m_Filename = m_RemotePath.substr(ix + 1);

    return m_Filename;
}

/**
  * Returns the path where the incomplete file should be saved while downloading
  * Encoded with FS encoding. Separator is the FS one.
  */
std::string
newsoul::Download::incompletePath() const
{
    if(m_IncompletePath != std::string())
        return m_IncompletePath;

    // Get the incomplete directory
    std::string incompletedir = m_Newsoul->config()->getStr({"downloads", "incomplete"});
    // Fall back to download directory.
    if(incompletedir.empty())
        incompletedir = m_Newsoul->config()->getStr({"downloads", "complete"});
    // Fall back to current directory.
    if(incompletedir.empty())
        incompletedir = ".";

    // Make sure the directory exists.
    os::mkdir(incompletedir);

    // Build file path: <incompletedir>/incomplete.<filesize>.<filename>
    std::stringstream path;
    path << incompletedir << os::separator() << "incomplete." << size() << ".";
    path << filename();

    m_IncompletePath = newsoul()->codeset()->fromUtf8ToFS(path.str());
    return m_IncompletePath;
}

/**
  * Return the destination path of the file, where it will be saved when finished
  * Encoded with FS encoding. Separator is the FS one.
  */
std::string
newsoul::Download::destinationPath(bool create) const
{
    // Build destination directory.
    std::stringstream path;
    // Was an absolute path provided with the download?
    if(!path::isAbsolute(m_LocalDir)) {
        // Nope, use the download directory
        std::string downloaddir = newsoul()->codeset()->fromUtf8ToFS(m_Newsoul->config()->getStr({"downloads", "complete"}));
        // Fallback to current directory.
        if(downloaddir.empty())
            downloaddir = ".";
        path << downloaddir;
        // Add the provided relative path.
        if(! m_LocalDir.empty())
            path << os::separator() << m_LocalDir;
    }
    else
        // Use the provided absolute path.
        path << m_LocalDir;

    // Make sure the directory exists.
    if (create)
        os::mkdir(path.str());

    path << os::separator();

    // Add the filename to the directory.
    path << newsoul()->codeset()->fromUtf8ToFS(filename());

    return path.str();
}

/**
  * Set the size of the downloaded file
  */
void
newsoul::Download::setSize(uint64 size)
{
    m_Size = size;
    if (state() == TS_Finished)
        setPosition(size);

    m_Newsoul->downloads()->downloadUpdatedEvent(this);
}

/**
  * Set the position in the downloaded file
  */
void
newsoul::Download::setPosition(uint64 position)
{
    m_Position = position;
    m_Newsoul->downloads()->downloadUpdatedEvent(this);
}

/**
  * Set the position in the downloaded file looking at the possibly existing incomplete file
  */
void
newsoul::Download::setPositionFromIncompleteFile()
{
    std::string temppath = incompletePath();

    if(temppath == std::string()) // No incomplete path set
        return;

    uint64 position = 0;

    std::ifstream ifs(temppath.c_str(), std::ifstream::in);
    if (!ifs.fail()) {
        ifs.seekg (0, std::ios_base::end);
        position = ifs.tellg();
        ifs.seekg (0, std::ios_base::beg);
    }
    ifs.close();
    setPosition(position);
}

/**
  * Called when some data has been received from the peer
  */
void
newsoul::Download::received(uint bytes)
{
	struct timeval now;
	gettimeofday(&now, NULL);

	if(m_CollectStart.tv_sec == 0)
		m_CollectStart = now;

	m_Collected += bytes;
    m_Position += bytes;
	long diff = difftime(now, m_CollectStart); // Returns diff in ms
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

        m_Newsoul->downloads()->downloadUpdatedEvent(this);
	}
}

/**
  * Set the place in queue for this download
  */
void
newsoul::Download::setPlace(uint place) {
    m_Place = place;
    m_Newsoul->downloads()->downloadUpdatedEvent(this);
}

/**
  * Change the current state of the upload and consequences
  */
void
newsoul::Download::setState(TrState state)
{
    bool changed = false;
    if (m_State != state)
        changed = true;

    TrState previous = m_State;

    m_State = state;
    if (state == TS_Finished)
        setPosition(size());

    m_Newsoul->downloads()->downloadUpdatedEvent(this);

    if (    state != TS_Transferring
            && state != TS_Negotiating
            && state != TS_Waiting
            && state != TS_Establishing
            && state != TS_Initiating
            && state != TS_Connecting)
        m_Newsoul->downloads()->checkDownloads();

    if (changed && (state == TS_Finished)) {
        if (previous != TS_Transferring)
            m_Newsoul->ifaces()->sendStatusMessage(true, std::string("Download already finished: '") + destinationPath() + std::string("' from ") + user());
        else
            m_Newsoul->ifaces()->sendStatusMessage(true, std::string("Download finished: '") + destinationPath() + std::string("' from ") + user());

        if (m_Newsoul->autoClearFinishedDownloads())
            m_Newsoul->downloads()->remove(user(), remotePath());
    }
    else if (changed && (state == TS_Transferring)) {
        if (position() > 0)
            m_Newsoul->ifaces()->sendStatusMessage(true, std::string("Continuing download: '") + destinationPath() + std::string("' from ") + user());
        else
            m_Newsoul->ifaces()->sendStatusMessage(true, std::string("Download started: '") + destinationPath() + std::string("' from ") + user());
    }
    else if (changed && ((state == TS_RemoteError) || (state == TS_CannotConnect) || (state == TS_ConnectionClosed) || (state == TS_LocalError))) {
        std::string retryMsg = "";
        if (m_Newsoul->autoRetryDownloads()) {
            long timeout = 30000; // 30 seconds
            newsoul()->reactor()->addTimeout(timeout, this, &Download::retry);
            std::stringstream msg;
            msg << ". Will retry in " << timeout/1000 << "s.";
            retryMsg = msg.str();
        }
        m_Newsoul->ifaces()->sendStatusMessage(true, std::string("Download failed: '") + destinationPath() + std::string("' from ") + user() + retryMsg);
    }
    else if (changed && (state == TS_Aborted))
        m_Newsoul->ifaces()->sendStatusMessage(true, std::string("Download aborted: '") + destinationPath() + std::string("' from ") + user());
}

/**
  * Retry the download if it in a failed state
  */
void
newsoul::Download::retry(long) {
    m_Newsoul->ifaces()->sendStatusMessage(true, std::string("Download retried: '") + destinationPath() + std::string("' from ") + user());
    newsoul()->downloads()->add(user(), remotePath()); // Re-adding ourself is a convenient way to retry a download
}

/**
  * An error has occured. The problem comes from the uploader.
  */
void
newsoul::Download::setRemoteError(const std::string & error)
{
    setSocket(0);
    setEnqueued(false);
    m_Error = error;
    setState(TS_RemoteError);
}

/**
  * This download will use this socket
  */
void
newsoul::Download::setSocket(DownloadSocket * socket)
{
    // If we already have a download socket, first stop it and then replace it by the new one
    if(m_Socket.isValid() && m_Socket != socket)
        m_Socket->stop();

    m_Socket = socket;

	m_CollectStart.tv_sec = m_CollectStart.tv_usec = 0;

    if (socket)
        socket->setDownRateLimiter(newsoul()->downloads()->limiter());

    newsoul()->reactor()->removeTimeout(m_InitTimeout);
}

/**
  * We want to start a download from our side: send a PTransferRequest.
  * Should not be necessary anymore (since 157)
  */
void
newsoul::Download::initiate(PeerSocket * socket) {
    if (!socket) {
        setState(TS_LocalError);
        //NNLOG("newsoul.down.warn", "Invalid PeerSocket in newsoul::Download::initiate()");
        return;
    }

    if (!newsoul()->downloads()->hasFreeSlots())
        return;

    if(m_State != TS_QueuedRemotely)
		return;

	setState(TS_Initiating);

	m_Ticket = m_Newsoul->token();

	//NNLOG("newsoul.down.debug", "Initiating download sequence %u", m_Ticket);

    newsoul()->downloads()->setTransferReplyCallback(socket->transferReplyReceivedEvent.connect(newsoul()->downloads(), &DownloadManager::onPeerTransferReplyReceived));

	std::string path = newsoul()->codeset()->toPeer(user(), m_RemotePath);
	if(! path.empty()) {
		PTransferRequest msg(m_Ticket, newsoul()->codeset()->toPeer(socket->user(), path), m_Size);
        socket->sendMessage(msg.make_network_packet());
	}
}

/**
  * Init timeout is finished
  */
void
newsoul::Download::initTimedOut(long) {
    if (state() == TS_Initiating)
        setState(TS_QueuedRemotely);
}


newsoul::DownloadManager::DownloadManager(Newsoul * newsoul) : m_Newsoul(newsoul)
{
    // Connect some events.
    newsoul->server()->loggedInStateChangedEvent.connect(this, &DownloadManager::onServerLoggedInStateChanged);
    newsoul->peers()->peerSocketReadyEvent.connect(this, &DownloadManager::onPeerSocketReady);
    newsoul->peers()->peerSocketUnavailableEvent.connect(this, &DownloadManager::onPeerSocketUnavailable);
    newsoul->peers()->peerOfflineEvent.connect(this, &DownloadManager::onPeerOffline);
    downloadAddedEvent.connect(this, &DownloadManager::onDownloadAdded);
    downloadUpdatedEvent.connect(this, &DownloadManager::onDownloadUpdated);

    m_AllowUpdate = false;
    m_AllowSave = true;
    m_PendingDownloadsSave = false;
    m_Limiter = new NewNet::RateLimiter();
    m_Limiter->setLimit(-1);
    this->updateRates();
}

newsoul::DownloadManager::~DownloadManager()
{
    //NNLOG("newsoul.down.debug", "Download Manager destroyed");
}

/**
  * Download a folder: first get the folder contents, then download all the files it contains.
  * The path should be encoded with utf8 encoding. Separator should be the network one (backslash).
  * The localPath should be encoded with FS encoding. Separator should be the FS one.
  */
void
newsoul::DownloadManager::addFolder(const std::string & user, const std::string & path, const std::string & localPath) {
    m_ContentsPending[user][path] = localPath;
    m_Newsoul->peers()->peerSocket(user);
}

/**
  * Look if there's some PFolderContentsRequest pending the given peersocket
  */
void
newsoul::DownloadManager::askPendingFolderContents(PeerSocket * socket) {
    if (socket) {
        std::map<std::string, std::map<std::string, std::string> >::iterator it;
        std::map<std::string, std::string>::iterator fit;

        std::map<std::string, std::map<std::string, std::string> > pending = m_ContentsPending; // copy map to avoid undefined iterator

        for (it = pending.begin(); it != pending.end() ; it++) {
            if (socket->user() == it->first) {
                //NNLOG("newsoul.down.debug", "Asking pending folder contents to %s", it->first.c_str());
                for (fit = (*it).second.begin(); fit != (*it).second.end(); fit++) {
                    m_ContentsAsked[(*it).first][(*fit).first] = (*fit).second;

                    PFolderContentsRequest msg(m_Newsoul->codeset()->toPeer((*it).first, (*fit).first));
                    socket->sendMessage(msg.make_network_packet());
                }
                m_ContentsPending.erase(it->first);
            }
        }
    }
}

/**
  * Look if there's some PPlaceInQueueRequest pending the given peersocket
  */
void
newsoul::DownloadManager::askPendingPlaces(PeerSocket * socket) {
    if (socket) {
        std::map<std::string, std::vector<std::string> >::iterator it;
        std::vector<std::string>::iterator pit;

        std::map<std::string, std::vector<std::string> > pending = m_PlacesPending;

        for (it = pending.begin(); it != pending.end() ; it++) {
            if (socket->user() == it->first) {
                //NNLOG("newsoul.down.debug", "Asking pending place in queue to %s", it->first.c_str());
                for (pit = (*it).second.begin(); pit != (*it).second.end(); pit++) {
                    PPlaceInQueueRequest msg(newsoul()->codeset()->toPeer((*it).first, *pit));
                    socket->sendMessage(msg.make_network_packet());
                }
                m_PlacesPending.erase(it->first);
            }
        }
    }
}

/**
 * Analyse the folder contents we've received and look if we can start some download with it
 */
void newsoul::DownloadManager::addFolderContents(const std::string & user, const Shares & folders) {
    if (m_ContentsAsked.find(user) == m_ContentsAsked.end()) {
        //NNLOG("newsoul.down.warn", "Unexpected folder content from %s.", user.c_str());
        return;
    }

    Shares::const_iterator fit;
    std::string downloadDir = newsoul()->config()->getStr({"downloads", "complete"});

    // Don't download files matching a blacklist item
    std::vector<std::string> blacklistItems = newsoul()->config()->getVec({"downloads", "blacklist"});

    for (fit = folders.begin(); fit != folders.end(); fit++) {
        // Folder we have asked the contents
        std::string remotePathBase = newsoul()->codeset()->fromPeer(user, fit->first);
        // The (optional) local path where the folder should be downloaded
        std::string localPathBase = m_ContentsAsked[user].find(remotePathBase)->second;
        if (m_ContentsAsked[user].find(remotePathBase) == m_ContentsAsked[user].end()) {
            //NNLOG("newsoul.down.warn", "Unexpected folder content from %s.", user.c_str());
            continue;
        }

        Dirs::const_iterator sit;
        m_AllowUpdate = false;
        for (sit = fit->second.begin(); sit != fit->second.end(); sit++) {
            // Remote path where the file is located (=remotePath without the filename)
            std::string remotePathDir = newsoul()->codeset()->fromPeer(user, sit->first);

            // Some clients (official one for example) sends us files from folder /example/folder2 when asking only for /example/folder
            // Just throw away the wrongly sent files
            if (std::string(remotePathDir + '\\').find(remotePathBase + '\\') != 0)
                continue;

            std::string localPath; // Complete local path of the file
            // Construct destination path
            if (localPathBase != std::string())
                localPath = localPathBase;
            else
                localPath = downloadDir;
            // We need to recreate the folder structure inside the folder we're downloading
            size_t posB = remotePathBase.find_last_of('\\');
            if (posB != std::string::npos && posB < remotePathDir.size())
                localPath += newsoul()->codeset()->fromUtf8ToFS(remotePathDir.substr(posB));

            Dir::const_iterator fiit;
            bool blacklisted = false;
            for (fiit = sit->second.begin(); fiit != sit->second.end(); fiit++) {
                std::string filename = newsoul()->codeset()->fromPeer(user, fiit->first);

                // Don't download files matching a blacklist item
                std::vector<std::string>::const_iterator bit;
                blacklisted = false;
                for (bit = blacklistItems.begin(); bit != blacklistItems.end(); bit++) {
                    blacklisted = !fnmatch((*bit).c_str(), filename.c_str(), FNM_PATHNAME);
                    // TODO (Kenji): Log error when fnmatch fails to perform
                    if (blacklisted) {
                        LOG(INFO) << "File `" << filename << "` matched blacklist pattern `" << *bit << "`";
                        m_Newsoul->ifaces()->sendStatusMessage(true, std::string("File '") + filename.c_str() + "' is blacklisted (" + bit->c_str()+")");
                        break;
                    }
                }

                if (blacklisted)
                    continue; // Skip to next file

                // Adding each file
                std::string remotePath = remotePathDir; // Complete remote path of a file
                remotePath += "\\";
                remotePath += filename;

                if (localPath != std::string() && localPath != downloadDir)
                    add(user, remotePath, localPath);
                else
                    add(user, remotePath);
                Download * addedDown = findDownload(user, remotePath);
                if (addedDown)
                    addedDown->setSize(fiit->second.size);
            }
        }
        m_AllowUpdate = true;
        checkDownloads();

        std::map<std::string, std::string>::iterator cit;
        cit = m_ContentsAsked[user].find(remotePathBase);
        if (cit != m_ContentsAsked[user].end()) {
            m_ContentsAsked[user].erase(cit);
            if (m_ContentsAsked[user].size() == 0)
                m_ContentsAsked.erase(user);
        }
    }
}

/**
  * Called when a download is added.
  * Add the user to the list of user we're downloading from
  */
void newsoul::DownloadManager::onDownloadAdded(Download * download) {
    if (download->state() == TS_Transferring ||
            download->state() == TS_Negotiating ||
            download->state() == TS_Waiting ||
            download->state() == TS_Establishing ||
            download->state() == TS_Initiating ||
            download->state() == TS_Connecting)
        addDownloading(download);

    if (download->state() == TS_Negotiating ||
            download->state() == TS_Waiting ||
            download->state() == TS_Establishing ||
            download->state() == TS_Initiating ||
            download->state() == TS_Connecting)
        addInitiating(download);
}

/**
  * Called when an download is updated.
  * Add or remove the user to/from the list of user we're downloading from
  */
void newsoul::DownloadManager::onDownloadUpdated(Download * download) {
    if (download->state() == TS_Transferring)
        addDownloading(download);
    else if (download->state() == TS_Negotiating ||
                download->state() == TS_Waiting ||
                download->state() == TS_Establishing ||
                download->state() == TS_Initiating ||
                download->state() == TS_Connecting) {
        addDownloading(download);
        addInitiating(download);
    }
    else {
        if (download == isDownloadingFrom(download->user()))
            removeDownloading(download->user());
        if (download == isInitiatingFrom(download->user()))
            removeInitiating(download->user());
    }
}

/**
  * We're downloading from this user
  */
void newsoul::DownloadManager::addDownloading(Download * download) {
    if (isDownloadingFrom(download->user()) != download) {
        //NNLOG("newsoul.down.debug", "We're downloading from %s", download->user().c_str());
        m_Downloading[download->user()] = download;
    }
}

/**
  * We're no longer downloading from this user
  */
void newsoul::DownloadManager::removeDownloading(const std::string & user) {
    std::map<std::string, NewNet::WeakRefPtr<Download> >::iterator it = m_Downloading.find(user);
    if (it != m_Downloading.end()) {
        //NNLOG("newsoul.down.debug", "Not downloading from %s", user.c_str());
        m_Downloading.erase(it);
        updateRates();
    }
}

/**
  * We're initiating a download from this user
  */
void newsoul::DownloadManager::addInitiating(Download * download) {
    if (isInitiatingFrom(download->user()) != download) {
        //NNLOG("newsoul.down.debug", "We're initiating the download from %s", download->user().c_str());
        m_Initiating[download->user()] = download;
    }
}

/**
  * We're no longer initiating a download from this user
  */
void newsoul::DownloadManager::removeInitiating(const std::string & user) {
    std::map<std::string, NewNet::WeakRefPtr<Download> >::iterator it = m_Initiating.find(user);
    if (it != m_Initiating.end()) {
        //NNLOG("newsoul.down.debug", "Not initiating to %s", user.c_str());
        m_Initiating.erase(it);
    }
}

/**
  * Are we already downloading from this user?
  */
newsoul::Download * newsoul::DownloadManager::isDownloadingFrom(const std::string & user) {
    if (m_Downloading.find(user) != m_Downloading.end())
        return m_Downloading[user];

    return 0;
}

/**
  * Are we already initiating an download from this user?
  */
newsoul::Download * newsoul::DownloadManager::isInitiatingFrom(const std::string & user) {
    if (m_Initiating.find(user) != m_Initiating.end())
        return m_Initiating[user];

    return 0;
}

/**
  * Look if there are some downloads to start
  */
void newsoul::DownloadManager::checkDownloads() {
    if (m_AllowUpdate) {
        //NNLOG("newsoul.down.debug", "Checking if there are some downloads to start");

        std::vector<NewNet::RefPtr<Download> >::iterator it = m_Downloads.begin();
        Download * download;
        for(; it != m_Downloads.end(); ++it) {
            download = *it;
            if ((download->state() == TS_QueuedRemotely && !download->enqueued()) || download->state() == TS_Offline) {
                // We will have something to do with this user: ask a peersocket
                newsoul()->peers()->peerSocket(download->user());
            }
        }

        saveDownloads();
    }
}

/**
  * Update the rate limiter for every downloads
  */
void newsoul::DownloadManager::updateRates() {
    std::map<std::string, NewNet::WeakRefPtr<Download> >::iterator it;
    int globalRate = newsoul()->config()->getInt({"downloads", "maxspeed"});
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
  * Send a request to the peer to put the given download on queue
  */
void
newsoul::DownloadManager::enqueueDownload(Download * download)
{
    //NNLOG("newsoul.down.debug", "Enqueuing %s", download->remotePath().c_str());
    download->setEnqueued(true);
    if (std::find(m_EnqueuingPending[download->user()].begin(), m_EnqueuingPending[download->user()].end(), download->remotePath()) == m_EnqueuingPending[download->user()].end())
        m_EnqueuingPending[download->user()].push_back(download->remotePath());
}

/**
  * Send a request to the peer to put the given download on queue
  */
void
newsoul::DownloadManager::askPendingEnqueuing(PeerSocket * socket)
{
    if (socket) {
        std::map<std::string, std::vector<std::string> >::iterator it;
        std::vector<std::string>::iterator eit;

        std::map<std::string, std::vector<std::string> > pending = m_EnqueuingPending;

        for ( it = pending.begin(); it != pending.end(); it++) {
            if (it->first == socket->user()) {
                //NNLOG("newsoul.down.debug", "Sending pending enqueuing request to %s", it->first.c_str());
                for (eit = (*it).second.begin(); eit != (*it).second.end(); eit++) {
                    PQueueDownload msg(newsoul()->codeset()->toPeer(it->first, *eit));
                    socket->sendMessage(msg.make_network_packet());
                }
                m_EnqueuingPending.erase(it->first);
            }
        }
    }
}

/**
  * Register the downloading of file localPath to the given user
  * The path should be encoded with utf8 encoding. Separator should be the network one (backslash).
  * The localDir should be encoded with FS encoding. Separator should be the FS one.
  */
void
newsoul::DownloadManager::add(const std::string & user, const std::string & path, const std::string & localPath, const uint & ticket)
{
    // Check if this download already exits.
    Download * download = findDownload(user, path);
    if(! download) {
        // Create new download object.
        download = new Download(newsoul(), user, path, localPath);
        if (ticket <= 0)
            download->setTicket(newsoul()->token());
        else
            download->setTicket(ticket);
        m_Downloads.push_back(download);
        //NNLOG("newsoul.down.debug", "Created new download entry, user=%s, path=%s, ticket=%u.", user.c_str(), path.c_str(), download->ticket());
        downloadAddedEvent(download);
    }
    else if (download->state() == TS_Offline || !newsoul()->server()->loggedIn()) {
        // Retrying an offline user... no sense
        return;
    }

    // Do nothing if already downloading: this prevent running downloads to be stopped (the peer would not like it)
    if ((download->state() != TS_Connecting) &&
        (download->state() != TS_Establishing) &&
        (download->state() != TS_Initiating) &&
        (download->state() != TS_Negotiating) &&
        (download->state() != TS_Waiting) &&
        (download->state() != TS_Transferring)) {

        download->setEnqueued(false); // Ensure we're gonna enqueue it even if it has already been done previously (useful when we want to retry)
        download->setState(TS_QueuedRemotely);
        download->setPositionFromIncompleteFile();

        // Check that we don't already have this file downloaded in destination dir
        std::ifstream file(download->destinationPath().c_str(), std::fstream::in | std::fstream::binary);
        if(file.is_open()) {
            //NNLOG("newsoul.down.debug", "%s has already been downloaded.", path.c_str());
            download->setState(TS_Finished);
            file.close();
        }

        checkDownloads();
        }
}

/**
  * Returns the Download object for the given user and the given path
  * The path should be encoded with utf8 encoding. Separator should be the network one (backslash).
  */
newsoul::Download *
newsoul::DownloadManager::findDownload(const std::string & user, const std::string & path)
{
    // Iterate over m_Downloads until we find a match.
    std::vector<NewNet::RefPtr<Download> >::iterator it, end = m_Downloads.end();
    for(it = m_Downloads.begin(); it != end; ++it) {
        if(((*it)->user() == user) && ((*it)->remotePath() == path))
            return *it;
    }

    //NNLOG("newsoul.down.debug", "Download %s not found", path.c_str());
    return 0;
}

/**
  * Returns the Download object for the given user and the given ticket
  */
newsoul::Download *
newsoul::DownloadManager::findDownload(const std::string & user, uint ticket)
{
    // Iterate over m_Downloads until we find a match.
    std::vector<NewNet::RefPtr<Download> >::iterator it, end = m_Downloads.end();
    for(it = m_Downloads.begin(); it != end; ++it) {
        if(((*it)->user() == user) && ((*it)->ticket() == ticket))
            return *it;
    }

    //NNLOG("newsoul.down.debug", "Download with ticket %d not found", ticket);
    return 0;
}

/**
  * Abort a download
  * The path should be encoded with utf8 encoding. Separator should be the network one (backslash).
  */
void
newsoul::DownloadManager::abort(const std::string & user, const std::string & path)
{
    Download * download = findDownload(user, path);
    if(! download)
        return;

    download->setSocket(0);
    download->setEnqueued(false);
    if(download->state() != TS_Finished)
        download->setState(TS_Aborted);
}

/**
  * Abort the download and removes it from the manager
  * The path should be encoded with utf8 encoding. Separator should be the network one (backslash).
  */
void
newsoul::DownloadManager::remove(const std::string & user, const std::string & path)
{
    Download * download = findDownload(user, path);
    if(! download)
        return;

    abort(user, path);
    std::vector<NewNet::RefPtr<Download> >::iterator it;
    it = std::find(m_Downloads.begin(), m_Downloads.end(), download);
    if (it != m_Downloads.end())
        m_Downloads.erase(it);

    saveDownloads();
}

/**
  * Updates the given transfer if we found it
  * The path should be encoded with utf8 encoding. Separator should be the network one (backslash).
  */
void
newsoul::DownloadManager::update(const std::string & user, const std::string & path)
{
    Download * download = findDownload(user, path);
    if(! download)
        return;

    m_PlacesPending[user].push_back(path);
    newsoul()->peers()->peerSocket(user);

    downloadUpdatedEvent(download);
}

/**
  * Called when the connection to the server changes (connected/disconnected)
  */
void
newsoul::DownloadManager::onServerLoggedInStateChanged(bool loggedIn)
{
    if(loggedIn) {
        // Look if there are some downloads to restart
        checkDownloads();
    }
    else {
        // Server connection was severed. As are our chances to connect to a peer.
        std::vector<NewNet::RefPtr<Download> >::iterator it, end = m_Downloads.end();
        for(it = m_Downloads.begin(); it != end; ++it) {
            if ( ((*it) != isDownloadingFrom((*it)->user())) && ((*it)->state() != TS_Finished) && ((*it)->state() != TS_Aborted) && ((*it)->state() != TS_Offline) ) {
                (*it)->setState(TS_Offline);
                (*it)->setEnqueued(false);
            }
        }
    }
}

/**
  * Receives the PTransferReply after we have asked to initiate a download sending a PTransferRequest
  * Should not be necessary anymore (since 157)
  */
void
newsoul::DownloadManager::onPeerTransferReplyReceived(const PTransferReply * message)
{
    // Find the download this concerns.
    const std::string & user = message->peerSocket()->user();
    Download * download = findDownload(user, message->ticket);
    if(! download)
        return; // No such download, bail out.

    if(download->state() != TS_Initiating)
        return; // No longer needing this download, bail out.

    if (m_TransferReplyCallback.isValid()) {
        message->peerSocket()->transferReplyReceivedEvent.disconnect(m_TransferReplyCallback);
        m_TransferReplyCallback = 0;
    }

    if(message->allowed) {
        // Transfer can start immediately, no queue at remote end.
        //NNLOG("newsoul.down.debug", "Got transfer reply: user=%s,path=%s,ticket=%u,allowed=yes,filesize=%llu. Initiating download.", user.c_str(), download->remotePath().c_str(), download->ticket(), message->filesize);
        download->setSize(message->filesize);
        DownloadSocket * downloadSocket = new DownloadSocket(newsoul(), download);
        download->setSocket(downloadSocket);
        newsoul()->reactor()->add(downloadSocket);
        downloadSocket->pickUp();
    }
    else {
        // Transfer (currently) not possible.
        //NNLOG("newsoul.down.debug", "Got transfer reply: user=%s,path=%s,ticket=%u,allowed=no,reason=%s", user.c_str(), download->remotePath().c_str(), download->ticket(), message->reason.c_str());
        download->setRemoteError(message->reason);
    }
}

/**
  * There's a peer socket available: see if we have something to send
  */
void newsoul::DownloadManager::onPeerSocketReady(PeerSocket * socket) {
    std::string username = socket->user();

    // Check if we have any downloads with status user offline for this user.
    std::vector<NewNet::RefPtr<Download> >::iterator dit, dend = m_Downloads.end();
    for(dit = m_Downloads.begin(); dit != dend; ++dit) {
        if (((*dit)->user() == username) && ((*dit)->state() == TS_Offline))
            (*dit)->setState(TS_QueuedRemotely);
    }

    // See if we can send some pending folder contents requests for this user
    askPendingFolderContents(socket);

	std::vector<NewNet::RefPtr<Download> >::iterator it = m_Downloads.begin();
	Download * download;
	for(; it != m_Downloads.end(); ++it) {
	    download = *it;
	    if (download->user() == socket->user() && download->state() == TS_QueuedRemotely && !download->enqueued()) {
            if (!isDownloadingFrom(download->user())) {
                //NNLOG("newsoul.down.debug", "Starting download %s", download->remotePath().c_str());
                // Starting from 157, there's no need to send a PTransferRequest. Enqueuing the file is sufficient
                download->setState(TS_Initiating);
                download->setInitTimeout(newsoul()->reactor()->addTimeout(10000, download, &Download::initTimedOut));
                enqueueDownload(download);
            }
            else {
                //NNLOG("newsoul.down.debug", "Enqueueing download %s", download->remotePath().c_str());
                enqueueDownload(download);
            }
	    }
	}

    // See if we can send some pending enqueuing requests for this user
    askPendingEnqueuing(socket);

    // See if we can send some pending place in queue requests for this user
    askPendingPlaces(socket);
}

/**
  *  Called when the connection cannot be made with the peer.
  */
void
newsoul::DownloadManager::onPeerSocketUnavailable(std::string user)
{
    Download * current = isDownloadingFrom(user);
	if (current) {
        current->setSocket(0);
        current->setEnqueued(false);
        current->setState(TS_CannotConnect);
	}
}

/**
  * One of our peer got offline: clean all is stuff
  */
void newsoul::DownloadManager::onPeerOffline(std::string user) {
    // Set downloads to offline
    std::vector<NewNet::RefPtr<Download> >::iterator it, end = m_Downloads.end();
    for(it = m_Downloads.begin(); it != end; ++it) {
        if ((*it)->user() == user) {
            (*it)->setEnqueued(false);
            if((*it)->state() != TS_Finished
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

    // Erase the pending enqueuing requests
    std::map<std::string, std::vector<std::string> >::iterator eit;
    eit = m_EnqueuingPending.find(user);
    if (eit != m_EnqueuingPending.end())
        m_EnqueuingPending.erase(eit);

    // Erase the pending folder contents requests pending
    std::map<std::string, std::map<std::string, std::string> >::iterator cit;
    cit = m_ContentsPending.find(user);
    if (cit != m_ContentsPending.end())
        m_ContentsPending.erase(cit);

    // Erase the pending folder contents requests asked
    std::map<std::string, std::map<std::string, std::string> >::iterator ait;
    ait = m_ContentsAsked.find(user);
    if (ait != m_ContentsAsked.end())
        m_ContentsAsked.erase(ait);

    // Erase the pending places in queue requests
    std::map<std::string, std::vector<std::string> >::iterator pit;
    pit = m_PlacesPending.find(user);
    if (pit != m_PlacesPending.end())
        m_PlacesPending.erase(pit);
}

/**
  *  Return true if there is some free slots, false otherwise.
  */
bool newsoul::DownloadManager::hasFreeSlots() {
    return (m_Downloading.size() < newsoul()->downSlots()) || ( newsoul()->downSlots() == 0);
}

/**
  * Loads the downloads stored in the config file
  */
void newsoul::DownloadManager::loadDownloads() {
    m_AllowUpdate = false; // We don't want downloads to be saved until we have finished to load them
    // Open config file
    std::string path = newsoul()->config()->getStr({"downloads", "queue"});
    std::ifstream file(path.c_str(), std::fstream::in | std::fstream::binary);

	if(file.fail() || !file.is_open()) {
		//NNLOG("newsoul.config.warn", "Cannot load downloads (%s).", path.c_str());
        file.close();
		return;
	}

    uint32 n;
    if (!read_int(&file, &n)) {
		//NNLOG("newsoul.down.warn", "Cannot load number of downloads.");
        file.close();
		return;
    }

	//NNLOG("newsoul.down.debug", "Loading %d downloads", n);

	while(n) {
		uint32 state;
		uint64 size;
		std::string user, path, localpath, temppath;
		if(!read_int(&file, &state) ||
		   !read_str(&file, user) ||
		   !read_off(&file, &size) ||
		   !read_str(&file, path) ||
		   !read_str(&file, localpath) ||
		   !read_str(&file, temppath)) {
			//NNLOG("newsoul.config.warn", "Cannot load downloads. Bailing out");
            file.close();
			return;
		}
		if (!path.empty()) {
            //NNLOG("newsoul.down.debug", "Loading download: %s from %s (size: %d)", path.c_str(), user.c_str(), size);
            size_t posB = localpath.find_last_of(os::separator());
            add(user, path, localpath.substr(0, posB));
            Download * dl = findDownload(user, path);
            if (dl) {
                if(state == 0)
                    dl->setState(TS_Aborted);
                else
                    dl->setState(TS_Offline); // We're not sure the peer is connected
                dl->setSize(size);
                dl->setIncompletePath(temppath);

                dl->setPositionFromIncompleteFile();
            }
		}
		//else
			//NNLOG("newsoul.config.warn", "Couldn't load a corrupted download: %s from %s (size: %d)", path.c_str(), user.c_str(), size);

		n--;
	}
	file.close();
	m_AllowUpdate = true; // We have finished: now try to enqueue downloads
	checkDownloads();
}

/**
  * Stores the download in the config file
  */
void newsoul::DownloadManager::saveDownloads() {
    if (m_AllowSave) {
        m_AllowSave = false;
        // Open config file
        std::string path = newsoul()->config()->getStr({"downloads", "queue"});
        std::string pathTemp(path + ".tmp");
        std::remove(pathTemp.c_str()); // Remove the temp file if it already exists

        std::ofstream file(pathTemp.c_str(), std::ofstream::binary | std::ofstream::app | std::ofstream::ate);

        if(file.fail() || !file.is_open()) {
            //NNLOG("newsoul.config.warn", "Cannot save downloads (%s). Trying again later", path.c_str());
            m_AllowSave = true;
            return;
        }

        uint32 transfers = 0;

        std::vector<NewNet::RefPtr<Download> >::const_iterator it = downloads().begin();
        for(; it != downloads().end(); ++it)
            if((*it)->state() != TS_Finished)
                transfers++;

        //NNLOG("newsoul.down.debug", "Saving %d downloads", transfers);

        if(!write_int(&file, transfers) == -1) {
            //NNLOG("newsoul.config.warn", "Cannot save downloads number, trying again later.");
            file.close();
            m_AllowSave = true;
            return;
        }

        for(it = downloads().begin(); it != downloads().end(); ++it) {
            if (m_PendingDownloadsSave)
                break; // If we have another save request, stop saving and restart from scratch

            uint32 state;
            switch((*it)->state()) {
            case TS_Finished:
                continue;
            case TS_Aborted:
                state = 0;
                break;
            default:
                state = 1;
                break;
            }

            // The incomplete path is useless if we haven't started the download
            std::string tmpPath = newsoul()->codeset()->fromFsToUtf8((*it)->incompletePath(), false);
            std::ifstream incompleteTest( tmpPath.c_str() );
            if (incompleteTest.fail())
                tmpPath = std::string();
            incompleteTest.close();

            if(!write_int(&file, state) ||
               write_str(&file, (*it)->user()) == -1 ||
               !write_off(&file, (*it)->size()) ||
               write_str(&file, (*it)->remotePath()) == -1 ||
               write_str(&file, newsoul()->codeset()->fromFsToUtf8((*it)->destinationPath(), false)) == -1 ||
               write_str(&file, tmpPath) == -1) {
                //NNLOG("newsoul.config.warn", "Cannot save downloads, trying again later.");
                file.close();
                m_AllowSave = true;
                return;
            }
        }

        file.close();

        if (!m_PendingDownloadsSave) {
        #ifdef WIN32
            // On Win32, rename doesn't overwrite an existing file automatically.
            remove(path.c_str());
        #endif // WIN32
            // Rename the temp file to the correct path.
            if(rename(pathTemp.c_str(), path.c_str()) == -1) {
                // Something happened. But nobody knows what.
                //NNLOG("newsoul.config.warn", "Renaming downloads config file failed for unknown reason.");
            }
        }

        // OK, we've finished. See if someone asked to save while we were saving (if true, we have to save now)
        m_AllowSave = true;
        if (m_PendingDownloadsSave)
            saveDownloads();
    }
    else {
        //NNLOG("newsoul.down.debug", "Delaying downloads saving");
        m_PendingDownloadsSave = true;
    }
}
