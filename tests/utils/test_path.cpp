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

#include "../../src/utils/path.h"

TEST_CASE("join", "[utils][path]") {
#ifdef _WIN32
    char sep = '\\';
    std::string expected = "f\\s";
#else
    char sep = '/';
    std::string expected = "f/s";
#endif

    SECTION("empty") {
        std::string result = path::join({});

        REQUIRE(result.empty());
    }

    SECTION("ends with sep + doesn't end with sep") {
        std::string result = path::join({std::string("f") + sep, "s"});

        REQUIRE(result == expected);
    }

    SECTION("doesn't end with sep + ends with sep") {
        std::string result = path::join({"f", sep + std::string("s")});

        REQUIRE(result == expected);
    }

    SECTION("ends with sep + ends with sep") {
        std::string result = path::join({std::string("f") + sep, sep + std::string("s")});

        REQUIRE(result == expected);
    }

    SECTION("doesn't end with sep + doesn't end with sep") {
        std::string result = path::join({"f", "s"});

        REQUIRE(result == expected);
    }

    SECTION("starts with sep", "[corner]") {
        std::string result = path::join({sep + std::string("f")});

        REQUIRE(result == "/f");
    }
}
