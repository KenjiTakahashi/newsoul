/*
 This is a part of newsoul @ http://github.com/KenjiTakahashi/newsoul
 Karol "Kenji Takahashi" Woźniak © 2014

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

#ifndef __MOCKS_SQLITE_H__
#define __MOCKS_SQLITE_H__

#include <initializer_list>
#include <sqlite3.h>
#include <string>
#include "../../src/sharesdb.h"

class TSharesDB : public newsoul::SharesDB {
public:
    TSharesDB() { }

    using newsoul::SharesDB::createDB;
    using newsoul::SharesDB::getAttrs;
    using newsoul::SharesDB::addFile;
    using newsoul::SharesDB::addDir;
    using newsoul::SharesDB::pack;
    using newsoul::SharesDB::compress;

    void setDB(sqlite3 *db) { this->db = db; }
};

namespace mocks {
    class SqliteMock {
        sqlite3 *db;

        void insertDIR(int n, int type, std::string base, int parentID=0);
        bool assertDIR(std::initializer_list<const std::string> paths, int type);

    public:
        SqliteMock(TSharesDB *sharesdb);

        void insertAttrs(const char *fn, int parentID=0);
        void insertAttrsDifferent(const char *fn);
        void insertFiles(int n);
        void insertFilesUpperCase(int n);
        void insertDirs(int n);
        void insertSubdirs(int n, int parentID);

        bool assertDirs(std::initializer_list<const std::string> paths);
        bool assertFiles(std::initializer_list<const std::string> files, bool props);
    };
}

#endif // __MOCKS_SQLITE_H__
