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

#include <sstream>
#include "catch.hpp"
#include "../src/config.h"

TEST_CASE("getInt", "[newsoul][Config]") {
    std::istringstream data("{\"int1\":1,\"key\":{\"int2\":2}}");
    newsoul::Config *config = new newsoul::Config(data, false);

    SECTION("top level") {
        int result = config->getInt({"int1"});

        REQUIRE(result == 1);
    }

    SECTION("nested") {
        int result = config->getInt({"key", "int2"});

        REQUIRE(result == 2);
    }

    SECTION("not found") {
        int result = config->getInt({"notfound"});

        REQUIRE(result == 0);
    }

    delete config;
}

TEST_CASE("getStr", "[newsoul][Config]") {
    std::istringstream data("{\"str1\":\"s1\",\"key\":{\"str2\":\"s2\"}}");
    newsoul::Config *config = new newsoul::Config(data, false);

    SECTION("top level") {
        const std::string result = config->getStr({"str1"});

        REQUIRE(result == "s1");
    }

    SECTION("nested") {
        const std::string result = config->getStr({"key", "str2"});

        REQUIRE(result == "s2");
    }

    SECTION("not found") {
        const std::string result = config->getStr({"notfound"});

        REQUIRE(result.empty());
    }

    delete config;
}

TEST_CASE("set<int>", "[newsoul][Config]") {
    std::istringstream data("");
    newsoul::Config *config = new newsoul::Config(data, false);

    SECTION("top level") {
        config->set({"int1"}, 1);

        int result = config->getInt({"int1"});

        REQUIRE(result == 1);
    }

    SECTION("nested") {
        config->set({"key", "int2"}, 2);

        int result = config->getInt({"key", "int2"});

        REQUIRE(result == 2);
    }

    SECTION("nested in existing section") {
        config->set({"key", "int1"}, 1);
        config->set({"key", "int2"}, 2);

        int result1 = config->getInt({"key", "int1"});
        int result2 = config->getInt({"key", "int2"});

        REQUIRE(result1 == 1);
        REQUIRE(result2 == 2);
    }

    delete config;
}

TEST_CASE("set<string>", "[newsoul][Config]") {
    std::istringstream data("");
    newsoul::Config *config = new newsoul::Config(data, false);

    SECTION("top level") {
        config->set({"str1"}, "s1");

        const std::string result = config->getStr({"str1"});

        REQUIRE(result == "s1");
    }

    SECTION("nested") {
        config->set({"key", "str2"}, "s2");

        const std::string result = config->getStr({"key", "str2"});
        
        REQUIRE(result == "s2");
    }

    SECTION("nested in existing section") {
        config->set({"key", "str1"}, "s1");
        config->set({"key", "str2"}, "s2");

        const std::string result1 = config->getStr({"key", "str1"});
        const std::string result2 = config->getStr({"key", "str2"});

        REQUIRE(result1 == "s1");
        REQUIRE(result2 == "s2");
    }

    delete config;
}
