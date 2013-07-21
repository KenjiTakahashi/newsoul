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

#ifndef _DB_CXX_H_
#define _DB_CXX_H_

class DbEnv;
class DbTxn;

typedef enum {
    DB_HASH
} DBTYPE;

class Dbt {
public:
    Dbt();
    Dbt(void *data, unsigned int size);
    ~Dbt();
};

class Dbc {
public:
    int get(Dbt *key, Dbt *dat, unsigned int flags);
    int del(unsigned int flags);
    int close();
};

class Db {
public:
    Db(DbEnv *env, unsigned int flags);
    ~Db();
    int set_flags(unsigned int flags);
    int open(DbTxn *txn, const char *name, const char *dbn, DBTYPE type, unsigned int flags, int mode);
    int close(unsigned int flags);
    int put(DbTxn *txn, Dbt *key, Dbt *dat, unsigned int flags);
    int del(DbTxn *txn, Dbt *key, unsigned int flags);
    int cursor(DbTxn *txn, Dbc **cursorp, unsigned int flags);
    int exists(DbTxn *txn, Dbt *key, unsigned int flags);
    int stat(DbTxn *txn, void *sp, unsigned int flags);
};

#endif
