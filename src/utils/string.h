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

#ifndef __UTILS_STRING_H__
#define __UTILS_STRING_H__

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace string {
    /*!
     * Converts given string to lowercase.
     * \param s String to convert.
     * \return Lowercased copy of s.
     */
    std::string tolower(const std::string &s);
    /*!
     * Splits given string on specified delimiter.
     * Note that "delimiter" is actually a "list of delimiting chars",
     * so s="b,a.r", delim=",." will return {b,a,r}.
     * Possible empty strings are omitted in the result.
     * \param s String to split.
     * \param delim Delimiter string to split on.
     * \return Vector of string split by delimiter.
     */
    std::vector<std::string> split(const std::string &s, const std::string &delim);
    /*!
     * Replaces all occurences of a char with other char.
     * \param s String to replace on.
     * \param from Char to get replaced.
     * \param to Char to replace with.
     * \return Copy of s with from replaced by to.
     */
    std::string replace(const std::string &s, char from, char to);
    /*!
     * Same as above, but replacing whole string chunks.
     * \param s String to replace on.
     * \param from String chunk to get replaced.
     * \param to String chunk to replace with.
     * \return Copy of s with from replaced by to.
     */
    std::string replace(const std::string &s, const std::string &from, const std::string &to);
}

#endif /* __UTILS_STRING_H__ */
