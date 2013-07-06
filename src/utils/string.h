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

#ifndef __UTILS_STRING_H__
#define __UTILS_STRING_H__

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace string {
    std::string tolower(const std::string &s);
    std::vector<std::string> split(const std::string &s, const std::string &delim);
    std::string replace(const std::string &s, char from, char to);
    std::string replace(const std::string &s, const std::string &from, const std::string &to);
}

#endif /* __UTILS_STRING_H__ */
