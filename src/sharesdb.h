/*
 This is a part of newsoul @ http://github.com/KenjiTakahashi/newsoul
 Karol "Kenji Takahashi" Woźniak © 2013 - 2014

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

#include <initializer_list>
#include <ftw.h>
#include <glog/logging.h>
#include <map>
#include <sqlite3.h>
#include <stdint.h>
#include <sys/stat.h>
#include <taglib/fileref.h>
#include <queue>
#include <vector>
#include <zlib.h>
#include "utils/os.h"
#include "utils/path.h"
#include "utils/string.h"
#include <functional>

namespace newsoul {
    class File {
    public:
        uint64_t size;
        std::string ext;
        std::vector<unsigned int> attrs;
        time_t mtime;

        bool operator==(const File &other) const {
            return (
                this->size == other.size &&
                this->ext == other.ext &&
                this->attrs == other.attrs &&
                this->mtime == other.mtime
            );
        }
    };

    typedef std::map<std::string, File> Dir;
    typedef std::map<std::string, Dir> Dirs;
    typedef std::map<std::string, Dirs> Shares;

    class SharesDB {
        static SharesDB *_this;
        //FIXME: This is silly and will have to go.
        std::function<void(void)> updateApp;
        std::vector<unsigned char> compressed;

        unsigned int getSingleValue(char *sql);
        unsigned int getCount(int type);

    protected:
        sqlite3 *db;

        SharesDB() { }
        void createDB();
        static int add(const char *path, const struct stat *st, int type, struct FTW *ftwbuf);
        /*!
         * Retrieves file attributes from database.
         * \param fn Path to file.
         * \param fe File structure to fill.
         * \return 0 on success, 1 on error.
         */
        int getAttrs(const std::string &fn, File *fe) const;
        /*!
         * Adds file to database.
         * Also used to update existing file entries.
         * \param dir Directory in which the file lies.
         * \param path Whole path (usually dir+fn).
         * \param st Statistics structure.
         * \param commit Do commit the changes immediately.
         */
        void addFile(const std::string &dir, const std::string &path, const struct stat &st, bool commit=true);
        /*!
         * Adds directory to database.
         * It is used to take care of empty dirs (we need to know about them).
         * \param path Whole path (usually dir+fn).
         * \param commit Do commit the changes immediately.
         */
        void addDir(const std::string &path, bool commit=true);

        template<typename T> void pack(std::vector<unsigned char> &data, T i);
        void pack(std::vector<unsigned char> &data, std::string s);
        void compress();

    public:
        SharesDB(const std::string &fn, std::function<void(void)> func);
        ~SharesDB();

        /*!
         * Adds given files/directories to the database.
         * Directories are scanned recursively.
         * Usually used to add directories through CLI.
         * \param paths Files/directories to add.
         */
        void add(std::initializer_list<const std::string> paths);
        /*!
         * Removes given files/directories from the database.
         * Usually used to remove directories through CLI.
         * \param paths Files/directories to remove.
         */
        void remove(std::initializer_list<const std::string> paths);
        /*!
         * Returns SLSK compatible, compressed version of the database.
         * \return Compressed DB entries.
         */
        inline const std::vector<unsigned char> &shares() const { return this->compressed; }
        /*!
         * Retrieves files contained within all directories
         * inside the given one.
         * \param fn Directory.
         * \return Files within fn.
         */
        Dirs contents(const std::string &fn);
        Dir query(const std::string &query) const;
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
