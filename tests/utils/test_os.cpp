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
#include <CppUTest/SimpleString.h>
#include <CppUTest/MemoryLeakWarningPlugin.h>
#include <CppUTest/TestHarness.h>
#include "../../src/utils/path.h"
#include "../../src/utils/os.h"

TEST_GROUP(mkdir) {
    std::string cwd;

    void setup() {
        char *cwdb = getcwd(NULL, 0);
        std::string cwd(cwdb); 
        MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();
        free(cwdb);
        MemoryLeakWarningPlugin::turnOnNewDeleteOverloads();
    }
};

TEST(mkdir, non_existing_top_level) {
    std::string path = path::join({cwd, "toplevel"});
    bool result = os::mkdir(path);

    struct stat st;
    int ret = stat(path.c_str(), &st);

    CHECK(result);
    CHECK_EQUAL(0, ret);
    CHECK(S_ISDIR(st.st_mode));

    rmdir(path.c_str());
}

TEST(mkdir, non_existing_two_levels) {
    std::string path = path::join({cwd, "sndlevel", "toplevel"});
    bool result = os::mkdir(path);

    struct stat st;
    int ret = stat(path.c_str(), &st);

    CHECK(result);
    CHECK_EQUAL(0, ret);
    CHECK(S_ISDIR(st.st_mode));

    rmdir(path.c_str());
    rmdir(path::join({cwd, "sndlevel"}).c_str());
}

TEST(mkdir, non_recursive) {
    std::string path = path::join({cwd, "sndlevel", "toplevel"});
    bool result = os::mkdir(path, false);

    struct stat st;
    int ret = stat(path.c_str(), &st);

    CHECK(!result);
    CHECK_EQUAL(-1, ret);
}

TEST(mkdir, ends_with_separator) {
    std::string path = path::join({cwd, "toplevel/"});
    bool result = os::mkdir(path);

    struct stat st;
    int ret = stat(path.c_str(), &st);

    CHECK(result);
    CHECK_EQUAL(0, ret);

    rmdir(path.c_str());
}

TEST_GROUP(separator) { };

TEST(separator, success) {
    const char result = os::separator();
#ifndef _WIN32
    CHECK_EQUAL('/', result);
#else
    CHECK_EQUAL('\\', result);
#endif
}
