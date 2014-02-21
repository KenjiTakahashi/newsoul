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

#include "sharesdb.h"

newsoul::SharesDB *newsoul::SharesDB::_this;

newsoul::SharesDB::SharesDB(const std::string &fn, std::function<void(void)> func) {
    this->updateApp = func;
    const std::string efn = path::expand(fn);
    os::_mkdir(efn);
    std::string dfn = path::join({efn, "shares.db"});
    int res = sqlite3_open(dfn.c_str(), &this->db);
    this->createDB();
    //TODO: Handle errors. Really
    this->compress();
}

newsoul::SharesDB::~SharesDB() {
    sqlite3_close(this->db);
}

void newsoul::SharesDB::createDB() {
    int res = sqlite3_exec(this->db,
        "PRAGMA foreign_keys = ON; "
        "PRAGMA recursive_triggers = ON; "

        "CREATE TABLE IF NOT EXISTS DIR( "
        "ID INTEGER PRIMARY KEY, "
        "path TEXT NOT NULL UNIQUE, "
        "parentID INTEGER, "
        "type INTEGER NOT NULL); "
        //type: 0 - file, 1 - directory

        "CREATE TABLE IF NOT EXISTS CLOSURE( "
        "parentID INTEGER NOT NULL, "
        "childID INTEGER NOT NULL, "
        "depth INTEGER NOT NULL, "
        "PRIMARY KEY(parentID, childID), "
        "UNIQUE(parentID, depth, childID), "
        "UNIQUE(childID, parentID, depth)); "

        "CREATE TABLE IF NOT EXISTS FILE( "
        "dirID INTEGER PRIMARY KEY, "
        "size INTEGER, "
        "ext CHAR(5), "
        "mtime INTEGER, "
        "bitrate INTEGER, "
        "length INTEGER, "
        "vbr INTEGER, "
        "FOREIGN KEY(dirID) REFERENCES DIR(ID) "
        "ON DELETE CASCADE ON UPDATE CASCADE); "

        "CREATE TRIGGER insert_closure AFTER INSERT ON DIR "
        "BEGIN "
        "INSERT INTO CLOSURE(parentID, childID, depth) "
        "VALUES(new.ID, new.ID, 0);"
        "INSERT INTO CLOSURE(parentId, childID, depth) "
        "SELECT parent.parentID, child.childID, parent.depth+child.depth+1 "
        "FROM CLOSURE parent, CLOSURE child "
        "WHERE parent.childID=new.parentID and child.parentID=new.ID; "
        "END; "

        "CREATE TRIGGER delete_closure AFTER DELETE ON DIR "
        "BEGIN "
        "DELETE FROM CLOSURE WHERE childID=old.ID; "
        "DELETE FROM DIR "
        "WHERE ID in (SELECT childID FROM CLOSURE WHERE parentID=old.ID); "
        "END; "
    , NULL, NULL, NULL);
}

int newsoul::SharesDB::add(const char *path, const struct stat *st, int type, struct FTW *ftwbuf) {
    std::string dir(path, 0, ftwbuf->base - 1);
    switch(type) {
        case FTW_F:
            SharesDB::_this->addFile(dir, path + ftwbuf->base, *st);
            break;
        case FTW_D:
            SharesDB::_this->addDir(path);
            break;
        default: //TODO: report error
            break;
    }
    return 0;
}

void newsoul::SharesDB::add(std::initializer_list<const std::string> paths) {
    SharesDB::_this = this;

    for(auto path : paths) {
        nftw(path.c_str(), SharesDB::add, 20, 0);
    }
}

void newsoul::SharesDB::remove(std::initializer_list<const std::string> paths) {
    char *sql;

    for(auto path : paths) {
        sql = sqlite3_mprintf("DELETE FROM DIR WHERE path=%Q;", path.c_str());
        int res = sqlite3_exec(this->db, sql, NULL, NULL, NULL);
        sqlite3_free(sql);
    }
}

