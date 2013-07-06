#include <efsw/FileWatcherInotify.hpp>

#if EFSW_PLATFORM == EFSW_PLATFORM_INOTIFY

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/inotify.h>
#include <efsw/FileSystem.hpp>
#include <efsw/System.hpp>
#include <efsw/Debug.hpp>

#define BUFF_SIZE ((sizeof(struct inotify_event)+FILENAME_MAX)*1024)

namespace efsw
{

FileWatcherInotify::FileWatcherInotify( FileWatcher * parent ) :
	FileWatcherImpl( parent ),
	mFD(-1),
	mThread(NULL)
{
	mFD = inotify_init();

	if (mFD < 0)
	{
		efDEBUG( "Error: %s\n", strerror(errno) );
	}
	else
	{
		mInitOK = true;
	}
}

FileWatcherInotify::~FileWatcherInotify()
{
	WatchMap::iterator iter = mWatches.begin();
	WatchMap::iterator end = mWatches.end();

	for(; iter != end; ++iter)
	{
		efSAFE_DELETE( iter->second );
	}

	mWatches.clear();

	if ( mFD != -1 )
	{
		close(mFD);
		mFD = -1;
	}

	if ( mThread )
	{
		mThread->terminate();
	}

	efSAFE_DELETE( mThread );
}

WatchID FileWatcherInotify::addWatch( const std::string& directory, FileWatchListener* watcher, bool recursive )
{
	return addWatch( directory, watcher, recursive, NULL );
}

WatchID FileWatcherInotify::addWatch( const std::string& directory, FileWatchListener* watcher, bool recursive, WatcherInotify * parent )
{
	std::string dir( directory );

	FileSystem::dirAddSlashAtEnd( dir );

	FileInfo fi( dir );

	if ( !fi.isDirectory() )
	{
		return Errors::Log::createLastError( Errors::FileNotFound, dir );
	}
	else if ( !fi.isReadable() )
	{
		return Errors::Log::createLastError( Errors::FileNotReadable, dir );
	}
	else if ( pathInWatches( dir ) )
	{
		return Errors::Log::createLastError( Errors::FileRepeated, directory );
	}

	/// Check if the directory is a symbolic link
	std::string curPath;
	std::string link( FileSystem::getLinkRealPath( dir, curPath ) );

	if ( "" != link )
	{
		/// Avoid adding symlinks directories if it's now enabled
		if ( NULL != parent && !mFileWatcher->followSymlinks() )
		{
			return Errors::Log::createLastError( Errors::FileOutOfScope, dir );
		}

		/// If it's a symlink check if the realpath exists as a watcher, or
		/// if the path is outside the current dir
		if ( pathInWatches( link ) )
		{
			return Errors::Log::createLastError( Errors::FileRepeated, directory );
		}
		else if ( !linkAllowed( curPath, link ) )
		{
			return Errors::Log::createLastError( Errors::FileOutOfScope, dir );
		}
		else
		{
			dir = link;
		}
	}

	int wd = inotify_add_watch (mFD, dir.c_str(), IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE | IN_MOVED_FROM | IN_DELETE);

	if ( wd < 0 )
	{
		if( errno == ENOENT )
		{
			return Errors::Log::createLastError( Errors::FileNotFound, dir );
		}
		else
		{
			return Errors::Log::createLastError( Errors::Unspecified, std::string(strerror(errno)) );
		}
	}

	WatcherInotify * pWatch	= new WatcherInotify();
	pWatch->Listener	= watcher;
	pWatch->ID			= wd;
	pWatch->Directory	= dir;
	pWatch->Recursive	= recursive;
	pWatch->Parent		= parent;

	mWatchesLock.lock();
	mWatches.insert(std::make_pair(wd, pWatch));
	mWatchesLock.unlock();

	if ( NULL == pWatch->Parent )
	{
		mRealWatches[ pWatch->ID ] = pWatch;
	}

	if ( pWatch->Recursive )
	{
		std::map<std::string, FileInfo> files = FileSystem::filesInfoFromPath( pWatch->Directory );
		std::map<std::string, FileInfo>::iterator it = files.begin();

		for ( ; it != files.end(); it++ )
		{
			FileInfo fi = it->second;

			if ( fi.isDirectory() && fi.isReadable() )
			{
				addWatch( fi.Filepath, watcher, recursive, pWatch );
			}
		}
	}

	return wd;
}

void FileWatcherInotify::removeWatch(const std::string& directory)
{
	mWatchesLock.lock();

	WatchMap::iterator iter = mWatches.begin();

	for(; iter != mWatches.end(); ++iter)
	{
		if( directory == iter->second->Directory )
		{
			WatcherInotify * watch = iter->second;

			if ( watch->Recursive )
			{
				WatchMap::iterator it = mWatches.begin();
				std::list<WatchID> eraseWatches;

				for(; it != mWatches.end(); ++it)
				{
					if ( it->second->inParentTree( watch ) )
					{
						eraseWatches.push_back( it->second->ID );
					}
				}

				for ( std::list<WatchID>::iterator eit = eraseWatches.begin(); eit != eraseWatches.end(); eit++ )
				{
					removeWatch( *eit );
				}
			}

			mWatches.erase( iter );

			if ( NULL == watch->Parent )
			{
				WatchMap::iterator eraseit = mRealWatches.find( watch->ID );

				if ( eraseit != mRealWatches.end() )
				{
					mRealWatches.erase( eraseit );
				}
			}

			inotify_rm_watch(mFD, watch->ID);

			efSAFE_DELETE( watch );

			return;
		}
	}

	mWatchesLock.unlock();
}

void FileWatcherInotify::removeWatch( WatchID watchid )
{
	mWatchesLock.lock();

	WatchMap::iterator iter = mWatches.find( watchid );

	if( iter == mWatches.end() )
		return;

	WatcherInotify * watch = iter->second;

	if ( watch->Recursive )
	{
		WatchMap::iterator it = mWatches.begin();
		std::list<WatchID> eraseWatches;

		for(; it != mWatches.end(); ++it)
		{
			if ( it->second != watch &&
				 it->second->inParentTree( watch )
			)
			{
				eraseWatches.push_back( it->second->ID );
			}
		}

		for ( std::list<WatchID>::iterator eit = eraseWatches.begin(); eit != eraseWatches.end(); eit++ )
		{
			removeWatch( *eit );
		}
	}

	mWatches.erase( iter );

	if ( NULL == watch->Parent )
	{
		WatchMap::iterator eraseit = mRealWatches.find( watch->ID );

		if ( eraseit != mRealWatches.end() )
		{
			mRealWatches.erase( eraseit );
		}
	}

	efDEBUG( "Removed watch %s\n", watch->Directory.c_str() );

	inotify_rm_watch(mFD, watchid);

	efSAFE_DELETE( watch );

	mWatchesLock.unlock();
}

void FileWatcherInotify::watch()
{
	if ( NULL == mThread )
	{
		mThread = new Thread( &FileWatcherInotify::run, this );
		mThread->launch();
	}
}

void FileWatcherInotify::run()
{
	WatchMap::iterator wit;
	std::list<WatcherInotify*> movedOutsideWatches;

	do
	{
		ssize_t len, i = 0;
		static char buff[BUFF_SIZE] = {0};

		len = read (mFD, buff, BUFF_SIZE);

		if (len != -1)
		{	
			while (i < len)
			{
				struct inotify_event *pevent = (struct inotify_event *)&buff[i];

				mWatchesLock.lock();

				wit = mWatches.find( pevent->wd );

				if ( wit != mWatches.end() )
				{
					handleAction(wit->second, pevent->name, pevent->mask);

					/// Keep track of the IN_MOVED_FROM events to known if the IN_MOVED_TO event is also fired
					if ( !wit->second->OldFileName.empty() )
					{
						movedOutsideWatches.push_back( wit->second );
					}
				}

				mWatchesLock.unlock();

				i += sizeof(struct inotify_event) + pevent->len;
			}

			if ( !movedOutsideWatches.empty() )
			{
				/// In case that the IN_MOVED_TO is never fired means that the file was moved to other folder
				for ( std::list<WatcherInotify*>::iterator it = movedOutsideWatches.begin(); it != movedOutsideWatches.end(); it++ )
				{
					if ( !(*it)->OldFileName.empty() )
					{
						/// So we send a IN_DELETE event for files that where moved outside of our scope
						handleAction( *it, (*it)->OldFileName, IN_DELETE );

						/// Remove the OldFileName
						(*it)->OldFileName = "";
					}
				}

				movedOutsideWatches.clear();
			}
		}
	} while( mFD > 0 );
}

void FileWatcherInotify::handleAction( Watcher* watch, const std::string& filename, unsigned long action, std::string oldFilename )
{
	if ( !watch || !watch->Listener )
	{
		return;
	}

	std::string fpath( watch->Directory + filename );

	if( IN_CLOSE_WRITE & action )
	{
		watch->Listener->handleFileAction( watch->ID, watch->Directory, filename,Actions::Modified );
	}
	else if( IN_MOVED_TO & action )
	{
		/// If OldFileName doesn't exist means that the file has been moved from other folder, so we just send the Add event
		if ( watch->OldFileName.empty() )
		{
			watch->Listener->handleFileAction( watch->ID, watch->Directory, filename, Actions::Add );
		}
		else
		{
			watch->Listener->handleFileAction( watch->ID, watch->Directory, filename, Actions::Moved, watch->OldFileName );
		}

		if ( watch->Recursive && FileSystem::isDirectory( fpath ) )
		{
			/// Update the new directory path
			std::string opath( watch->Directory + watch->OldFileName );
			FileSystem::dirAddSlashAtEnd( opath );
			FileSystem::dirAddSlashAtEnd( fpath );

			for ( WatchMap::iterator it = mWatches.begin(); it != mWatches.end(); it++ )
			{
				if ( it->second->Directory == opath )
				{
					it->second->Directory = fpath;

					break;
				}
			}
		}

		watch->OldFileName = "";
	}
	else if( IN_CREATE & action )
	{
		watch->Listener->handleFileAction( watch->ID, watch->Directory, filename, Actions::Add );

		/// If the watcher is recursive, checks if the new file is a folder, and creates a watcher
		if ( watch->Recursive && FileSystem::isDirectory( fpath ) )
		{
			bool found = false;

			/// First check if exists
			for ( WatchMap::iterator it = mWatches.begin(); it != mWatches.end(); it++ )
			{
				if ( it->second->Directory == fpath )
				{
					found = true;
					break;
				}
			}

			if ( !found )
			{
				addWatch( fpath, watch->Listener, watch->Recursive, static_cast<WatcherInotify*>( watch ) );
			}
		}
	}
	else if ( IN_MOVED_FROM & action )
	{
		watch->OldFileName = filename;
	}
	else if( IN_DELETE & action )
	{
		watch->Listener->handleFileAction( watch->ID, watch->Directory, filename, Actions::Delete );

		/// If the file erased is a directory and recursive is enabled, removes the directory erased
		if ( watch->Recursive && FileSystem::isDirectory( fpath ) )
		{
			for ( WatchMap::iterator it = mWatches.begin(); it != mWatches.end(); it++ )
			{
				if ( it->second->Directory == fpath )
				{
					removeWatch( it->second->ID );
					break;
				}
			}
		}
	}
}

std::list<std::string> FileWatcherInotify::directories()
{
	std::list<std::string> dirs;

	mWatchesLock.lock();

	WatchMap::iterator it = mRealWatches.begin();

	for ( ; it != mRealWatches.end(); it++ )
	{
		dirs.push_back( it->second->Directory );
	}

	mWatchesLock.unlock();

	return dirs;
}

bool FileWatcherInotify::pathInWatches( const std::string& path )
{
	/// Search in the real watches, since it must allow adding a watch already watched as a subdir
	WatchMap::iterator it = mRealWatches.begin();

	for ( ; it != mRealWatches.end(); it++ )
	{
		if ( it->second->Directory == path )
		{
			return true;
		}
	}

	return false;
}

}

#endif
