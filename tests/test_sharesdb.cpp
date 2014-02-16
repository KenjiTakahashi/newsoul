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

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestRegistry.h>
#include <CppUTestExt/MockSupport.h>
#include <CppUTestExt/MockSupportPlugin.h>
#include <initializer_list>
#include <sys/stat.h>
#include "mocks/sqlite.h"

TEST_GROUP(getAttrs) { };
TEST(getAttrs, entry_exists) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mockDB.insertAttrs("/file.ext0");

    newsoul::File result;
    int ret = shares.getAttrs("/file.ext0", &result);

    CHECK_EQUAL(0, ret);
    CHECK_EQUAL(20, result.size);
    CHECK_EQUAL("ext", result.ext);
    CHECK(std::vector<unsigned int>({192, 10, 0}) == result.attrs);
    CHECK_EQUAL(600, result.mtime);
}
TEST(getAttrs, entry_does_not_exist) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);

    newsoul::File result;
    int ret = shares.getAttrs("/file.ext0", &result);

    CHECK_EQUAL(1, ret);
}

TEST_GROUP(addFile) {
    struct stat st;

    void setup() {
        this->st.st_size = 20;
        this->st.st_mtime = 600;

        mock().expectOneCall("FileRef::FileRef").withParameter("1", "/dir0/file.ext");
        mock().ignoreOtherCalls();
    }
};
TEST(addFile, with_properties) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mock().setData("TagLib::isNull", false);

    shares.addFile("/dir0", "/dir0/file.ext", this->st);

    CHECK(mockDB.assertDirs({"/dir0"}));
    CHECK(mockDB.assertFiles({"/dir0/file.ext"}, true));
}
TEST(addFile, without_properties) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mock().setData("TagLib::isNull", true);

    shares.addFile("/dir0", "/dir0/file.ext", this->st);

    CHECK(mockDB.assertDirs({"/dir0"}));
    CHECK(mockDB.assertFiles({"/dir0/file.ext"}, false));
}
TEST(addFile, into_existing_dir) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mockDB.insertDirs(1);
    mock().setData("TagLib::isNull", true);

    shares.addFile("/dir0", "/dir0/file.ext", this->st);

    CHECK(mockDB.assertFiles({"/dir0/file.ext"}, false));
}
TEST(addFile, update) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mockDB.insertAttrsDifferent("/dir0/file.ext");
    mock().setData("TagLib::isNull", false);

    shares.addFile("/dir0", "/dir0/file.ext", this->st);

    CHECK(mockDB.assertFiles({"/dir0/file.ext"}, true));
}

TEST_GROUP(addDir) { };
TEST(addDir, new_root) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);

    shares.addDir("/fdir/sdir");

    CHECK(mockDB.assertDirs({"/fdir", "/fdir/sdir"}));
}
TEST(addDir, existing_root) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mockDB.insertDirs(1);

    shares.addDir("/dir0/sdir");

    CHECK(mockDB.assertDirs({"/dir0", "/dir0/sdir"}));
}
TEST(addDir, second_root) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mockDB.insertDirs(1);

    shares.addDir("/fdir/sdir");

    CHECK(mockDB.assertDirs({"/dir0", "/fdir", "/fdir/sdir"}));
}