void newsoul::SharesDB::addFile(const std::string &dir, const std::string &path, const struct stat &st) {
    std::string ext = string::tolower(path.substr(path.rfind('.') + 1));
    int bitrate = 0;
    int length = 0;
    int vbr = 0;

    TagLib::FileRef fp(path.c_str());
    if(!fp.isNull() && fp.audioProperties()) {
        TagLib::AudioProperties *props = fp.audioProperties();
        bitrate = props->bitrate();
        length = props->length();
    }

    this->addDir(dir);

    char *sql = sqlite3_mprintf(
        "BEGIN TRANSACTION; "
        "INSERT OR REPLACE INTO DIR(path, parentID, type) "
        "VALUES(%Q, COALESCE((SELECT ID FROM DIR WHERE path=%Q), 0), 0); "
        "INSERT OR REPLACE INTO "
        "FILE(dirID, size, ext, mtime, bitrate, length, vbr) "
        "VALUES(last_insert_rowid(), %d, %Q, %d, %d, %d, %d); "
        "COMMIT TRANSACTION; "
        , path.c_str(), dir.c_str()
        , st.st_size, ext.c_str(), st.st_mtime, bitrate, length, vbr
    );
    int res = sqlite3_exec(this->db, sql, NULL, NULL, NULL);
    sqlite3_free(sql);
}

