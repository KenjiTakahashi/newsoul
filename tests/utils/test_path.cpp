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
