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

#include <CppUTestExt/MockSupport.h>
#include <cstring>
#include <db_cxx.h>

Dbt::Dbt() {
    mock().actualCall("Dbt::Dbt(0)");

    DBT *dbt = this;
    memset(dbt, 0, sizeof(DBT));
}

Dbt::Dbt(void *data_arg, u_int32_t size_arg) {
    mock().actualCall("Dbt::Dbt(2)").withParameter("1", (char*)data_arg).withParameter("2", (int)size_arg);

    DBT *dbt = this;
    memset(dbt, 0, sizeof(DBT));
    set_data(data_arg);
    set_size(size_arg);
}
Dbt::~Dbt() { }

int Dbc::close() {
    mock().actualCall("Dbc::close");
    return mock().intReturnValue();
}
int Dbc::cmp(Dbc *other_csr, int *result, u_int32_t flags) { return mock().intReturnValue(); }
int Dbc::count(db_recno_t *countp, u_int32_t flags) { return mock().intReturnValue(); }
int Dbc::del(u_int32_t flags) {
    mock().actualCall("Dbc::del");
    return mock().intReturnValue();
}
int Dbc::dup(Dbc** cursorp, u_int32_t flags) { return mock().intReturnValue(); }
int Dbc::get(Dbt* key, Dbt *data, u_int32_t flags) {
    if(mock().getData("Dbc::get::withParameter").getIntValue()) {
        mock().actualCall("Dbc::get").withParameter("1", (char*)key->get_data()).withParameter("2", (char*)data->get_data());
    } else {
        mock().actualCall("Dbc::get");
    }

    if(mock().getData("data").getIntValue()) {
        const char *k = (const char*)key->get_data();
        switch(k[strlen(k) - 1]) {
            case 's': {
                unsigned int d = mock().getData("sdata").getIntValue();
                data->set_data(&d);
                data->set_size(sizeof(d));
            } break;
            case 'e': {
                const char *d = mock().getData("edata").getStringValue();
                data->set_data(const_cast<char*>(d));
                data->set_size(sizeof(d));
            } break;
            case 'a': {
                unsigned int *d = (unsigned int*)mock().getData("adata").getPointerValue();
                data->set_data(d);
                data->set_size(sizeof(d));
            } break;
            case 'l': {
                unsigned int d = mock().getData("ldata").getIntValue();
                data->set_data(&d);
                data->set_size(sizeof(d));
            } break;
            case 'm': {
                time_t *d = (time_t*)mock().getData("mdata").getPointerValue();
                data->set_data(d);
                data->set_size(sizeof(d));
            } break;
            default:  // Should not happen
                break;
        }
        return 0;
    } else {
        return 1;  // Non-zero code
    }
}
int Dbc::get_priority(DB_CACHE_PRIORITY *priorityp) { return mock().intReturnValue(); }
int Dbc::pget(Dbt* key, Dbt* pkey, Dbt *data, u_int32_t flags) { return mock().intReturnValue(); }
int Dbc::put(Dbt* key, Dbt *data, u_int32_t flags) { return mock().intReturnValue(); }
int Dbc::set_priority(DB_CACHE_PRIORITY priority) { return mock().intReturnValue(); }