TEST_GROUP(remove) { };
TEST(remove, directory) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mockDB.insertDirs(2);

    shares.remove({"/dir0"});

    CHECK(!mockDB.assertDirs({"/dir0"}));
    CHECK(mockDB.assertDirs({"/dir1"}));
}
TEST(remove, file) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mockDB.insertAttrs("/dir/file.ext0");
    mockDB.insertAttrs("/dir/file.ext1");

    shares.remove({"/dir/file.ext0"});

    CHECK(!mockDB.assertFiles({"/dir/file.ext0"}, true));
    CHECK(mockDB.assertFiles({"/dir/file.ext1"}, true));
}
TEST(remove, file_with_empty_dir) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mockDB.insertDirs(1);
    mockDB.insertAttrs("/dir0/file.ext0", 1);

    shares.remove({"/dir0/file.ext0"});

    CHECK(!mockDB.assertFiles({"/dir0/file.ext0"}, true));
    CHECK(mockDB.assertDirs({"/dir0"}));
}
TEST(remove, file_without_non_empty_dir) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mockDB.insertDirs(1);
    mockDB.insertAttrs("/dir0/file.ext0", 1);
    mockDB.insertAttrs("/dir0/file.ext1", 1);

    shares.remove({"/dir0/file.ext0"});

    CHECK(!mockDB.assertFiles({"/dir0/file.ext0"}, true));
    CHECK(mockDB.assertFiles({"/dir0/file.ext1"}, true));
    CHECK(mockDB.assertDirs({"/dir0"}));
}
TEST(remove, directory_with_file) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mockDB.insertDirs(2);
    mockDB.insertAttrs("/dir0/file.ext0", 1);
    mockDB.insertAttrs("/dir1/file.ext1", 2);

    shares.remove({"/dir0"});

    CHECK(!mockDB.assertDirs({"/dir0"}));
    CHECK(mockDB.assertDirs({"/dir1"}));
    CHECK(!mockDB.assertFiles({"/dir0/file.ext0"}, true));
    CHECK(mockDB.assertFiles({"/dir1/file.ext1"}, true));
}

TEST_GROUP(compress) { };

TEST_GROUP(contents) { };
TEST(contents, non_existing) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);

    newsoul::Dirs result = shares.contents("/non/existing");

    newsoul::Dirs expected;
    CHECK(expected == result);
}
TEST(contents, without_subdirs) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mockDB.insertDirs(1);

    newsoul::Dirs result = shares.contents("/dir0");

    newsoul::Dirs expected({{"/dir0", {}}});
    CHECK(expected == result);
}
TEST(contents, with_one_subdir) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mockDB.insertDirs(1);
    mockDB.insertSubdirs(1, 1);

    newsoul::Dirs result = shares.contents("/dir0");

    newsoul::Dirs expected({{"/dir0", {}}, {"/dir0/subdir0", {}}});
    CHECK(expected == result);
}
TEST(contents, with_multiple_subdirs) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mockDB.insertDirs(1);
    mockDB.insertSubdirs(2, 1);

    newsoul::Dirs result = shares.contents("/dir0");

    newsoul::Dirs expected({
        {"/dir0", {}},
        {"/dir0/subdir0", {}},
        {"/dir0/subdir1", {}}
    });
    CHECK(expected == result);
}
TEST(contents, with_file_in_main_dir) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mockDB.insertDirs(1);
    mockDB.insertAttrs("/dir0/file.ext0", 1);

    newsoul::Dirs result = shares.contents("/dir0");

    newsoul::File fe = {.size=20, .ext="ext", .attrs={192, 10, 0}, .mtime=600};
    newsoul::Dirs expected({{"/dir0", {{"file.ext0", fe}}}});
    CHECK(expected == result);
}
TEST(contents, with_file_in_subdir) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mockDB.insertDirs(1);
    mockDB.insertSubdirs(1, 1);
    mockDB.insertAttrs("/dir0/subdir0/file.ext0", 2);

    newsoul::Dirs result = shares.contents("/dir0");

    newsoul::File fe = {.size=20, .ext="ext", .attrs={192, 10, 0}, .mtime=600};
    newsoul::Dirs expected({
        {"/dir0", {}},
        {"/dir0/subdir0", {{"file.ext0", fe}}}
    });
    CHECK(expected == result);
}

