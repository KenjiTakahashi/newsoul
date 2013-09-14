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
}

newsoul::Config::Config(std::istream &is, bool autosave) : autosave(autosave) {
    this->init(is);
}

newsoul::Config::Config(const std::string &fn, bool autosave) : fn(fn), autosave(autosave) {
    std::ifstream f(this->fn);
    if(!f.is_open() || !f.good()) {
        this->fn = path::join({os::config(), "newsoul", "config.json"});
        f.open(this->fn);
    }
#ifndef _WIN32
    if(!f.is_open() || !f.good()) {
        this->fn = path::join({"/etc", "newsoul", "config.json"});
        f.open(this->fn);
    }
#endif
    this->init(f);
}

newsoul::Config::~Config() {
    if(this->autosave) {
        this->save();
    }

    json_object_put(this->json);
}

void newsoul::Config::save() {
    std::ofstream f(this->fn);
    const char *jsonstring = json_object_to_json_string_ext(this->json, JSON_C_TO_STRING_PRETTY);
    f.write(jsonstring, strlen(jsonstring));
    f.close();
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

bool newsoul::Config::getBool(std::initializer_list<const std::string> keys) {
    struct json_object *result = this->get(keys);
    return json_object_get_boolean(result) == 1 ? true : false;
}

std::vector<std::string> newsoul::Config::getVec(std::initializer_list<const std::string> keys) {
    struct json_object *result = this->get(keys);
    std::vector<std::string> out;
    if(result == NULL || !json_object_is_type(result, json_type_array)) {
        return out;
    }

    for(int i = 0; i < json_object_array_length(result); ++i) {
        out.push_back(json_object_get_string(json_object_array_get_idx(result, i)));
    }

    return out;
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

    if(this->autosave) {
        this->save();
    }
}

void newsoul::Config::set(std::initializer_list<const std::string> keys, int value) {
    this->set(keys, json_object_new_int(value));
}

void newsoul::Config::set(std::initializer_list<const std::string> keys, const std::string &value) {
    this->set(keys, json_object_new_string(value.c_str()));
}

void newsoul::Config::set(std::initializer_list<const std::string> keys, bool value) {
    this->set(keys, json_object_new_boolean(value));
}

bool newsoul::Config::contains(std::initializer_list<const std::string> keys, const std::string &value) {
    struct json_object *result = this->get(keys);
    if(result == NULL || !json_object_is_type(result, json_type_array)) {
        return false;
    }

    for(int i = 0; i < json_object_array_length(result); ++i) {
        std::string s(json_object_get_string(json_object_array_get_idx(result, i)));
        if(s == value) {
            return true;
        }
    }
    return false;
}

void newsoul::Config::add(std::initializer_list<const std::string> keys, const std::string &value) {
    struct json_object *result = this->get(keys);
    if(result == NULL || !json_object_is_type(result, json_type_array)) {
        result = json_object_new_array();
        this->set(keys, result);
    }

    if(this->contains(keys, value)) {
        return;
    }

    json_object_array_add(result, json_object_new_string(value.c_str()));

    if(this->autosave) {
        this->save();
    }
}

bool newsoul::Config::del(std::initializer_list<const std::string> keys, const std::string &key) {
    struct json_object *result = this->get(keys);

    if(result != NULL) {
        if(json_object_object_get(result, key.c_str()) != NULL) {
            json_object_object_del(result, key.c_str());

            if(this->autosave) {
                this->save();
            }

            return true;
        } else if(json_object_is_type(result, json_type_array)) {
            //This is tricky, but I'm afraid there is no other way.
            struct json_object *narray = json_object_new_array();
            struct json_object *e;

            bool found = false;
            for(int i = 0; i < json_object_array_length(result); ++i) {
                e = json_object_array_get_idx(result, i);
                if(json_object_get_string(e) != key) {
                    json_object_array_add(narray, json_object_get(e));
                } else {
                    found = true;
                }
            }
            if(!found) {
                json_object_put(narray);
                return false;
            }

            struct json_object *jkey = this->json;
            const std::string skey = *(keys.end() - 1);
            for(auto key = keys.begin(); key < keys.end() - 1; ++key) {
                jkey = json_object_object_get(jkey, (*key).c_str());
            }

            json_object_object_del(jkey, skey.c_str());
            json_object_object_add(jkey, skey.c_str(), narray);

            if(this->autosave) {
                this->save();
            }

            return true;
        }
    }

    return false;
}
