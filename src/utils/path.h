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

#ifndef __UTILS_PATH_H__
#define __UTILS_PATH_H__

#include <initializer_list>
#include <string>
#ifndef _WIN32
#include <wordexp.h>
#endif
#include "os.h"

namespace path {
    /*!
     * Joins given list a path chunks into one, proper path.
     * Detects whether on windows or *NIX and uses proper delimiter.
     * Makes sure that there are no double delimiters on join places.
     * \param paths List of path chunks to join.
     * \return All paths joined into one.
     */
    std::string join(std::initializer_list<const std::string> paths);

    /*!
     * Expands environment variables and other things, like '~' char.
     * Does not, however, expand commands.
     * \param path Path for which to expand vars.
     * \return New path with vars expanded.
     */
    std::string expand(const std::string &path);

    /*!
     * Checks whether given path is absolute.
     * \param path Path to check.
     * \return True if path is absolute, false otherwise.
     */
    bool isAbsolute(const std::string &path);
}

#endif /* __UTILS_PATH_H__ */
