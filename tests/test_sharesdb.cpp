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
#include <initializer_list>
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

    newsoul::File result;
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

    newsoul::File result;
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
    mock().expectOneCall("Db::put").withParameter("2", "/dir/file.exte").withParameter("3", "");
    mock().expectOneCall("Db::put").withParameter("2", "/dir/file.extl").withParameter("3", 0);
    std::vector<int> attrs;
    mock().expectOneCall("Db::put").withParameter("2", "/dir/file.exta").withParameterOfType("attrs", "3", attrs.data());
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

    mock().expectOneCall("Dbt::Dbt(2)").withParameter("1", "/dir/filee").withParameter("2", 11);
    mock().expectOneCall("Db::del").withParameter("2", "/dir/filee");
    mock().expectOneCall("Dbt::Dbt(2)").withParameter("1", "/dir/filel").withParameter("2", 11);
    mock().expectOneCall("Db::del").withParameter("2", "/dir/filel");
    mock().expectOneCall("Dbt::Dbt(2)").withParameter("1", "/dir/filea").withParameter("2", 11);
    mock().expectOneCall("Db::del").withParameter("2", "/dir/filea");
    mock().expectOneCall("Dbt::Dbt(2)").withParameter("1", "/dir/files").withParameter("2", 11);
    mock().expectOneCall("Db::del").withParameter("2", "/dir/files");
    mock().expectOneCall("Dbt::Dbt(2)").withParameter("1", "/dir/filem").withParameter("2", 11);
    mock().expectOneCall("Db::del").withParameter("2", "/dir/filem");

    mock().expectOneCall("Dbt::Dbt(2)").withParameter("1", "/dir").withParameter("2", 5);
    mock().expectOneCall("Dbt::Dbt(2)").withParameter("1", "file").withParameter("2", 5);
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
    mock().expectOneCall("Dbt::Dbt(2)").withParameter("1", "/fdir/sdir").withParameter("2", 11);
    mock().expectOneCall("Dbt::Dbt(2)").withParameter("1", "").withParameter("2", 1);

    mock().expectOneCall("Db::put").withParameter("2", "/fdir").withParameter("3", "sdir");
    mock().expectOneCall("Db::put").withParameter("2", "/fdir/sdir").withParameter("3", "");

    shares.addDir("/fdir", "sdir", "/fdir/sdir");
}

TEST_GROUP(compress) { };

TEST_GROUP(contents) {
    unsigned int i;
    std::vector<std::string> data;

    void setup() {
        mock().setData("Dbc::withContents", 1);
        this->i = 0;
        mock().setDataObject("Dbc::contents::i", "unsigned int", &this->i);

        mock().ignoreOtherCalls();
    }

    void setContents(std::initializer_list<std::string> contents) {
        this->data = std::vector<std::string>(contents);
        mock().setDataObject("Dbc::contents", "std::vector", &this->data);
    }
};
TEST(contents, non_existing) {
    TSharesDB shares;
    mock().setData("Dbc::withContents", 0);

    newsoul::Dirs result = shares.contents("/non/existing");

    newsoul::Dirs expected;
    CHECK(expected == result);
}
TEST(contents, without_subdirs) {
    TSharesDB shares;
    this->setContents({"", "null"});

    newsoul::Dirs result = shares.contents("/dir");

    newsoul::Dirs expected({{"/dir", {}}});
    CHECK(expected == result);
}
TEST(contents, with_one_subdir) {
    TSharesDB shares;
    this->setContents({"/dir/subdir", "null", ""});

    newsoul::Dirs result = shares.contents("/dir");

    newsoul::Dirs expected({{"/dir", {}}, {"/dir/subdir", {}}});
    CHECK(expected == result);
}
TEST(contents, with_multiple_subdirs) {
    TSharesDB shares;
    this->setContents({"/dir/subdir1", "null", "/dir/subdir2", "null", ""});

    newsoul::Dirs result = shares.contents("/dir");

    newsoul::Dirs expected({{"/dir", {}}, {"/dir/subdir1", {}}, {"/dir/subdir2", {}}});
    CHECK(expected == result);
}

TEST_GROUP(query) { };

TEST_GROUP(toProperCase) { };
TEST(toProperCase, entry_exists) {
    TSharesDB shares;
    mock().setData("data", 1);
    mock().setData("keydata", "/Dir/File.ext");

    mock().ignoreOtherCalls();

    std::string result = shares.toProperCase("/dir/file.ext");

    CHECK_EQUAL("/Dir/File.ext", result);
}
TEST(toProperCase, entry_does_not_exist) {
    TSharesDB shares;

    mock().ignoreOtherCalls();

    std::string result = shares.toProperCase("/dir/file.ext");

    CHECK_EQUAL(std::string(), result);
}

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
TEST(filesCount, success) {
    TSharesDB shares;

    mock().expectOneCall("Db::stat");

    unsigned int result = shares.filesCount();

    CHECK_EQUAL(2, result);
}

TEST_GROUP(dirsCount) { };
TEST(dirsCount, success) {
    TSharesDB shares;

    mock().expectOneCall("Db::stat");

    unsigned int result = shares.dirsCount();

    CHECK_EQUAL(10, result);
}
