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
#include <CppUTest/TestHarness.h>
#include "../src/config.h"

TEST_GROUP(getInt) {
    newsoul::Config *config;

    void setup() {
        std::istringstream data("{\"int1\":1,\"key\":{\"int2\":2}}");
        config = new newsoul::Config(data);
    }

    void teardown() {
        delete config;
    }
};

TEST(getInt, top_level) {
    int result = config->getInt({"int1"});

    CHECK_EQUAL(1, result);
}

TEST(getInt, nested) {
    int result = config->getInt({"key", "int2"});

    CHECK_EQUAL(2, result);
}

TEST(getInt, not_found) {
    int result = config->getInt({"notfound"});

    CHECK_EQUAL(0, result);
}

TEST_GROUP(getStr) {
    newsoul::Config *config;

    void setup() {
        std::istringstream data("{\"str1\":\"s1\",\"key\":{\"str2\":\"s2\"}}");
        config = new newsoul::Config(data);
    }

    void teardown() {
        delete config;
    }
};

TEST(getStr, top_level) {
    const std::string result = config->getStr({"str1"});

    CHECK_EQUAL("s1", result);
}

TEST(getStr, nested) {
    const std::string result = config->getStr({"key", "str2"});

    CHECK_EQUAL("s2", result);
}

TEST(getStr, not_found) {
    const std::string result = config->getStr({"notfound"});

    CHECK(result.empty());
}

TEST_GROUP(getBool) {
    newsoul::Config *config;

    void setup() {
        std::istringstream data("{\"bool1\":true,\"key\":{\"bool2\":true}}");
        config = new newsoul::Config(data);
    }

    void teardown() {
        delete config;
    }
};

TEST(getBool, top_level) {
    bool result = config->getBool({"bool1"});

    CHECK_EQUAL(true, result);
}

TEST(getBool, nested) {
    bool result = config->getBool({"key", "bool2"});

    CHECK_EQUAL(true, result);
}

TEST(getBool, not_found) {
    bool result = config->getBool({"notfound"});

    CHECK_EQUAL(false, result);
}

TEST_GROUP(getVec) {
    newsoul::Config *config;

    void setup() {
        std::istringstream data("{\"vec1\":[\"v1\",\"v2\"],\"key\":{\"vec2\":[\"v3\",\"v4\"]}, \"notvec\":1}");
        config = new newsoul::Config(data);
    }

    void teardown() {
        delete config;
    }
};

TEST(getVec, top_level) {
    std::vector<std::string> result = config->getVec({"vec1"});

    CHECK(std::vector<std::string>({"v1", "v2"}) == result);
}

TEST(getVec, nested) {
    std::vector<std::string> result = config->getVec({"key", "vec2"});

    CHECK(std::vector<std::string>({"v3", "v4"}) == result);
}

TEST(getVec, not_found) {
    std::vector<std::string> result = config->getVec({"notfound"});

    CHECK(result.empty());
}

TEST(getVec, not_array) {
    std::vector<std::string> result = config->getVec({"notvec"});

    CHECK(result.empty());
}

TEST_GROUP(setInt) {
    newsoul::Config *config;

    void setup() {
        std::istringstream data("{\"e\":{\"int1\":1}}");
        config = new newsoul::Config(data);
    }

    void teardown() {
        delete config;
    }
};

TEST(setInt, top_level) {
    config->set({"int1"}, 1);

    int result = config->getInt({"int1"});

    CHECK_EQUAL(1, result);
}

TEST(setInt, nested) {
    config->set({"key", "int2"}, 2);

    int result = config->getInt({"key", "int2"});

    CHECK_EQUAL(2, result);
}

TEST(setInt, nested_in_existing_section) {
    config->set({"e", "int2"}, 2);

    int result1 = config->getInt({"e", "int1"});
    int result2 = config->getInt({"e", "int2"});

    CHECK_EQUAL(1, result1);
    CHECK_EQUAL(2, result2);
}

TEST_GROUP(setString) {
    newsoul::Config *config;

    void setup() {
        std::istringstream data("{\"e\":{\"str1\":\"s1\"}}");
        config = new newsoul::Config(data);
    }

    void teardown() {
        delete config;
    }
};

TEST(setString, top_level) {
    config->set({"str1"}, std::string("s1"));

    const std::string result = config->getStr({"str1"});

    CHECK_EQUAL("s1", result);
}

TEST(setString, nested) {
    config->set({"key", "str2"}, std::string("s2"));

    const std::string result = config->getStr({"key", "str2"});

    CHECK_EQUAL("s2", result);
}

TEST(setString, nested_in_existing_section) {
    config->set({"e", "str2"}, std::string("s2"));

    const std::string result1 = config->getStr({"e", "str1"});
    const std::string result2 = config->getStr({"e", "str2"});

    CHECK_EQUAL("s1", result1);
    CHECK_EQUAL("s2", result2);
}

