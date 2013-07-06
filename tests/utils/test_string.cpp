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

#include "../catch.hpp"
#include "../../src/utils/string.h"

TEST_CASE("tolower", "[utils][string]") {
    SECTION("all lower") {
        std::string result = string::tolower("tolower");

        REQUIRE(result == "tolower");
    }

    SECTION("all upper") {
        std::string result = string::tolower("TOLOWER");

        REQUIRE(result == "tolower");
    }

    SECTION("first upper") {
        std::string result = string::tolower("Tolower");

        REQUIRE(result == "tolower");
    }

    SECTION("some upper") {
        std::string result = string::tolower("ToLoWeR");

        REQUIRE(result == "tolower");
    }
}

TEST_CASE("split", "[utils][string]") {
    //I know vectors can be compared, but REQUIRE is complaining.
    SECTION("split,string on ,") {
        std::vector<std::string> result = string::split("split,string", ",");

        REQUIRE(result.size() == 2);
        REQUIRE(result[0] == "split");
        REQUIRE(result[1] == "string");
    }

    SECTION("split,.string on ,.") {
        std::vector<std::string> result = string::split("split,.string", ",.");

        REQUIRE(result.size() == 2);
        REQUIRE(result[0] == "split");
        REQUIRE(result[1] == "string");
    }

    SECTION("split,long.string on ,.") {
        std::vector<std::string> result = string::split("split,long.string", ",.");

        REQUIRE(result.size() == 3);
        REQUIRE(result[0] == "split");
        REQUIRE(result[1] == "long");
        REQUIRE(result[2] == "string");
    }

    SECTION("split,,string on ,", "[corner]") {
        std::vector<std::string> result = string::split("split,,string", ",");

        REQUIRE(result.size() == 2);
        REQUIRE(result[0] == "split");
        REQUIRE(result[1] == "string");
    }

    SECTION(",split,string on ,", "[corner]") {
        std::vector<std::string> result = string::split(",split,string", ",");

        REQUIRE(result.size() == 2);
        REQUIRE(result[0] == "split");
        REQUIRE(result[1] == "string");
    }

    SECTION("split,string, on ,", "[corner]") {
        std::vector<std::string> result = string::split("split,string,", ",");

        REQUIRE(result.size() == 2);
        REQUIRE(result[0] == "split");
        REQUIRE(result[1] == "string");
    }
}

TEST_CASE("replace", "[utils][string]") {
    SECTION("replace e with y") {
        std::string result = string::replace("replace", 'e', 'y');

        REQUIRE(result == "ryplacy");
    }

    SECTION("replace rep with bar") {
        std::string result = string::replace("replace", "rep", "bar");

        REQUIRE(result == "barlace");
    }

    SECTION("replace rap with bar", "[corner]") {
        std::string result = string::replace("replace", "rap", "bar");

        REQUIRE(result == "replace");
    }
}
