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

TEST_GROUP(getAttrs) { };

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

    mock().expectOneCall("Dbt::Dbt(0)");
    mock().expectNCalls(5, "Dbt::Dbt(2)");
    mock().expectOneCall("Db::cursor");
    mock().expectNCalls(5, "Dbc::get");

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

    mock().expectOneCall("Dbt::Dbt(0)");
    mock().expectNCalls(5, "Dbt::Dbt(2)");
    mock().expectOneCall("Db::cursor");
    mock().expectOneCall("Dbc::get");

    FileEntry result;
    int ret = shares.getAttrs("/entry", &result);

    CHECK_EQUAL(1, ret);
}

int main(int argc, char *argv[]) {
    MockSupportPlugin mockPlugin;
    TestRegistry::getCurrentRegistry()->installPlugin(&mockPlugin);
    return CommandLineTestRunner::RunAllTests(argc, argv);
}

TEST_GROUP(addFile) { };

TEST_GROUP(removeFile) { };

TEST_GROUP(addDir) { };

TEST_GROUP(removeDir) { };

TEST_GROUP(pack) { };

TEST_GROUP(compress) { };

TEST_GROUP(contents) { };

TEST_GROUP(query) { };

TEST_GROUP(toProperCase) { };

TEST_GROUP(isShared) { };

TEST_GROUP(filesCount) { };

TEST_GROUP(dirsCount) { };
