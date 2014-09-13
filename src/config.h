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

#ifndef __NEWSOUL_CONFIG_H__
#define __NEWSOUL_CONFIG_H__

#include <cstring>
#include <initializer_list>
#include <fstream>
#include <map>
#include <streambuf>
#include <vector>

#include <json-c/json.h>

#include "component.h"
#include "utils/convert.h"
#include "utils/os.h"
#include "utils/path.h"
#include "utils/string.h"

namespace newsoul {
    class Config : public Component {
        std::string fn;
        bool autosave;
        struct json_object *json;

        /*!
         * Does real initialization, i.e. reads stream contents and
         * tries to parse JSON data from them.
         * \param is Input stream to read from.
         */
        void init(std::istream &is);

        /*!
         * General getter method.
         * \param keys List of keys which will be chained into JSON path.
         */
        struct json_object *get(std::initializer_list<const std::string> keys);
        /*!
         * General setter method.
         * If any key on the path does not exist, it will be created.
         * \param keys List of keys which will be chained into JSON path.
         * \param value Value to set at the end of the path.
         */
        void set(std::initializer_list<const std::string> keys, struct json_object *value);

    public:
        /*!
         * Overloaded constructor taking an input stream.
         * Meant for testing usage and should not be used
         * in a real application.
         * \param is Input stream.
         * \param autosave Whether to save on set and destruction.
         */
        Config(std::istream &is, bool autosave=false);
        /*!
         * Default constructor.
         * If no filename is supplied, tries to read from default
         * user config location, then falls back to /etc/newsoul
         * (on *nices only).
         * \param fn Filename to read from.
         * \param autosave Whether to save on set and destruction.
         */
        Config(const std::string &fn="", bool autosave=true);
        ~Config();

        /*!
         * Saves current JSON state to disk.
         */
        void save();

        /*!
         * Gets a value of type integer.
         * \param keys List of keys which will be chained into JSON path.
         * \return Read value or 0 by default.
         */
        int getInt(std::initializer_list<const std::string> keys);
        /*!
         * Gets a value of type string.
         * \param keys List of keys which will be chained into JSON path.
         * \return Read value or empty string by default.
         */
        const std::string getStr(std::initializer_list<const std::string> keys);
        /*!
         * Gets a value of type boolean.
         * \param keys List of keys which will be chained into JSON path.
         * \return Read value or false by default.
         */
        bool getBool(std::initializer_list<const std::string> keys);
        /*!
         * Gets a list of items.
         * \param keys List of keys which will be chained into JSON path.
         * \return Read value or empty vector.
         */
        std::vector<std::string> getVec(std::initializer_list<const std::string> keys);

        /*!
         * Sets value of type integer.
         * \param keys List of keys which will be chained into JSON path.
         * \param value Value to set.
         */
        void set(std::initializer_list<const std::string> keys, int value);
        /*!
         * Sets value of type string.
         * \param keys List of keys which will be chained into JSON path.
         * \param value Value to set.
         */
        void set(std::initializer_list<const std::string> keys, const std::string &value);
        /*!
         * Sets value of type boolean.
         * \param keys List of keys which will be chained into JSON path.
         * \param value Value to set.
         */
        void set(std::initializer_list<const std::string> keys, bool value);

        /*!
         * Checks whether given value is contained within a JSON array.
         * \param keys List of keys which will be chained into JSON path.
         * \return True if given JSON path contains value, false otherwise.
         */
        bool contains(std::initializer_list<const std::string> keys, const std::string &value);

        /*!
         * Adds an item to array. If key path exists, last pieces' value
         * will be overwritten by new array with value provided as parameter.
         * Note: Array is actually a set, i.e. it ignores duplicates.
         * \param keys List of keys which will be chained into JSON path.
         * \param value Value to add to array pointed to by key path.
         */
        void add(std::initializer_list<const std::string> keys, const std::string &value);

        /*!
         * Deletes a key, along with all objects belonging to it.
         * \param keys List of keys which will be chained into JSON path.
         * \param key key to remove. It might also be an element of array.
         * \return False if no such key was found (i.e. nothing got deleted).
         */
        bool del(std::initializer_list<const std::string> keys, const std::string &key);

        /*!
         * Compatibility layer for clients using old-style
         * configuration structure.
         */
        typedef std::map<std::string, std::map<std::string, std::string>> CompatConfig;
        const CompatConfig compatData();
    };
}

#endif // __NEWSOUL_CONFIG_H__
