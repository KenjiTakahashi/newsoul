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

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestRegistry.h>
#include <CppUTestExt/MockSupport.h>
#include <CppUTestExt/MockSupportPlugin.h>
#include <sys/stat.h>
#include "../src/sharesdb.h"

class TSharesDB : public newsoul::SharesDB {
    public:
        TSharesDB() { }

        using newsoul::SharesDB::getAttrs;
        using newsoul::SharesDB::addFile;
        using newsoul::SharesDB::removeFile;
        using newsoul::SharesDB::addDir;
        using newsoul::SharesDB::removeDir;
        using newsoul::SharesDB::pack;
        using newsoul::SharesDB::compress;
};

TEST_GROUP(getAttrs) {
    void setup() {
        mock().expectOneCall("Dbt::Dbt(0)");
        mock().expectOneCall("Dbt::Dbt(2)").withParameter("1", "/entrys").withParameter("2", 8);
        mock().expectOneCall("Dbt::Dbt(2)").withParameter("1", "/entrye").withParameter("2", 8);
        mock().expectOneCall("Dbt::Dbt(2)").withParameter("1", "/entrya").withParameter("2", 8);
        mock().expectOneCall("Dbt::Dbt(2)").withParameter("1", "/entryl").withParameter("2", 8);
        mock().expectOneCall("Dbt::Dbt(2)").withParameter("1", "/entrym").withParameter("2", 8);
        mock().expectOneCall("Db::cursor");
    }
};

TEST(getAttrs, entry_exists) {
    TSharesDB shares;
    mock().setData("data", 1);
    mock().setData("sdata", 10);
    mock().setData("edata", "ext");
    unsigned int a[3] = {10, 20, 30};
    mock().setData("adata", a);
    mock().setData("ldata", 3);
    time_t t = 300;
    mock().setData("mdata", &t);

    mock().expectNCalls(5, "Dbc::get");
    mock().expectOneCall("Dbc::close");

    FileEntry result;
    int ret = shares.getAttrs("/entry", &result);

    CHECK_EQUAL(0, ret);
    CHECK_EQUAL(10, result.size);
    CHECK_EQUAL("ext", result.ext);
    CHECK(std::vector<unsigned int>({10, 20, 30}) == result.attrs);
    CHECK_EQUAL(300, result.mtime);
}

TEST(getAttrs, entry_does_not_exist) {
    TSharesDB shares;
    mock().setData("data", 0);

    mock().expectOneCall("Dbc::get");
    mock().expectOneCall("Dbc::close");

    FileEntry result;
    int ret = shares.getAttrs("/entry", &result);

    CHECK_EQUAL(1, ret);
}

TEST_GROUP(addFile) {
    struct stat st;

    void setup() {
        this->st.st_size = 20;
        this->st.st_mtime = 600;
    }
};

TEST(addFile, with_properties) {
    TSharesDB shares;
    mock().setData("TagLib::isNull", false);

    mock().expectOneCall("FileRef::FileRef").withParameter("1", "/dir/file.ext");
    mock().expectOneCall("Db::put").withParameter("2", "/dir/file.exte").withParameter("3", "ext");
    mock().expectOneCall("Db::put").withParameter("2", "/dir/file.extl").withParameter("3", 3);
    std::vector<int> attrs({1, 10, 0});
    mock().expectOneCall("Db::put").withParameter("2", "/dir/file.exta").withParameterOfType("attrs", "3", attrs.data());
    mock().expectOneCall("Db::put").withParameter("2", "/dir/file.exts").withParameter("3", 20);
    mock().expectOneCall("Db::put").withParameter("2", "/dir/file.extm").withParameter("3", 600);
    mock().expectOneCall("Db::put").withParameter("2", "/dir").withParameter("3", "file.ext");
    mock().ignoreOtherCalls();

    shares.addFile("/dir", "file.ext", "/dir/file.ext", this->st);
}

