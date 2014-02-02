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

#include "sqlite.h"

void sqlite_trace(void *db, const char *sql) {
    printf("!TRACE!   %s\n", sql);
}

void sqlite_profile(void *db, const char *sql, sqlite3_uint64 time) {
    printf("!PROFILE! %s\n", sql);
    sqlite3_int64 rowid = sqlite3_last_insert_rowid((sqlite3*)db);
    printf("!PROFILE! last insert rowid: %lld\n", rowid);
}

mocks::SqliteMock::SqliteMock(TSharesDB *sharesdb) {
    //sqlite3_open("/home/kenji/tmp/d.db", &this->db);
    sqlite3_open(":memory:", &this->db);
    sharesdb->setDB(this->db);
    sharesdb->createDB();
    //sqlite3_trace(this->db, sqlite_trace, this->db);
    //sqlite3_profile(this->db, sqlite_profile, this->db);
}

void mocks::SqliteMock::insertAttrs(const char *fn, int parentID) {
    char *sql = sqlite3_mprintf(
        "BEGIN TRANSACTION;"
        "INSERT INTO DIR(path, parentID, type) "
        "VALUES(%Q, %d, 0);"
        "INSERT INTO FILE(dirID, size, ext, mtime, bitrate, length, vbr) "
        "VALUES(last_insert_rowid(), 20, 'ext', 600, 192, 10, 0);"
        "COMMIT TRANSACTION;"
        , fn, parentID
    );
    sqlite3_exec(this->db, sql, NULL, NULL, NULL);
}

void mocks::SqliteMock::insertAttrsDifferent(const char *fn) {
    char *sql = sqlite3_mprintf(
        "BEGIN TRANSACTION;"
        "INSERT INTO DIR(path, parentID, type) "
        "VALUES(%Q, 0, 0);"
        "INSERT INTO FILE(dirID, size, ext, mtime, bitrate, length, vbr) "
        "VALUES(last_insert_rowid(), 10, 'ext', 300, 10, 20, 0);"
        "COMMIT TRANSACTION;"
        , fn
    );
    sqlite3_exec(this->db, sql, NULL, NULL, NULL);
}

void mocks::SqliteMock::insertDIR(int n, int type, std::string base, int parentID) {
    char *sql = sqlite3_mprintf(
        "INSERT INTO DIR(path, parentID, type) "
        "VALUES (?, %d, %d); "
        , parentID, type
    );
    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(this->db, sql, -1, &stmt, NULL);
    for(int i = 0; i < n; ++i) {
        std::string path = base + std::to_string(i);
        sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_free(sql);
}

void mocks::SqliteMock::insertFiles(int n) {
    this->insertDIR(n, 0, std::string("/dir/file.ext"));
}

void mocks::SqliteMock::insertFilesUpperCase(int n) {
    this->insertDIR(n, 0, std::string("/Dir/File.ext"));
}

void mocks::SqliteMock::insertDirs(int n) {
    this->insertDIR(n, 1, std::string("/dir"));
}

void mocks::SqliteMock::insertSubdirs(int n, int parentID) {
    std::string parent = std::to_string(parentID - 1);
    this->insertDIR(n, 1, "/dir" + parent + "/subdir", parentID);
}

bool mocks::SqliteMock::assertDIR(std::initializer_list<const std::string> paths, int type) {
    char *sql = sqlite3_mprintf(
        "SELECT EXISTS(SELECT 1 FROM DIR WHERE path=? AND type=%d LIMIT 1);"
        , type
    );
    sqlite3_stmt *stmt;

    bool exists = true;
    sqlite3_prepare_v2(this->db, sql, -1, &stmt, NULL);
    for(const std::string &path : paths) {
        sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_STATIC);
        if(sqlite3_step(stmt) == SQLITE_ROW && !sqlite3_column_int(stmt, 0)) {
            exists = false;
            break;
        }
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);

    sqlite3_free(sql);
    return exists;
}

bool mocks::SqliteMock::assertDirs(std::initializer_list<const std::string> paths) {
    return this->assertDIR(paths, 1);
}

bool mocks::SqliteMock::assertFiles(std::initializer_list<const std::string> files, bool props) {
    if(!this->assertDIR(files, 0)) {
        return false;
    }

    std::string sql =
        "SELECT EXISTS(SELECT 1 FROM FILE WHERE "
        "dirID=(SELECT ID FROM DIR WHERE path=?) AND "
        "size=20 AND ext='ext' AND mtime=600 AND ";
    if(props) {
        sql += "bitrate=192 AND length=10 AND vbr=0);";
    } else {
        sql += "bitrate=0 AND length=0 AND vbr=0);";
    }
    sqlite3_stmt *stmt;

    bool exists = true;
    sqlite3_prepare_v2(this->db, sql.c_str(), -1, &stmt, NULL);
    for(const std::string &file : files) {
        sqlite3_bind_text(stmt, 1, file.c_str(), -1, SQLITE_STATIC);
        if(sqlite3_step(stmt) == SQLITE_ROW && !sqlite3_column_int(stmt, 0)) {
            exists = false;
            break;
        }
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);

    return exists;
}