TEST_GROUP(setBool) {
    newsoul::Config *config;

    void setup() {
        std::istringstream data("{\"e\":{\"b1\":true}}");
        config = new newsoul::Config(data);
    }

    void teardown() {
        delete config;
    }
};

TEST(setBool, top_level) {
    config->set({"b1"}, true);

    bool result = config->getBool({"b1"});

    CHECK_EQUAL(true, result);
}

TEST(setBool, nested) {
    config->set({"key", "b2"}, true);

    bool result = config->getBool({"key", "b2"});

    CHECK_EQUAL(true, result);
}

TEST(setBool, nested_in_existing_section) {
    config->set({"e", "b2"}, true);

    bool result1 = config->getBool({"e", "b1"});
    bool result2 = config->getBool({"e", "b2"});

    CHECK_EQUAL(true, result1);
    CHECK_EQUAL(true, result2);
}

TEST_GROUP(contains) {
    newsoul::Config *config;

    void setup() {
        std::istringstream data("{\"vec1\":[\"v1\",\"v2\"],\"key\":{\"vec2\":[\"v3\",\"v4\"]},\"notvec\":1}");
        config = new newsoul::Config(data);
    }

    void teardown() {
        delete config;
    }
};

TEST(contains, top_level) {
    bool result = config->contains({"vec1"}, "v1");

    CHECK_EQUAL(true, result);
}

TEST(contains, nested) {
    bool result = config->contains({"key", "vec2"}, "v3");

    CHECK_EQUAL(true, result);
}

TEST(contains, not_found_value) {
    bool result = config->contains({"vec1"}, "v3");

    CHECK_EQUAL(false, result);
}

TEST(contains, not_found_key) {
    bool result = config->contains({"notfound"}, "");

    CHECK_EQUAL(false, result);
}

TEST(contains, not_array) {
    bool result = config->contains({"notvec"}, "v5");

    CHECK_EQUAL(false, result);
}

TEST_GROUP(add) {
    newsoul::Config *config;

    void setup() {
        std::istringstream data("{\"vec1\":[\"v1\",\"v2\"],\"key\":{\"vec2\":[\"v3\",\"v4\"]},\"notvec\":1}");
        config = new newsoul::Config(data);
    }

    void teardown() {
        delete config;
    }
};

TEST(add, top_level) {
    config->add({"vec1"}, "v5");

    std::vector<std::string> result = config->getVec({"vec1"});

    CHECK(std::vector<std::string>({"v1", "v2", "v5"}) == result);
}

TEST(add, nested) {
    config->add({"key", "vec2"}, "v6");

    std::vector<std::string> result = config->getVec({"key", "vec2"});

    CHECK(std::vector<std::string>({"v3", "v4", "v6"}) == result);
}

TEST(add, not_array) {
    config->add({"notvec"}, "v7");

    std::vector<std::string> result = config->getVec({"notvec"});

    CHECK(std::vector<std::string>({"v7"}) == result);
}

TEST(add, non_existing_top_level_key) {
    config->add({"notfound"}, "v8");

    std::vector<std::string> result = config->getVec({"notfound"});

    CHECK(std::vector<std::string>({"v8"}) == result);
}

TEST_GROUP(del) {
    newsoul::Config *config;

    void setup() {
        std::istringstream data("{\"v1\":1,\"vec1\":[\"v9\"],\"key\":{\"v2\":\"2\",\"vec2\":[\"v0\",\"v11\"]}}");
        config = new newsoul::Config(data);
    }

    void teardown() {
        delete config;
    }
};

TEST(del, top_level_object) {
    bool res1 = config->del({}, "v1");
    int res2 = config->getInt({"v1"});

    CHECK_EQUAL(true, res1);
    CHECK_EQUAL(0, res2);
}

TEST(del, nested_object) {
    bool res1 = config->del({"key"}, "v2");
    const std::string res2 = config->getStr({"key", "v2"});

    CHECK_EQUAL(true, res1);
    CHECK_EQUAL("", res2);
}

TEST(del, not_found_object) {
    bool res1 = config->del({"v1"}, "notfound");

    CHECK_EQUAL(false, res1);
}

TEST(del, top_level_array_value) {
    bool res1 = config->del({"vec1"}, "v9");
    std::vector<std::string> res2 = config->getVec({"vec1"});

    CHECK_EQUAL(true, res1);
    CHECK(res2.empty());
}

TEST(del, nested_array_value) {
    bool res1 = config->del({"key", "vec2"}, "v0");
    std::vector<std::string> res2 = config->getVec({"key", "vec2"});

    CHECK_EQUAL(true, res1);
    CHECK(std::vector<std::string>({"v11"}) == res2);
}

TEST(del, not_found_array_value) {
    bool res1 = config->del({"vec1"}, "notfound");

    CHECK_EQUAL(false, res1);
}

TEST(del, not_found_key) {
    bool res1 = config->del({"notfound"}, "notfound");

    CHECK_EQUAL(false, res1);
}