Db::Db(DbEnv *dbenv, u_int32_t flags) { }
Db::~Db() { }
int Db::associate(DbTxn *txn, Db *secondary, int (*callback)(Db *, const Dbt *, const Dbt *, Dbt *), u_int32_t flags) { return mock().intReturnValue(); }
int Db::associate_foreign(Db *foreign, int (*callback)(Db *, const Dbt *, Dbt *, const Dbt *, int *), u_int32_t flags) { return mock().intReturnValue(); }
int Db::close(u_int32_t flags) { return mock().intReturnValue(); }
int Db::compact(DbTxn *txnid, Dbt *start, Dbt *stop, DB_COMPACT *c_data, u_int32_t flags, Dbt *end) { return mock().intReturnValue(); }
int Db::cursor(DbTxn *txnid, Dbc **cursorp, u_int32_t flags) {
    mock().actualCall("Db::cursor");
    return mock().intReturnValue();
}
int Db::del(DbTxn *txnid, Dbt *key, u_int32_t flags) {
    mock().actualCall("Db::del").withParameter("2", (char*)key->get_data());
    return mock().intReturnValue();
}
void Db::err(int, const char *, ...) { }
void Db::errx(const char *, ...) { }
int Db::exists(DbTxn *txnid, Dbt *key, u_int32_t flags) {
    mock().actualCall("Db::exists");
    return mock().intReturnValue();
}
int Db::fd(int *fdp) { return mock().intReturnValue(); }
int Db::get(DbTxn *txnid, Dbt *key, Dbt *data, u_int32_t flags) {
    mock().actualCall("Db::get");
    return mock().intReturnValue();
}
int Db::get_alloc(db_malloc_fcn_type *, db_realloc_fcn_type *, db_free_fcn_type *) { return mock().intReturnValue(); }
int Db::get_append_recno(int (**)(Db *, Dbt *, db_recno_t)) { return mock().intReturnValue(); }
int Db::get_bt_compare(int (**)(Db *, const Dbt *, const Dbt *)) { return mock().intReturnValue(); }
int Db::get_bt_compress(int (**)(Db *, const Dbt *, const Dbt *, const Dbt *, const Dbt *, Dbt *), int (**)(Db *, const Dbt *, const Dbt *, Dbt *, Dbt *, Dbt *)) { return mock().intReturnValue(); }
int Db::get_bt_minkey(u_int32_t *) { return mock().intReturnValue(); }
int Db::get_bt_prefix(size_t (**)(Db *, const Dbt *, const Dbt *)) { return mock().intReturnValue(); }
int Db::get_byteswapped(int *) { return mock().intReturnValue(); }
int Db::get_cachesize(u_int32_t *, u_int32_t *, int *) { return mock().intReturnValue(); }
int Db::get_create_dir(const char **) { return mock().intReturnValue(); }
int Db::get_dbname(const char **, const char **) { return mock().intReturnValue(); }
int Db::get_dup_compare(int (**)(Db *, const Dbt *, const Dbt *)) { return mock().intReturnValue(); }
int Db::get_encrypt_flags(u_int32_t *) { return mock().intReturnValue(); }
void Db::get_errcall(void (**)(const DbEnv *, const char *, const char *)) { }
void Db::get_errfile(FILE **) { }
void Db::get_errpfx(const char **) { }
int Db::get_feedback(void (**)(Db *, int, int)) { return mock().intReturnValue(); }
int Db::get_flags(u_int32_t *) { return mock().intReturnValue(); }
#if DB_VERSION_MINOR >= 2
int Db::get_heapsize(u_int32_t *, u_int32_t *) { return mock().intReturnValue(); }
#endif
#if DB_VERSION_MINOR >= 3
int Db::get_heap_regionsize(u_int32_t *) { return mock().intReturnValue(); }
int Db::get_lk_exclusive(bool *, bool *) { return mock().intReturnValue(); }
#endif
int Db::get_h_compare(int (**)(Db *, const Dbt *, const Dbt *)) { return mock().intReturnValue(); }
int Db::get_h_ffactor(u_int32_t *) { return mock().intReturnValue(); }
int Db::get_h_hash(u_int32_t (**)(Db *, const void *, u_int32_t)) { return mock().intReturnValue(); }
int Db::get_h_nelem(u_int32_t *) { return mock().intReturnValue(); }
int Db::get_lorder(int *) { return mock().intReturnValue(); }
void Db::get_msgcall(void (**)(const DbEnv *, const char *)) { }
void Db::get_msgfile(FILE **) { }
int Db::get_multiple() { return mock().intReturnValue(); }
int Db::get_open_flags(u_int32_t *) { return mock().intReturnValue(); }
int Db::get_pagesize(u_int32_t *) { return mock().intReturnValue(); }
int Db::get_partition_callback(u_int32_t *, u_int32_t (**)(Db *, Dbt *key)) { return mock().intReturnValue(); }
int Db::get_partition_dirs(const char ***) { return mock().intReturnValue(); }
int Db::get_partition_keys(u_int32_t *, Dbt **) { return mock().intReturnValue(); }
int Db::get_priority(DB_CACHE_PRIORITY *) { return mock().intReturnValue(); }
int Db::get_q_extentsize(u_int32_t *) { return mock().intReturnValue(); }
int Db::get_re_delim(int *) { return mock().intReturnValue(); }
int Db::get_re_len(u_int32_t *) { return mock().intReturnValue(); }
int Db::get_re_pad(int *) { return mock().intReturnValue(); }
int Db::get_re_source(const char **) { return mock().intReturnValue(); }
int Db::get_transactional() { return mock().intReturnValue(); }
int Db::get_type(DBTYPE *) { return mock().intReturnValue(); }
int Db::join(Dbc **curslist, Dbc **dbcp, u_int32_t flags) { return mock().intReturnValue(); }
int Db::key_range(DbTxn *, Dbt *, DB_KEY_RANGE *, u_int32_t) { return mock().intReturnValue(); }
int Db::open(DbTxn *txnid, const char *, const char *subname, DBTYPE, u_int32_t, int) {
    mock().actualCall("Db::open");
    return mock().intReturnValue();
}
int Db::pget(DbTxn *txnid, Dbt *key, Dbt *pkey, Dbt *data, u_int32_t flags) { return mock().intReturnValue(); }
int Db::put(DbTxn *txnid, Dbt *key, Dbt *data, u_int32_t) {
    mock().actualCall("Db::put").withParameter("2", (char*)key->get_data()).withParameter("3", (char*)data->get_data());
    return mock().intReturnValue();
}
int Db::remove(const char *, const char *, u_int32_t) {
    mock().actualCall("Db::remove");
    return mock().intReturnValue();
}
int Db::rename(const char *, const char *, const char *, u_int32_t) { return mock().intReturnValue(); }
int Db::set_alloc(db_malloc_fcn_type, db_realloc_fcn_type, db_free_fcn_type) { return mock().intReturnValue(); }
void Db::set_app_private(void *) { }
int Db::set_append_recno(int (*)(Db *, Dbt *, db_recno_t)) { return mock().intReturnValue(); }
int Db::set_bt_compare(bt_compare_fcn_type) { return mock().intReturnValue(); }
int Db::set_bt_compare(int (*)(Db *, const Dbt *, const Dbt *)) { return mock().intReturnValue(); }
int Db::set_bt_compress(int (*) (Db *, const Dbt *, const Dbt *, const Dbt *, const Dbt *, Dbt *), int (*)(Db *, const Dbt *, const Dbt *, Dbt *, Dbt *, Dbt *)) { return mock().intReturnValue(); }
int Db::set_bt_minkey(u_int32_t) { return mock().intReturnValue(); }
int Db::set_bt_prefix(bt_prefix_fcn_type) { return mock().intReturnValue(); } /*deprecated*/
int Db::set_bt_prefix(size_t (*)(Db *, const Dbt *, const Dbt *)) { return mock().intReturnValue(); }
int Db::set_cachesize(u_int32_t, u_int32_t, int) { return mock().intReturnValue(); }
int Db::set_create_dir(const char *) { return mock().intReturnValue(); }
int Db::set_dup_compare(dup_compare_fcn_type) { return mock().intReturnValue(); } /*deprecated*/
int Db::set_dup_compare(int (*)(Db *, const Dbt *, const Dbt *)) { return mock().intReturnValue(); }
int Db::set_encrypt(const char *, u_int32_t) { return mock().intReturnValue(); }
void Db::set_errcall(void (*)(const DbEnv *, const char *, const char *)) { }
void Db::set_errfile(FILE *) { }
void Db::set_errpfx(const char *) { }
int Db::set_feedback(void (*)(Db *, int, int)) { return mock().intReturnValue(); }
int Db::set_flags(u_int32_t) { return mock().intReturnValue(); }
#if DB_VERSION_MINOR >= 2
int Db::set_heapsize(u_int32_t, u_int32_t) { return mock().intReturnValue(); }
#endif
#if DB_VERSION_MINOR >= 3
int Db::set_heap_regionsize(u_int32_t) { return mock().intReturnValue(); }
int Db::set_lk_exclusive(bool) { return mock().intReturnValue(); }
#endif
int Db::set_h_compare(h_compare_fcn_type) { return mock().intReturnValue(); }
int Db::set_h_compare(int (*)(Db *, const Dbt *, const Dbt *)) { return mock().intReturnValue(); }
int Db::set_h_ffactor(u_int32_t) { return mock().intReturnValue(); }
int Db::set_h_hash(h_hash_fcn_type) { return mock().intReturnValue(); }
int Db::set_h_hash(u_int32_t (*)(Db *, const void *, u_int32_t)) { return mock().intReturnValue(); }
int Db::set_h_nelem(u_int32_t) { return mock().intReturnValue(); }
int Db::set_lorder(int) { return mock().intReturnValue(); }
void Db::set_msgcall(void (*)(const DbEnv *, const char *)) { }
void Db::set_msgfile(FILE *) { }
int Db::set_pagesize(u_int32_t) { return mock().intReturnValue(); }
int Db::set_paniccall(void (*)(DbEnv *, int)) { return mock().intReturnValue(); }
int Db::set_partition(u_int32_t, Dbt *, u_int32_t (*)(Db *, Dbt *)) { return mock().intReturnValue(); }
int Db::set_partition_dirs(const char **) { return mock().intReturnValue(); }
int Db::set_priority(DB_CACHE_PRIORITY) { return mock().intReturnValue(); }
int Db::set_q_extentsize(u_int32_t) { return mock().intReturnValue(); }
int Db::set_re_delim(int) { return mock().intReturnValue(); }
int Db::set_re_len(u_int32_t) { return mock().intReturnValue(); }
int Db::set_re_pad(int) { return mock().intReturnValue(); }
int Db::set_re_source(const char *) { return mock().intReturnValue(); }
int Db::sort_multiple(Dbt *, Dbt *, u_int32_t) { return mock().intReturnValue(); }
int Db::stat(DbTxn *, void *sp, u_int32_t flags) {
    mock().actualCall("Db::stat");
    return mock().intReturnValue();
}
int Db::stat_print(u_int32_t flags) { return mock().intReturnValue(); }
int Db::sync(u_int32_t flags) { return mock().intReturnValue(); }
int Db::truncate(DbTxn *, u_int32_t *, u_int32_t) { return mock().intReturnValue(); }
int Db::upgrade(const char *name, u_int32_t flags) { return mock().intReturnValue(); }
int Db::verify(const char *, const char *, __DB_STD(ostream) *, u_int32_t) { return mock().intReturnValue(); }

void *Db::get_app_private() const { return mock().pointerReturnValue(); }
__DB_STD(ostream) *Db::get_error_stream() { return (__DB_STD(ostream)*)mock().pointerReturnValue(); }
void Db::set_error_stream(__DB_STD(ostream) *) { }
__DB_STD(ostream) *Db::get_message_stream() { return (__DB_STD(ostream)*)mock().pointerReturnValue(); }
void Db::set_message_stream(__DB_STD(ostream) *) { }

DbEnv *Db::get_env() { return (DbEnv*)mock().pointerReturnValue(); }
DbMpoolFile *Db::get_mpf() { return (DbMpoolFile*)mock().pointerReturnValue(); }
