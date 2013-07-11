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

#include "config.h"

void newsoul::Config::init(std::istream &is) {
    std::string j((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());

    this->json = json_tokener_parse(j.c_str());

    if(this->json == NULL) {
        //TODO: load defaults
        this->json = json_tokener_parse("{}");
    }
}

newsoul::Config::Config(std::istream &is, bool autosave) : autosave(autosave) {
    this->init(is);
}

newsoul::Config::Config(const std::string &fn, bool autosave) : fn(fn), autosave(autosave) {
    std::ifstream f(this->fn);
    this->init(f);
}

newsoul::Config::~Config() {
    this->save();
    json_object_put(this->json);
}

void newsoul::Config::save() {
    if(this->autosave) {
        std::ofstream f(this->fn);
        const char *jsonstring = json_object_to_json_string(this->json);
        f.write(jsonstring, strlen(jsonstring));
        f.close();
    }
}

struct json_object *newsoul::Config::get(std::initializer_list<const std::string> keys) {
    struct json_object *part = this->json;

    for(const std::string &key : keys) {
        part = json_object_object_get(part, key.c_str());
    }

    return part;
}

int newsoul::Config::getInt(std::initializer_list<const std::string> keys) {
    struct json_object *result = this->get(keys);
    return json_object_get_int(result);
}

const std::string newsoul::Config::getStr(std::initializer_list<const std::string> keys) {
    struct json_object *result = this->get(keys);
    if(result == NULL) {
        return std::string();
    }
    return std::string(json_object_get_string(result));
}

void newsoul::Config::set(std::initializer_list<const std::string> keys, struct json_object *value) {
    struct json_object *copy = this->json;
    struct json_object *part;

    for(auto key = keys.begin(); key < keys.end() - 1; ++key) {
        part = json_object_object_get(copy, (*key).c_str());
        if(part == NULL) {
            part = json_object_new_object();
            json_object_object_add(copy, (*key).c_str(), part);
        }
        copy = part;
    }
    json_object_object_add(copy, (*(keys.end() - 1)).c_str(), value);
}

void newsoul::Config::set(std::initializer_list<const std::string> keys, int value) {
    this->set(keys, json_object_new_int(value));
    this->save();
}

void newsoul::Config::set(std::initializer_list<const std::string> keys, const std::string &value) {
    this->set(keys, json_object_new_string(value.c_str()));
    this->save();
}
