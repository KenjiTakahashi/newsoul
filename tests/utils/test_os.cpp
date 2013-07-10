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

#ifndef _WIN32
#include <unistd.h>
#else
#define rmdir(path) _rmdir(path)
#endif
#include <cstdlib>
#include "../catch.hpp"
#include "../../src/utils/path.h"
#include "../../src/utils/os.h"

TEST_CASE("mkdir", "[utils][os]") {
    char *cwdb = getcwd(NULL, 0);
    std::string cwd(cwdb); 
    free(cwdb);
    SECTION("non-existing top level") {
        std::string path = path::join({cwd, "toplevel"});
        bool result = os::mkdir(path);

        struct stat st;
        int ret = stat(path.c_str(), &st);

        REQUIRE(result);
        REQUIRE(ret == 0);
        REQUIRE(S_ISDIR(st.st_mode));

        rmdir(path.c_str());
    }

    SECTION("non-existing two levels") {
        std::string path = path::join({cwd, "sndlevel", "toplevel"});
        bool result = os::mkdir(path);

        struct stat st;
        int ret = stat(path.c_str(), &st);

        REQUIRE(result);
        REQUIRE(ret == 0);
        REQUIRE(S_ISDIR(st.st_mode));

        rmdir(path.c_str());
        rmdir(path::join({cwd, "sndlevel"}).c_str());
    }

    SECTION("non-recursive") {
        std::string path = path::join({cwd, "sndlevel", "toplevel"});
        bool result = os::mkdir(path, false);

        struct stat st;
        int ret = stat(path.c_str(), &st);

        REQUIRE(!result);
        REQUIRE(ret == -1);
    }

    SECTION("ends with separator", "[corner]") {
        std::string path = path::join({cwd, "toplevel/"});
        bool result = os::mkdir(path);

        struct stat st;
        int ret = stat(path.c_str(), &st);

        REQUIRE(result);
        REQUIRE(ret == 0);

        rmdir(path.c_str());
    }
}

TEST_CASE("separator", "[utils][os]") {
    const char result = os::separator();
#ifndef _WIN32
    REQUIRE(result == '/');
#else
    REQUIRE(result == '\\');
#endif
}
