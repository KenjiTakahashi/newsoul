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

#include <cstdlib>
#include <CppUTest/TestHarness.h>
#include "../../src/utils/path.h"

#ifdef _WIN32
char sep = '\\';
std::string expected = "f\\s";
#else
char sep = '/';
std::string expected = "f/s";
#endif

TEST_GROUP(join) { };
TEST(join, empty) {
    std::string result = path::join({});

    CHECK(result.empty());
}
TEST(join, ends_with_sep_and_does_not_end_with_sep) {
    std::string result = path::join({std::string("f") + sep, "s"});

    CHECK_EQUAL(expected, result);
}
TEST(join, does_not_end_with_sep_and_ends_with_sep) {
    std::string result = path::join({"f", sep + std::string("s")});

    CHECK_EQUAL(expected, result);
}
TEST(join, ends_with_sep_and_ends_with_sep) {
    std::string result = path::join({std::string("f") + sep, sep + std::string("s")});

    CHECK_EQUAL(expected, result);
}
TEST(join, does_not_end_with_sep_and_does_not_end_with_sep) {
    std::string result = path::join({"f", "s"});

    CHECK_EQUAL(expected, result);
}
TEST(join, starts_with_sep) {
    std::string result = path::join({sep + std::string("f")});

    CHECK_EQUAL("/f", result);
}

TEST_GROUP(path_split) { };
TEST(path_split, empty) {
    std::vector<std::string> result = path::split("");

    CHECK(result.empty());
}
TEST(path_split, root_dir) {
    std::vector<std::string> result = path::split("/");

    CHECK(result.empty());
}
TEST(path_split, single_dir_absolute) {
    std::vector<std::string> result = path::split(sep + std::string("f"));

    std::vector<std::string> expected({"f"});

    CHECK(expected == result);
}
TEST(path_split, single_dir_relative) {
    std::vector<std::string> result = path::split("f");

    std::vector<std::string> expected({"f"});

    CHECK(expected == result);
}
TEST(path_split, two_dirs) {
    std::vector<std::string> result = path::split(expected);

    std::vector<std::string> expected({"f", "s"});

    CHECK(expected == result);
}
TEST(path_split, dirname_basename) {
    std::string path = std::string("/dir/name") + sep + "basename";
    std::vector<std::string> result = path::split(path, 1);

    std::vector<std::string> expected({"/dir/name", "basename"});

    CHECK(expected == result);
}

// TODO(Kenji): Find a way to mock getenv etc.
// Then do more test cases.
TEST_GROUP(path_expanduser) { };
TEST(path_expanduser, tilde) {
    std::string path("~");
    std::string expected(getenv("HOME"));

    std::string result = path::expanduser(path);

    CHECK(expected == result);
}
TEST(path_expanduser, tilde_with_dir) {
    std::string path("~/dir/name");
    std::string expected = std::string(getenv("HOME")) + "/dir/name";

    std::string result = path::expanduser(path);

    CHECK(expected == result);
}
TEST(path_expanduser, tilde_with_user) {
}
TEST(path_expanduser, tidle_in_middle) {
    std::string path("dir/~/name");

    std::string result = path::expanduser(path);

    CHECK(path == result);
}
TEST(path_expanduser, no_tilde) {
    std::string path("dir/name");

    std::string result = path::expanduser(path);

    CHECK(path == result);
}

TEST_GROUP(path_expandvars) { };
TEST(path_expandvars, HOME) {
    std::string path("$HOME/dir/name");
    std::string expected = std::string(getenv("HOME")) + "/dir/name";

    std::string result = path::expandvars(path);

    CHECK(expected == result);
}
TEST(path_expandvars, HOME_with_brackets) {
    std::string path("${HOME}/dir/name");
    std::string expected = std::string(getenv("HOME")) + "/dir/name";

    std::string result = path::expandvars(path);

    CHECK(expected == result);
}
TEST(path_expandvars, HOME_in_the_middle) {
    std::string path("/dir/$HOME/name");
    std::string expected = "/dir/" + std::string(getenv("HOME")) + "/name";

    std::string result = path::expandvars(path);

    CHECK(expected == result);
}
TEST(path_expandvars, HOME_at_the_end) {
    std::string path("/dir/name/$HOME");
    std::string expected = "/dir/name/" + std::string(getenv("HOME"));

    std::string result = path::expandvars(path);

    CHECK(expected == result);
}
TEST(path_expandvars, multiple_vars) {
}

TEST_GROUP(path_normpath) { };
TEST(path_normpath, dot) {
    std::string path("/dir/./name");
    std::string expected("/dir/name");

    std::string result = path::normpath(path);

    CHECK(expected == result);
}
TEST(path_normpath, double_dot) {
    std::string path("/dir/fake/../name");
    std::string expected("/dir/name");

    std::string result = path::normpath(path);

    CHECK(expected == result);
}
TEST(path_normpath, double_slash) {
    std::string path("/dir//name");
    std::string expected("/dir/name");

    std::string result = path::normpath(path);

    CHECK(expected == result);
}

TEST_GROUP(path_expand) { };
TEST(path_expand, with_spaces) {
    std::string path("/dir/name with spaces");

    std::string result = path::expand(path);

    CHECK(path == result);
}

TEST_GROUP(isAbsolute) { };
TEST(isAbsolute, absolute) {
    std::string s;
#ifdef _WIN32
    s += "C:";
#endif
    s += sep + std::string("absolute") + sep + "path";

    bool result = path::isAbsolute(s);

    CHECK_EQUAL(true, result);
}
TEST(isAbsolute, relative) {
    std::string s = std::string("relative") + sep + "path";

    bool result = path::isAbsolute(s);

    CHECK_EQUAL(false, result);
}
