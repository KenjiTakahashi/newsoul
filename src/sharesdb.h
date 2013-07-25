/*
 This is a part of newsoul @ http://github.com/KenjiTakahashi/newsoul
 Karol "Kenji Takahashi" Woźniak © 2013

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __NEWSOUL_SHARESDB_H__
#define __NEWSOUL_SHARESDB_H__

#include <db_cxx.h>
#include <map>
#include <stdint.h>
#include <sys/stat.h>
#include <taglib/fileref.h>
#include <vector>
#include <zlib.h>
#include "utils/os.h"
#include "utils/path.h"
#include "utils/string.h"
#include "efsw/include/efsw/efsw.hpp"

typedef struct FILEENTRY {
    uint64_t size;
    std::string ext;
    std::vector<unsigned int> attrs;
    time_t mtime;
} FileEntry;
typedef std::map<std::string, FileEntry> Folder;
typedef std::map<std::string, Folder> Shares;
typedef std::map<std::string, Shares> Folders;

namespace newsoul {
    class SharesDB : public efsw::FileWatchListener {
        efsw::FileWatcher fw; /*!< Real time FS watcher (efsw).*/
        /*!
         * Stores shared directory structure.
         * It is a K/V storage with duplicates.
         * Keys are FS paths and values are dirs and files inside them
         * (thus we need duplicated keys).
         */
        Db dirsdb;
        /*!
         * Stores info about a file.
         * It is a K/V storage with a little trickery.
         * Keys are:
         * 1) Filepath + "s" for size value,
         * 2) Filepath + "e" for ext value,
         * 3) Filepath + "a" for attrs array,
         * 3) Filepath + "l" for 3) size,
         * 4) Filepath + "m" for modification time,
         * where values are corresponding with these in FileEntry structure.
         */
        Db attrdb;
        //FIXME: This is silly and will have to go.
        std::function<void(void)> updateApp;
        std::vector<unsigned char> compressed;

    protected:
        SharesDB() : dirsdb(NULL, 0), attrdb(NULL, 0) { }
        /*!
         * Retrieves file attributes from database.
         * \param fn Path to file.
         * \return Filled FileEntry structure.
         */
        FileEntry getAttrs(const std::string &fn);
        /*!
         * Adds file to database.
         * Also used to update existing file entries.
         * \param dir Directory in which the file lies.
         * \param fn Filename.
         * \param path Whole path (usually dir+fn).
         * \param st Statistics structure.
         */
        void addFile(const std::string &dir, const std::string &fn, const std::string &path, struct stat &st);
        /*!
         * Removes file from database.
         * \param dir Directory in which the file lies.
         * \param fn Filename.
         * \param path Whole path (usually dir+fn).
         */
        void removeFile(const std::string &dir, const std::string &fn, const std::string &path);
        /*!
         * Adds directory to database.
         * It is used to take care of empty dirs (we need to know about them).
         * \param dir Directory in which the dir lies.
         * \param fn Basename.
         */
        void addDir(const std::string &dir, const std::string &fn);
        /*!
         * Removes directory from database.
         * \see newsoul::Config::addDir for some details.
         * \param path Path to directory (dir+basename).
         */
        void removeDir(const std::string &path);
        template<typename T> void pack(std::vector<unsigned char> &data, T i);
        void pack(std::vector<unsigned char> &data, std::string s);
        void compress();

    public:
        SharesDB(const std::string &fn, std::function<void(void)> func);
        ~SharesDB();

        void handleFileAction(efsw::WatchID wid, const std::string &dir, const std::string &fn, efsw::Action action, std::string oldFn);

        /*!
         * Returns SLSK compatible, compressed version of the database.
         * \return Compressed DB entries.
         */
        inline const std::vector<unsigned char> &shares() const { return this->compressed; }
        /*!
         * Retrieves files contained within given directory.
         * \param fn Directory.
         * \return Files within fn.
         */
        Shares contents(const std::string &fn);
        Folder query(const std::string &query) const;
        /*!
         * Repairs distorted path case.
         * Use case: We get path from an case insensitive FS (e.g. NTFS).
         * \param lower Distorted path.
         * \return Repaired path.
         */
        std::string toProperCase(const std::string &lower);
        /*!
         * Checks whether a specified file is shared.
         * \param fn Path to file.
         * \return True if shared, false otherwise.
         */
        bool isShared(const std::string &fn);
        /*!
         * Returns number of all files in the database.
         * \see newsoul::Config::dirsCount.
         * \return Number of files.
         */
        unsigned int filesCount();
        /*!
         * Returns number of all directories in the database.
         * \see newsoul::Config::filesCount.
         * \return Number of directories.
         */
        unsigned int dirsCount();
    };
}

#endif // __NEWSOUL_SHARESDB_H__
