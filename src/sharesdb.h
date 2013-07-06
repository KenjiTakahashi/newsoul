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

#ifndef __NEWSOUL_DB_H__
#define __NEWSOUL_DB_H__

#include <db_cxx.h>
#include <map>
#include <stdint.h>
#include <sys/stat.h>
#include <taglib/fileref.h>
#include <vector>
#include <zlib.h>
#include "newsoul.h"
#include "utils/path.h"
#include "utils/string.h"
#include "efsw/include/efsw/efsw.hpp"
#include "NewNet/nnweakrefptr.h"

namespace Museek { //FIXME: remove it once we move to new ns
    class Museekd;
}

typedef struct FILEENTRY {
    uint64_t size;
    std::string ext;
    std::vector<unsigned int> attrs;
} FileEntry;
typedef std::map<std::string, FileEntry> Folder;
typedef std::map<std::string, Folder> Shares;
typedef std::map<std::string, Shares> Folders;

namespace newsoul {
    class SharesDB : public efsw::FileWatchListener {
        efsw::FileWatcher *fw; /*!< Real time FS watcher (efsw) */
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
         * where values are corresponding with these in FileEntry structure.
         */
        Db attrdb;
        NewNet::WeakRefPtr<Museek::Museekd> app;
        std::vector<unsigned char> compressed;

        FileEntry getAttrs(const std::string &fn);
        void addFile(const std::string &dir, const std::string &fn, const std::string &path, unsigned int size);
        void removeFile(const std::string &dir, const std::string &fn, const std::string &path);
        void addDir(const std::string &dir, const std::string &fn);
        void removeDir(const std::string &path);
        template<typename T> void pack(std::vector<unsigned char> &data, T i);
        void pack(std::vector<unsigned char> &data, std::string s);
        void compress();

    public:
        SharesDB(Museek::Museekd *museekd, const std::string &fn);
        ~SharesDB();

        void handleFileAction(efsw::WatchID wid, const std::string &dir, const std::string &fn, efsw::Action action, std::string oldFn);

        inline const std::vector<unsigned char> &shares() const { return this->compressed; }
        Shares contents(const std::string &fn);
        Folder query(const std::string &query) const;
        std::string toProperCase(const std::string &lower);
        bool isShared(const std::string &fn);
        unsigned int filesCount();
        unsigned int dirsCount();
    };
}

#endif // __NEWSOUL_DB_H__
