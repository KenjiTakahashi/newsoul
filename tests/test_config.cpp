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
    auto config = new newsoul::Config(data);

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
    auto config = new newsoul::Config(data);

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

TEST_CASE("getBool", "[newsoul][Config]") {
    std::istringstream data("{\"bool1\":true,\"key\":{\"bool2\":true}}");
    auto config = new newsoul::Config(data, false);

    SECTION("top level") {
        bool result = config->getBool({"bool1"});

        REQUIRE(result == true);
    }

    SECTION("nested") {
        bool result = config->getBool({"key", "bool2"});

        REQUIRE(result == true);
    }

    SECTION("not found") {
        bool result = config->getBool({"notfound"});

        REQUIRE(result == false);
    }

    delete config;
}

TEST_CASE("getVec", "[newsoul][Config]") {
    std::istringstream data("{\"vec1\":[\"v1\",\"v2\"],\"key\":{\"vec2\":[\"v3\",\"v4\"]}, \"notvec\":1}");
    auto config = new newsoul::Config(data, false);

    SECTION("top level") {
        std::vector<std::string> result = config->getVec({"vec1"});

        REQUIRE(result[0] == "v1");
        REQUIRE(result[1] == "v2");
    }

    SECTION("nested") {
        std::vector<std::string> result = config->getVec({"key", "vec2"});

        REQUIRE(result[0] == "v3");
        REQUIRE(result[1] == "v4");
    }

    SECTION("not found") {
        std::vector<std::string> result = config->getVec({"notfound"});

        REQUIRE(result.empty());
    }

    SECTION("not array", "[corner]") {
        std::vector<std::string> result = config->getVec({"notvec"});

        REQUIRE(result.empty());
    }

    delete config;
}

TEST_CASE("set<int>", "[newsoul][Config]") {
    std::istringstream data("{\"e\":{\"int1\":1}}");
    auto config = new newsoul::Config(data);

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
        config->set({"e", "int2"}, 2);

        int result1 = config->getInt({"e", "int1"});
        int result2 = config->getInt({"e", "int2"});

        REQUIRE(result1 == 1);
        REQUIRE(result2 == 2);
    }

    delete config;
}

TEST_CASE("set<string>", "[newsoul][Config]") {
    std::istringstream data("{\"e\":{\"str1\":\"s1\"}}");
    auto config = new newsoul::Config(data);

    SECTION("top level") {
        config->set({"str1"}, std::string("s1"));

        const std::string result = config->getStr({"str1"});

        REQUIRE(result == "s1");
    }

    SECTION("nested") {
        config->set({"key", "str2"}, std::string("s2"));

        const std::string result = config->getStr({"key", "str2"});

        REQUIRE(result == "s2");
    }

    SECTION("nested in existing section") {
        config->set({"e", "str2"}, std::string("s2"));

        const std::string result1 = config->getStr({"e", "str1"});
        const std::string result2 = config->getStr({"e", "str2"});

        REQUIRE(result1 == "s1");
        REQUIRE(result2 == "s2");
    }

    delete config;
}

TEST_CASE("set<bool>", "[newsoul][Config]") {
    std::istringstream data("{\"e\":{\"b1\":true}}");
    auto config = new newsoul::Config(data);

    SECTION("top level") {
        config->set({"b1"}, true);

        bool result = config->getBool({"b1"});

        REQUIRE(result == true);
    }

    SECTION("nested") {
        config->set({"key", "b2"}, true);

        bool result = config->getBool({"key", "b2"});

        REQUIRE(result == true);
    }

    SECTION("nested in existing section") {
        config->set({"e", "b2"}, true);

        bool result1 = config->getBool({"e", "b1"});
        bool result2 = config->getBool({"e", "b2"});

        REQUIRE(result1 == true);
        REQUIRE(result2 == true);
    }

    delete config;
}

TEST_CASE("contains", "[newsoul][Config]") {
    std::istringstream data("{\"vec1\":[\"v1\",\"v2\"],\"key\":{\"vec2\":[\"v3\",\"v4\"]}, \"notvec\":1}");
    auto config = new newsoul::Config(data);

    SECTION("top level") {
        bool result = config->contains({"vec1"}, "v1");

        REQUIRE(result == true);
    }

    SECTION("nested") {
        bool result = config->contains({"key", "vec2"}, "v3");

        REQUIRE(result == true);
    }

    SECTION("not found value") {
        bool result = config->contains({"vec1"}, "v3");

        REQUIRE(result == false);
    }

    SECTION("not found key") {
        bool result = config->contains({"notfound"}, "");

        REQUIRE(result == false);
    }

    SECTION("not array") {
        bool result = config->contains({"notvec"}, "v5");

        REQUIRE(result == false);
    }

    delete config;
}

TEST_CASE("add", "[newsoul][Config]") {
    std::istringstream data("{\"vec1\":[\"v1\",\"v2\"],\"key\":{\"vec2\":[\"v3\",\"v4\"]},\"notvec\":1}");
    auto config = new newsoul::Config(data);

    SECTION("top level") {
        config->add({"vec1"}, "v5");

        std::vector<std::string> result = config->getVec({"vec1"});

        REQUIRE(result[0] == "v1");
        REQUIRE(result[1] == "v2");
        REQUIRE(result[2] == "v5");
    }

    SECTION("nested") {
        config->add({"key", "vec2"}, "v6");

        std::vector<std::string> result = config->getVec({"key", "vec2"});

        REQUIRE(result[0] == "v3");
        REQUIRE(result[1] == "v4");
        REQUIRE(result[2] == "v6");
    }

    SECTION("not array") {
        config->add({"notvec"}, "v7");

        std::vector<std::string> result = config->getVec({"notvec"});

        REQUIRE(result[0] == "v7");
    }

    SECTION("non existing top level key") {
        config->add({"notfound"}, "v8");

        std::vector<std::string> result = config->getVec({"notfound"});

        REQUIRE(result[0] == "v8");
    }

    delete config;
}

TEST_CASE("del", "[newsoul][Config]") {
    std::istringstream data("{\"v1\":1,\"vec1\":[\"v9\"],\"key\":{\"v2\":\"2\",\"vec2\":[\"v0\", \"v11\"]}}");
    auto config = new newsoul::Config(data);

    SECTION("top level object") {
        bool res1 = config->del({}, "v1");
        int res2 = config->getInt({"v1"});

        REQUIRE(res1 == true);
        REQUIRE(res2 == 0);
    }

    SECTION("nested object") {
        bool res1 = config->del({"key"}, "v2");
        const std::string res2 = config->getStr({"key", "v2"});

        REQUIRE(res1 == true);
        REQUIRE(res2 == "");
    }

    SECTION("not found object") {
        bool res1 = config->del({"v1"}, "notfound");

        REQUIRE(res1 == false);
    }

    SECTION("top level array value") {
        bool res1 = config->del({"vec1"}, "v9");
        std::vector<std::string> res2 = config->getVec({"vec1"});

        REQUIRE(res1 == true);
        REQUIRE(res2.empty() == true);
    }

    SECTION("nested array value") {
        bool res1 = config->del({"key", "vec2"}, "v0");
        std::vector<std::string> res2 = config->getVec({"key", "vec2"});

        REQUIRE(res1 == true);
        REQUIRE(res2[0] == "v11");
    }

    SECTION("not found array value") {
        bool res1 = config->del({"vec1"}, "notfound");

        REQUIRE(res1 == false);
    }

    SECTION("not found key", "[corner]") {
        bool res1 = config->del({"notfound"}, "notfound");

        REQUIRE(res1 == false);
    }

    delete config;
}