TEST_GROUP(query) { };
TEST(query, one_word) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mockDB.insertDirs(2);
    mockDB.insertAttrs("/dir0/file0", 1);
    mockDB.insertAttrs("/dir0/file1", 1);
    mockDB.insertAttrs("/dir1/file0", 2);

    newsoul::Dir result = shares.query("file0");

    newsoul::File fe = {.size=20, .ext="ext", .attrs={192, 10, 0}, .mtime=600};
    newsoul::Dir expected({
        {"/dir0/file0", fe},
        {"/dir1/file0", fe}
    });
    CHECK(expected == result);
}
TEST(query, two_words) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mockDB.insertDirs(2);
    mockDB.insertAttrs("/dir0/file0", 1);
    mockDB.insertAttrs("/dir0/file1", 1);
    mockDB.insertAttrs("/dir1/file0", 2);

    newsoul::Dir result = shares.query("dir0 file");

    newsoul::File fe = {.size=20, .ext="ext", .attrs={192, 10, 0}, .mtime=600};
    newsoul::Dir expected({
        {"/dir0/file0", fe},
        {"/dir0/file1", fe}
    });
    CHECK(expected == result);
}
TEST(query, negation) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mockDB.insertDirs(2);
    mockDB.insertAttrs("/dir0/file0", 1);
    mockDB.insertAttrs("/dir1/file0", 2);
    mockDB.insertAttrs("/dir1/file1", 2);

    newsoul::Dir result = shares.query("file -dir0");

    newsoul::File fe = {.size=20, .ext="ext", .attrs={192, 10, 0}, .mtime=600};
    newsoul::Dir expected({
        {"/dir1/file0", fe},
        {"/dir1/file1", fe}
    });
    CHECK(expected == result);
}
TEST(query, wildcard) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
}
TEST(query, phrase) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mockDB.insertDirs(2);
    mockDB.insertAttrs("/dir0/space file0", 1);
    mockDB.insertAttrs("/dir0/space file1", 1);
    mockDB.insertAttrs("/dir0/spacefile0", 1);
    mockDB.insertAttrs("/dir1/space file2", 2);
    mockDB.insertAttrs("/dir1/spacefile2", 2);

    newsoul::Dir result = shares.query("\"space file\"");

    newsoul::File fe = {.size=20, .ext="ext", .attrs={192, 10, 0}, .mtime=600};
    newsoul::Dir expected({
        {"/dir0/space file0", fe},
        {"/dir0/space file1", fe},
        {"/dir1/space file2", fe}
    });
    //CHECK(expected == result);
}

TEST_GROUP(toProperCase) { };
TEST(toProperCase, entry_exists_upper_case) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mockDB.insertFilesUpperCase(1);

    std::string result = shares.toProperCase("/dir/file.ext0");

    CHECK_EQUAL("/Dir/File.ext0", result);
}
TEST(toProperCase, entry_exists_lower_case) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mockDB.insertFiles(1);

    std::string result = shares.toProperCase("/dir/file.ext0");

    CHECK_EQUAL("/dir/file.ext0", result);
}
TEST(toProperCase, entry_does_not_exist) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);

    std::string result = shares.toProperCase("/dir/file.ext0");

    CHECK_EQUAL(std::string(), result);
}

TEST_GROUP(isShared) { };
TEST(isShared, shared) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mockDB.insertFiles(1);

    bool result = shares.isShared("/dir/file.ext0");

    CHECK_EQUAL(true, result);
}
TEST(isShared, not_shared) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);

    bool result = shares.isShared("/dir/file.ext0");

    CHECK_EQUAL(false, result);
}

TEST_GROUP(filesCount) { };
TEST(filesCount, zero) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);

    unsigned int result = shares.filesCount();

    CHECK_EQUAL(0, result);
}
TEST(filesCount, one) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mockDB.insertFiles(1);
    mockDB.insertDirs(3);

    unsigned int result = shares.filesCount();

    CHECK_EQUAL(1, result);
}

TEST_GROUP(dirsCount) { };
TEST(dirsCount, zero) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);

    unsigned int result = shares.dirsCount();

    CHECK_EQUAL(0, result);
}
TEST(dirsCount, one) {
    TSharesDB shares;

    mocks::SqliteMock mockDB(&shares);
    mockDB.insertDirs(1);
    mockDB.insertFiles(3);

    unsigned int result = shares.dirsCount();

    CHECK_EQUAL(1, result);
}
