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

#ifndef __NEWSOUL_CONFIG_H__
#define __NEWSOUL_CONFIG_H__

#include <cstring>
#include <initializer_list>
#include <json-c/json.h>
#include <fstream>
#ifdef _WIN32
#include <shlobj.h>
#endif
#include <streambuf>
#include "utils/os.h"

namespace newsoul {
    class Config {
        const std::string fn;
        bool autosave;
        struct json_object *json;

        void init(std::istream &is);

        struct json_object *get(std::initializer_list<const std::string> keys);
        void set(std::initializer_list<const std::string> keys, struct json_object *value);

    public:
        Config(std::istream &is, bool autosave=true);
        Config(const std::string &fn="", bool autosave=true);
        ~Config();

        void save();

        int getInt(std::initializer_list<const std::string> keys);
        const std::string getStr(std::initializer_list<const std::string> keys);
        void set(std::initializer_list<const std::string> keys, int value);
        void set(std::initializer_list<const std::string> keys, const std::string &value);
    };
}

#endif // __NEWSOUL_CONFIG_H__
