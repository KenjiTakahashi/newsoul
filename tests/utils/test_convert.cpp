/*
 This is a part of newsoul @ http://github.com/KenjiTakahashi/newsoul
 Karol "Kenji Takahashi" Woźniak © 2014

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
#include "../../src/utils/convert.h"

namespace newsoul {
    TEST_GROUP(string2bool) { };
    TEST(string2bool, low_true) {
        bool result = convert::string2bool("true");

        CHECK_EQUAL(true, result);
    }
    TEST(string2bool, mixed_true) {
        bool result = convert::string2bool("TrUe");

        CHECK_EQUAL(true, result);
    }
    TEST(string2bool, false_) {
        bool result = convert::string2bool("False");

        CHECK_EQUAL(false, result);
    }

    TEST_GROUP(bool2string) { };
    TEST(bool2string, true_) {
        const std::string result = convert::bool2string(true);

        CHECK_EQUAL("true", result);
    }
    TEST(bool2string, false_) {
        const std::string result = convert::bool2string(false);

        CHECK_EQUAL("false", result);
    }
}