void newsoul::SharesDB::addDir(const std::string &path) {
    char *sql = sqlite3_mprintf(
        "INSERT INTO DIR(path, parentID, type) "
        "VALUES(?, COALESCE((SELECT ID FROM DIR WHERE path=?), 0), 1); "
    );
    sqlite3_stmt *stmt;

    //TODO: REFACTOR
    std::vector<std::string> pieces = string::split(path, "/"); //TODO: Write proper path:split
    std::string recreated_path = "/" + pieces[0]; //FIXME: Use proper separator
    int res = sqlite3_exec(this->db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
    res = sqlite3_prepare_v2(this->db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, recreated_path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_null(stmt, 2);
    res = sqlite3_step(stmt);
    for(unsigned int i = 1; i < pieces.size(); ++i) {
        sqlite3_reset(stmt);
        std::string parent(recreated_path);
        sqlite3_bind_text(stmt, 2, parent.c_str(), -1, SQLITE_STATIC);
        recreated_path = path::join({recreated_path, pieces[i]});
        sqlite3_bind_text(stmt, 1, recreated_path.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    res = sqlite3_exec(this->db, "COMMIT TRANSACTION;", NULL, NULL, NULL);
    sqlite3_free(sql);
}

template<typename T> void newsoul::SharesDB::pack(std::vector<unsigned char> &data, T i) {
    for(unsigned int j = 0; j < sizeof(i); ++j) {
        data.push_back(i & 0xff);
        i >>= 8;
    }
}

void newsoul::SharesDB::pack(std::vector<unsigned char> &data, std::string s) {
    this->pack<uint32_t>(data, s.size());
    for(const char &c : s) {
        data.push_back(c);
    }
}

void newsoul::SharesDB::compress() {
    const char *sql1 = "SELECT ID, path FROM DIR WHERE type=1";
    const char *sql2 =
        "SELECT COUNT(*) FROM DIR AS d JOIN CLOSURE AS c ON d.ID=c.childID "
        "WHERE c.parentID=? AND d.type=0 AND c.depth=1; ";
    const char *sql3 =
        "SELECT d.path, f.* FROM DIR AS d JOIN FILE AS f ON d.ID=f.dirID "
        "WHERE d.parentID=?; ";
    sqlite3_stmt *stmt1;
    sqlite3_stmt *stmt2;
    sqlite3_stmt *stmt3;

    std::vector<unsigned char> inBuf;
    this->pack<uint32_t>(inBuf, this->dirsCount());
    sqlite3_prepare_v2(this->db, sql1, -1, &stmt1, NULL);
    sqlite3_prepare_v2(this->db, sql2, -1, &stmt2, NULL);
    sqlite3_prepare_v2(this->db, sql3, -1, &stmt3, NULL);
    while(sqlite3_step(stmt1) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt1, 0);
        const unsigned char *p = sqlite3_column_text(stmt1, 1);
        std::string path(reinterpret_cast<const char*>(p));
        this->pack<uint32_t>(inBuf, path.size());
        for(const char &c : path) {
            if(c == '/') {
                inBuf.push_back('\\');
            } else {
                inBuf.push_back(c);
            }
        }

        sqlite3_bind_int(stmt2, 1, id);
        if(sqlite3_step(stmt2) == SQLITE_ROW) {
            this->pack<uint32_t>(inBuf, sqlite3_column_int(stmt2, 0));
        }
        sqlite3_reset(stmt2);

        sqlite3_bind_int(stmt3, 1, id);
        while(sqlite3_step(stmt3) == SQLITE_ROW) {
            const unsigned char *p = sqlite3_column_text(stmt3, 0);
            const std::string fpath(reinterpret_cast<const char*>(p));
            //FIXME: Implement path::split
            std::string basename = fpath.substr(fpath.rfind('/') + 1);

            inBuf.push_back(1);
            this->pack(inBuf, basename);
            this->pack<uint64_t>(inBuf, sqlite3_column_int64(stmt3, 2));
            const unsigned char *e = sqlite3_column_text(stmt3, 3);
            std::string ext(reinterpret_cast<const char*>(e));
            this->pack(inBuf, ext);
            this->pack<uint32_t>(inBuf, 3);
            this->pack<uint32_t>(inBuf, 0);
            this->pack<uint32_t>(inBuf, sqlite3_column_int(stmt3, 5));
            this->pack<uint32_t>(inBuf, 1);
            this->pack<uint32_t>(inBuf, sqlite3_column_int(stmt3, 6));
            this->pack<uint32_t>(inBuf, 2);
            this->pack<uint32_t>(inBuf, sqlite3_column_int(stmt3, 7));
        }
        sqlite3_reset(stmt3);
    }
    sqlite3_finalize(stmt3);
    sqlite3_finalize(stmt2);
    sqlite3_finalize(stmt1);

    uLong outLen = compressBound(inBuf.size());
    char *outBuf = new char[outLen];
    if(::compress((Bytef*)outBuf, &outLen, (Bytef*)inBuf.data(), inBuf.size()) == Z_OK) {
        this->compressed = std::vector<unsigned char>(outBuf, outBuf + outLen);
    } else {
        //TODO: report error
    }

    delete [] outBuf;
}

int newsoul::SharesDB::getAttrs(const std::string &fn, File *fe) const {
    char *sql = sqlite3_mprintf(
        "SELECT * FROM FILE WHERE dirID=("
        "SELECT ID FROM DIR WHERE path=%Q)"
        "LIMIT 1;"
        , fn.c_str()
    );
    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(this->db, sql, -1, &stmt, NULL);
    sqlite3_free(sql);
    if(sqlite3_step(stmt) == SQLITE_ROW) {
        fe->size = sqlite3_column_int(stmt, 1);
        const unsigned char *ext = sqlite3_column_text(stmt, 2);
        fe->ext = std::string(reinterpret_cast<const char*>(ext));
        fe->mtime = sqlite3_column_int(stmt, 3);
        fe->attrs = std::vector<unsigned int>({
            (unsigned int)sqlite3_column_int(stmt, 4),
            (unsigned int)sqlite3_column_int(stmt, 5),
            (unsigned int)sqlite3_column_int(stmt, 6)
        });
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_finalize(stmt);
    return 1;
}

newsoul::Dirs newsoul::SharesDB::contents(const std::string &fn) {
    char *sql1 = sqlite3_mprintf(
        "SELECT d.* FROM DIR AS d JOIN CLOSURE AS c ON d.ID=c.childID "
        "WHERE c.parentID=(SELECT ID FROM DIR WHERE path=%Q) "
        "ORDER BY d.type DESC; "
        , fn.c_str()
    );
    const char *sql2 = "SELECT * FROM FILE WHERE path=?;";
    sqlite3_stmt *stmt1;
    sqlite3_stmt *stmt2;

    Dirs results;
    sqlite3_prepare_v2(this->db, sql1, -1, &stmt1, NULL);
    sqlite3_prepare_v2(this->db, sql2, -1, &stmt2, NULL);
    while(sqlite3_step(stmt1) == SQLITE_ROW) {
        int type = sqlite3_column_int(stmt1, 3);
        const unsigned char *p = sqlite3_column_text(stmt1, 1);
        const std::string path(reinterpret_cast<const char*>(p));
        if(type == 0) {
            //FIXME: Implement path::split
            int index = path.rfind('/');
            std::string basedir = path.substr(0, index);
            std::string basename = path.substr(index + 1);

            File fe;
            if(this->getAttrs(path, &fe) == 0) {
                results[basedir][basename] = fe;
            }
        } else {
            results[path];
        }
    }

    sqlite3_free(sql1);
    return results;
}

newsoul::Dir newsoul::SharesDB::query(const std::string &query) const {
    //This implementation does not follow soulseek conventions on
    //quotes, because they honestly make no sense.
    //
    //Instead, it aims at providing result sets in fashion similar
    //to "normal" search engines, e.g. Google.
    std::string sql("SELECT path FROM DIR WHERE type=0");
    sqlite3_stmt *stmt;

    bool phrase = false;
    for(std::string partial : string::split(query, " ")) {
        if(!phrase) {
            sql += " AND path ";
            if(partial[0] == '-') {
                partial = partial.substr(1);
                sql += "NOT ";
            }
            sql += "LIKE '%";
            if(partial[0] == '"') {
                partial = partial.substr(1);
                phrase = true;
            }
            if(partial[partial.length() - 1] == '"') {
                partial = partial.substr(0, partial.length() - 1);
                phrase = false;
            }
        }
        if(phrase && partial[partial.length() - 1] == '"') {
            partial = partial.substr(0, partial.length() - 1);
            sql += " ";
            phrase = false;
        }
        sql += partial;
        if(!phrase) {
            sql += "%'";
        }
    }

    Dir results;
    sqlite3_prepare_v2(this->db, sql.c_str(), -1, &stmt, NULL);
    while(sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *p = sqlite3_column_text(stmt, 0);
        const std::string path(reinterpret_cast<const char*>(p));
        File fe;
        this->getAttrs(path, &fe);
        results[path] = fe;
    }

    return results;
}

std::string newsoul::SharesDB::toProperCase(const std::string &lower) {
    char *sql = sqlite3_mprintf(
        "SELECT path FROM DIR WHERE LOWER(path)=%Q;"
        , lower.c_str()
    );
    sqlite3_stmt *stmt;

    std::string result;
    sqlite3_prepare_v2(this->db, sql, -1, &stmt, NULL);
    if(sqlite3_step(stmt) == SQLITE_ROW) {
        result = std::string(
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))
        );
    }
    sqlite3_finalize(stmt);

    sqlite3_free(sql);
    return result;
}

unsigned int newsoul::SharesDB::getSingleValue(char *sql) {
    sqlite3_stmt *stmt;

    int value = 0;
    sqlite3_prepare_v2(this->db, sql, -1, &stmt, NULL);
    if(sqlite3_step(stmt) == SQLITE_ROW) {
        value = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    sqlite3_free(sql);
    return value;
}

unsigned int newsoul::SharesDB::getCount(int type) {
    char *sql = sqlite3_mprintf(
        "SELECT COUNT(*) FROM DIR WHERE type=%d;", type
    );

    return this->getSingleValue(sql);
}

bool newsoul::SharesDB::isShared(const std::string &fn) {
    char *sql = sqlite3_mprintf(
        "SELECT EXISTS(SELECT 1 FROM DIR WHERE path=%Q LIMIT 1);"
        , fn.c_str()
    );

    return this->getSingleValue(sql);
}

unsigned int newsoul::SharesDB::filesCount() {
    return this->getCount(0);
}

unsigned int newsoul::SharesDB::dirsCount() {
    return this->getCount(1);
}
