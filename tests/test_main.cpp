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

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestRegistry.h>
#include <CppUTestExt/MockSupportPlugin.h>

class AttrsComparator : public MockNamedValueComparator {
public:
    bool isEqual(void *obj1, void *obj2) {
        int *o1 = (int*)obj1;
        int *o2 = (int*)obj2;
        if(o1 == NULL && o2 != NULL) {
            return false;
        } else if(o1 != NULL && o2 == NULL) {
            return false;
        } else if(o1 == NULL && o2 == NULL) {
            return true;
        }
        bool res = true;
        for(int i = 0; i < 3; ++i) {
            res = o1[i] == o2[i];
        }
        return res;
    }

    SimpleString valueToString(void *obj) {
        return StringFrom(obj);
    }
};

int main(int argc, char *argv[]) {
    AttrsComparator attrsComparator;
    MockSupportPlugin mockPlugin;
    mockPlugin.installComparator("attrs", attrsComparator);
    TestRegistry::getCurrentRegistry()->installPlugin(&mockPlugin);
    return CommandLineTestRunner::RunAllTests(argc, argv);
}
