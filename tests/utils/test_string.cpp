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

#include <CppUTest/TestHarness.h>
#include "../../src/utils/string.h"

namespace newsoul {
    TEST_GROUP(tolower) { };
    TEST(tolower, all_lower) {
        std::string result = string::tolower("tolower");

        CHECK_EQUAL("tolower", result);
    }
    TEST(tolower, all_upper) {
        std::string result = string::tolower("TOLOWER");

        CHECK_EQUAL("tolower", result);
    }
    TEST(tolower, first_upper) {
        std::string result = string::tolower("Tolower");

        CHECK_EQUAL("tolower", result);
    }
    TEST(tolower, some_upper) {
        std::string result = string::tolower("ToLoWeR");

        CHECK_EQUAL("tolower", result);
    }

    TEST_GROUP(split) { };
    TEST(split, split_string_on_comma) {
        std::vector<std::string> result = string::split("split,string", ",");

        CHECK(std::vector<std::string>({"split", "string"}) == result);
    }
    TEST(split, split_string_on_comma_and_period) {
        std::vector<std::string> result = string::split("split,.string", ",.");

        CHECK(std::vector<std::string>({"split", "string"}) == result);
    }
    TEST(split, split_long_string_on_comma_and_period) {
        std::vector<std::string> result = string::split("split,long.string", ",.");

        CHECK(std::vector<std::string>({"split", "long", "string"}) == result);
    }
    TEST(split, split_string_on_comma2) {
        std::vector<std::string> result = string::split("split,,string", ",");

        CHECK(std::vector<std::string>({"split", "string"}) == result);
    }
    TEST(split, split_string_on_comma3) {
        std::vector<std::string> result = string::split(",split,string", ",");

        CHECK(std::vector<std::string>({"split", "string"}) == result);
    }
    TEST(split, split_string_on_comma4) {
        std::vector<std::string> result = string::split("split,string,", ",");

        CHECK(std::vector<std::string>({"split", "string"}) == result);
    }

    TEST_GROUP(replace) { };
    TEST(replace, replace_e_with_y) {
        std::string result = string::replace("replace", 'e', 'y');

        CHECK_EQUAL("ryplacy", result);
    }
    TEST(replace, replace_rep_with_bar) {
        std::string result = string::replace("replace", "rep", "bar");

        CHECK_EQUAL("barlace", result);
    }
    TEST(replace, replace_rap_with_bar) {
        std::string result = string::replace("replace", "rap", "bar");

        CHECK_EQUAL("replace", result);
    }

    TEST_GROUP(string_join) { };
    TEST(string_join, one) {
        std::string result = string::join({"one"}, ";");

        CHECK_EQUAL("one", result);
    }
    TEST(string_join, two) {
        std::string result = string::join({"one", "two"}, ";");

        CHECK_EQUAL("one;two", result);
    }
    TEST(string_join, multichar_delim) {
        std::string result = string::join({"one", "two"}, ",;");

        CHECK_EQUAL("one,;two", result);
    }
}
