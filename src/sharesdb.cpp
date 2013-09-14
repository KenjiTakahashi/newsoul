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

#include "sharesdb.h"

newsoul::SharesDB *newsoul::SharesDB::_this;

newsoul::SharesDB::SharesDB(const std::string &fn, std::function<void(void)> func) : dirsdb(NULL, DB_CXX_NO_EXCEPTIONS), attrdb(NULL, DB_CXX_NO_EXCEPTIONS) {
    this->updateApp = func;
    const std::string efn = path::expand(fn);
    os::_mkdir(efn);
    std::string dfn = path::join({efn, "dirs.db"});
    std::string afn = path::join({efn, "attr.db"});
    this->dirsdb.set_flags(DB_DUPSORT);
    //TODO: Handle errors. Really
    this->dirsdb.open(NULL, afn.c_str(), NULL, DB_HASH, DB_CREATE, 0);
    this->attrdb.open(NULL, dfn.c_str(), NULL, DB_HASH, DB_CREATE, 0);
    this->compress();
    this->fw.watch();
}

newsoul::SharesDB::~SharesDB() {
    this->attrdb.close(0);
    this->dirsdb.close(0);
}

int newsoul::SharesDB::add(const char *path, const struct stat *st, int type, struct FTW *ftwbuf) {
    std::string dir(path, 0, ftwbuf->base - 1);
    struct stat *mst = const_cast<struct stat*>(st);
    switch(type) {
        case FTW_F:
            SharesDB::_this->addFile(dir, path + ftwbuf->base, path, *mst);
            break;
        case FTW_D:
            SharesDB::_this->addDir(dir, path + ftwbuf->base, path);
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
    struct stat st;

    for(auto path : paths) {
        const char *path_c = path.c_str();
        stat(path_c, &st);

        if(S_ISREG(st.st_mode)) {
            unsigned int index = path.find_last_of(os::separator());
            this->removeFile(path.substr(0, index), path.substr(index), path);
        } else if(S_ISDIR(st.st_mode)) {
            this->removeDir(path);
        } else {
            //TODO: report error
        }
    }
}

void newsoul::SharesDB::addFile(const std::string &dir, const std::string &fn, const std::string &path, struct stat &st) {
    std::vector<int> attrs;
    unsigned int size = 0;
    std::string ext;
    TagLib::FileRef fp(path.c_str());

    if(!fp.isNull() && fp.audioProperties()) {
        TagLib::AudioProperties *props = fp.audioProperties();

        ext = string::tolower(fn.substr(fn.rfind('.') + 1));
        size = 3;
        attrs.push_back(props->bitrate());
        attrs.push_back(props->length());
        attrs.push_back(0); //FIXME: vbr
    }

    std::string e = path + "e";
    Dbt extkey(const_cast<char*>(e.c_str()), e.size() + 1);
    Dbt extdat(const_cast<char*>(ext.c_str()), ext.size() + 1);

    std::string l = path + "l";
    Dbt asizekey(const_cast<char*>(l.c_str()), l.size() + 1);
    Dbt asizedat(&size, sizeof(unsigned int));

    std::string a = path + "a";
    Dbt attrskey(const_cast<char*>(a.c_str()), a.size() + 1);
    Dbt attrsdat(attrs.data(), attrs.size() * sizeof(unsigned int));

    std::string s = path + "s";
    Dbt sizekey(const_cast<char*>(s.c_str()), path.size() + 2);
    Dbt sizedat(&st.st_size, sizeof(unsigned int));

    std::string m = path + "m";
    Dbt mtimekey(const_cast<char*>(m.c_str()), path.size() + 2);
    Dbt mtimedat(&st.st_mtime, sizeof(time_t));

    this->attrdb.put(NULL, &extkey, &extdat, 0);
    this->attrdb.put(NULL, &asizekey, &asizedat, 0);
    this->attrdb.put(NULL, &attrskey, &attrsdat, 0);
    this->attrdb.put(NULL, &sizekey, &sizedat, 0);
    this->attrdb.put(NULL, &mtimekey, &mtimedat, 0);

    Dbt dkey(const_cast<char*>(dir.c_str()), dir.size() + 1);
    Dbt ddat(const_cast<char*>(fn.c_str()), fn.size() + 1);
    this->dirsdb.put(NULL, &dkey, &ddat, DB_OVERWRITE_DUP);
}

void newsoul::SharesDB::removeFile(const std::string &dir, const std::string &fn, const std::string &path) {
    for(auto k : {"e", "l", "a", "s", "m"}) {
        std::string key_s = path + k;
        Dbt key(const_cast<char*>(key_s.c_str()), key_s.size() + 1);
        this->attrdb.del(NULL, &key, 0);
    }

    Dbt dkey(const_cast<char*>(dir.c_str()), dir.size() + 1);
    Dbt ddat(const_cast<char*>(fn.c_str()), fn.size() + 1);
    Dbc *cursor;

    this->dirsdb.cursor(NULL, &cursor, 0);
    cursor->get(&dkey, &ddat, DB_GET_BOTH);
    cursor->del(0);

    cursor->close();
}

void newsoul::SharesDB::addDir(const std::string &dir, const std::string &fn, const std::string &path) {
    Dbt key(const_cast<char*>(dir.c_str()), dir.size() + 1);
    Dbt dat(const_cast<char*>(fn.c_str()), fn.size() + 1);
    Dbt pkey(const_cast<char*>(path.c_str()), path.size() + 1);
    std::string empty = "";
    Dbt pdat(const_cast<char*>(empty.c_str()), 1);

    this->dirsdb.put(NULL, &key, &dat, DB_OVERWRITE_DUP);
    this->dirsdb.put(NULL, &pkey, &pdat, DB_OVERWRITE_DUP);
}

void newsoul::SharesDB::removeDir(const std::string &path) {
    Dbc *cursor;
    Dbt key;
    struct stat st;

    std::queue<std::string> queue;
    std::string k;

    std::string spath(path);
    if(spath[spath.length() - 1] == os::separator()) {
        spath.resize(spath.length() - 1);
    }

    queue.push(spath);
    this->dirsdb.cursor(NULL, &cursor, 0);
    while(!queue.empty()) {
        k = queue.front();

        key.set_data(const_cast<char*>(k.c_str()));
        key.set_size(k.size() + 1);

        Dbt dat;
        int ret = cursor->get(&key, &dat, DB_SET);
        while(ret != DB_NOTFOUND) {
            std::string d((char*)dat.get_data());

            if(!d.empty()) {
                std::string kd = path::join({k, d});

                stat(kd.c_str(), &st);
                if(S_ISREG(st.st_mode)) {
                    this->removeFile(k, d, kd);
                } else if(S_ISDIR(st.st_mode)) {
                    queue.push(kd);
                }
            }

            cursor->del(0);
            ret = cursor->get(&key, &dat, DB_NEXT_DUP);
        }

        queue.pop();
    }
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
    Dbc *cursor1, *cursor2;
    Dbt key, dat1;
    struct stat st;
    std::vector<unsigned char> inBuf;

    this->dirsdb.cursor(NULL, &cursor1, 0);
    this->dirsdb.cursor(NULL, &cursor2, 0);
    this->pack<uint32_t>(inBuf, this->dirsCount());
    while(cursor1->get(&key, &dat1, DB_NEXT_NODUP) != DB_NOTFOUND) {
        std::string path((char*)key.get_data());
        this->pack<uint32_t>(inBuf, path.size());
        for(const char &c : path) {
            if(c == '/') {
                inBuf.push_back('\\');
            } else {
                inBuf.push_back(c);
            }
        }

        Dbt dat2;
        int ret = cursor2->get(&key, &dat2, DB_SET);
        ret = cursor2->get(&key, &dat2, DB_NEXT_DUP);
        unsigned int length = 0;
        while(ret != DB_NOTFOUND) {
            length += 1;
            ret = cursor2->get(&key, &dat2, DB_NEXT_DUP);
        }
        this->pack<uint32_t>(inBuf, length);

        Dbt dat3;
        ret = cursor2->get(&key, &dat3, DB_SET);
        while(ret != DB_NOTFOUND) {
            std::string file((char*)dat3.get_data());
            std::string fpath = path::join({path, file});
            stat(fpath.c_str(), &st);
            if(S_ISREG(st.st_mode)) {
                File fe;
                if(this->getAttrs(fpath, &fe) == 0) {
                    inBuf.push_back(1);
                    this->pack(inBuf, file);
                    this->pack<uint64_t>(inBuf, fe.size);
                    this->pack(inBuf, fe.ext);
                    this->pack<uint32_t>(inBuf, fe.attrs.size());
                    auto ait = fe.attrs.begin();
                    for(unsigned int j = 0; ait != fe.attrs.end(); ++ait) {
                        this->pack<uint32_t>(inBuf, j++);
                        this->pack<uint32_t>(inBuf, *ait);
                    }
                }
            }
            ret = cursor2->get(&key, &dat3, DB_NEXT_DUP);
        }
    }

    uLong outLen = compressBound(inBuf.size());
    char *outBuf = new char[outLen];
    if(::compress((Bytef*)outBuf, &outLen, (Bytef*)inBuf.data(), inBuf.size()) == Z_OK) {
        this->compressed = std::vector<unsigned char>(outBuf, outBuf + outLen);
    } else {
        //TODO: report error
    }

    delete [] outBuf;
}

void newsoul::SharesDB::handleFileAction(efsw::WatchID wid, const std::string &dir, const std::string &fn, efsw::Action action, std::string oldFn="") {
    std::string path = path::join({dir, fn});
    struct stat st;
    stat(path.c_str(), &st);

    switch(action) {
        case efsw::Actions::Add:
            if(S_ISREG(st.st_mode)) {
                this->addFile(dir, fn, path, st);
            } else if(S_ISDIR(st.st_mode)) {
                this->addDir(dir, fn, path);
            }
            break;
        case efsw::Actions::Delete:
            if(S_ISREG(st.st_mode)) {
                this->removeFile(dir, fn, path);
            } else if(S_ISDIR(st.st_mode)) {
                this->removeDir(path);
            }
            break;
        case efsw::Actions::Modified:
            if(S_ISREG(st.st_mode)) {
                this->addFile(dir, fn, path, st);
            }
            break;
        case efsw::Actions::Moved:
            //FIXME: implement, waiting for efsw
            if(S_ISREG(st.st_mode)) {
            } else if(S_ISDIR(st.st_mode)) {
            }
            break;
    }

    this->compress();
    this->updateApp();
}

int newsoul::SharesDB::getAttrs(const std::string &fn, File *fe) {
    Dbc *cursor;
    Dbt dat;
    std::string s = fn + "s";
    Dbt sizekey(const_cast<char*>(s.c_str()), fn.size() + 2);
    std::string e = fn + "e";
    Dbt extkey(const_cast<char*>(e.c_str()), fn.size() + 2);
    std::string l = fn + "l";
    Dbt asizekey(const_cast<char*>(l.c_str()), fn.size() + 2);
    std::string a = fn + "a";
    Dbt attrskey(const_cast<char*>(a.c_str()), fn.size() + 2);
    std::string m = fn + "m";
    Dbt mtimekey(const_cast<char*>(m.c_str()), fn.size() + 2);

    this->attrdb.cursor(NULL, &cursor, 0);
    if(cursor->get(&sizekey, &dat, DB_SET) != 0) {
        cursor->close();
        return 1;
    }
    fe->size = *(unsigned int*)dat.get_data();
    if(cursor->get(&extkey, &dat, DB_SET) != 0) {
        cursor->close();
        return 1;
    }
    fe->ext = std::string((char*)dat.get_data());
    if(cursor->get(&asizekey, &dat, DB_SET) != 0) {
        cursor->close();
        return 1;
    }
    unsigned int len = *(unsigned int*)dat.get_data();
    if(cursor->get(&attrskey, &dat, DB_SET) != 0) {
        cursor->close();
        return 1;
    }
    unsigned int *attrs = (unsigned int*)dat.get_data();
    fe->attrs = std::vector<unsigned int>(attrs, attrs + len);
    if(cursor->get(&mtimekey, &dat, DB_SET) != 0) {
        cursor->close();
        return 1;
    }
    fe->mtime = *(time_t*)dat.get_data();
    cursor->close();

    return 0;
}

newsoul::Dirs newsoul::SharesDB::contents(const std::string &fn) {
    Dirs results;
    Dbc *cursor;
    Dbt key, dat;
    struct stat st;

    std::queue<std::string> queue;
    std::string k;

    queue.push(fn);
    this->dirsdb.cursor(NULL, &cursor, 0);
    while(!queue.empty()) {
        k = queue.front();

        key.set_data(const_cast<char*>(k.c_str()));
        key.set_size(k.size() + 1);

        while(cursor->get(&key, &dat, DB_NEXT) == 0) {
            results[k];

            std::string d((char*)dat.get_data());

            stat(d.c_str(), &st);
            if(S_ISREG(st.st_mode)) {
                File fe;
                if(this->getAttrs(path::join({k, d}), &fe) == 0) {
                    results[k][d] = fe;
                }
            }

            if(!d.empty()) {
                queue.push(d);
            }
        }

        queue.pop();
    }
    cursor->close();

    return results;
}

newsoul::Dir newsoul::SharesDB::query(const std::string &query) const {
}

std::string newsoul::SharesDB::toProperCase(const std::string &lower) {
    Dbc *cursor;
    Dbt key, dat;

    this->attrdb.cursor(NULL, &cursor, 0);
    while(cursor->get(&key, &dat, DB_NEXT) == 0) {
        std::string s((char*)key.get_data());
        if(string::tolower(s) == lower) {
            return s;
        }
    }
    cursor->close();

    return std::string();
}

bool newsoul::SharesDB::isShared(const std::string &fn) {
    Dbt key(const_cast<char*>(fn.c_str()), fn.size() + 1);

    return this->attrdb.exists(NULL, &key, 0) != DB_NOTFOUND;
}

unsigned int newsoul::SharesDB::filesCount() {
    //FIXME: This might be slow
    DB_HASH_STAT *stats = (DB_HASH_STAT*)malloc(sizeof(DB_HASH_STAT));
    this->attrdb.stat(NULL, &stats, 0);
    unsigned int res = stats->hash_nkeys / 5;
    free(stats);
    return res;
}

unsigned int newsoul::SharesDB::dirsCount() {
    //FIXME: This might be slow
    DB_HASH_STAT *stats = (DB_HASH_STAT*)malloc(sizeof(DB_HASH_STAT));
    this->dirsdb.stat(NULL, &stats, 0);
    unsigned int res = stats->hash_nkeys;
    free(stats);
    return res;
}