TEST(addFile, without_properties) {
    TSharesDB shares;
    mock().setData("TagLib::isNull", true);

    mock().expectOneCall("FileRef::FileRef").withParameter("1", "/dir/file.ext");
    mock().expectOneCall("Db::put").withParameter("2", "/dir/file.exts").withParameter("3", 20);
    mock().expectOneCall("Db::put").withParameter("2", "/dir/file.extm").withParameter("3", 600);
    mock().expectOneCall("Db::put").withParameter("2", "/dir").withParameter("3", "file.ext");
    mock().ignoreOtherCalls();

    shares.addFile("/dir", "file.ext", "/dir/file.ext", this->st);
}

TEST_GROUP(removeFile) { };

TEST(removeFile, success) {
    TSharesDB shares;
    mock().setData("Dbc::get::withParameter", 1);

    mock().expectOneCall("Dbt::Dbt(2)").withParameter("1", "/dir/file").withParameter("2", 10);
    mock().expectOneCall("Dbt::Dbt(2)").withParameter("1", "/dir").withParameter("2", 5);
    mock().expectOneCall("Dbt::Dbt(2)").withParameter("1", "file").withParameter("2", 5);

    mock().expectOneCall("Db::del").withParameter("2", "/dir/file");
    mock().expectOneCall("Db::cursor");
    mock().expectOneCall("Dbc::get").withParameter("1", "/dir").withParameter("2", "file");
    mock().expectOneCall("Dbc::del");

    mock().expectOneCall("Dbc::close");

    shares.removeFile("/dir", "file", "/dir/file");
}

TEST_GROUP(addDir) { };

TEST(addDir, success) {
    TSharesDB shares;

    mock().expectOneCall("Dbt::Dbt(2)").withParameter("1", "/fdir").withParameter("2", 6);
    mock().expectOneCall("Dbt::Dbt(2)").withParameter("1", "sdir").withParameter("2", 5);

    mock().expectOneCall("Db::put").withParameter("2", "/fdir").withParameter("3", "sdir");

    shares.addDir("/fdir", "sdir");
}

TEST_GROUP(removeDir) { };

TEST(removeDir, success) {
    // We do not test removing files from attrdb, as it calls removeFile.
    TSharesDB shares;

    mock().expectOneCall("Dbt::Dbt(0)");
    mock().expectOneCall("Dbt::Dbt(2)").withParameter("1", "/fdir/sdir").withParameter("2", 11);
    mock().expectOneCall("Db::cursor");
    mock().expectOneCall("Dbc::get");

    mock().expectOneCall("Dbc::close");
    mock().expectOneCall("Db::del").withParameter("2", "/fdir/sdir");

    shares.removeDir("/fdir/sdir");
}

TEST_GROUP(pack) { };

TEST_GROUP(compress) { };

TEST_GROUP(contents) { };

TEST_GROUP(query) { };

TEST_GROUP(toProperCase) { };

TEST_GROUP(isShared) { };

TEST(isShared, shared) {
    TSharesDB shares;

    mock().expectOneCall("Db::exists").withParameter("2", "/dir/file.ext").andReturnValue(1);
    mock().ignoreOtherCalls();

    bool result = shares.isShared("/dir/file.ext");

    CHECK_EQUAL(true, result);
}

TEST(isShared, not_shared) {
    TSharesDB shares;

    mock().expectOneCall("Db::exists").withParameter("2", "/dir/file.ext").andReturnValue(DB_NOTFOUND);
    mock().ignoreOtherCalls();

    bool result = shares.isShared("/dir/file.ext");

    CHECK_EQUAL(false, result);
}

TEST_GROUP(filesCount) { };

TEST(filesCount, test) {
    TSharesDB shares;

    mock().expectOneCall("Db::stat");

    unsigned int result = shares.filesCount();

    CHECK_EQUAL(10, result);
}

TEST_GROUP(dirsCount) { };

TEST(dirsCount, test) {
    TSharesDB shares;

    mock().expectOneCall("Db::stat");

    unsigned int result = shares.dirsCount();

    CHECK_EQUAL(10, result);
}
