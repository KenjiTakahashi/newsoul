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

#ifndef __UTILS_PATH_H__
#define __UTILS_PATH_H__

#include <initializer_list>
#include <pcrecpp.h>
#include <pwd.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include "os.h"
#include "string.h"

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
     * Splits given path into separate directories/file names.
     * The input path should be normalized (so not contain '..', etc.).
     * The second argument specifies maximum number of splits to perform,
     * counting right to left. This enables getting e.g. dirname/basename
     * split by calling `split(<path>, 1)`.
     * \param path Path to split.
     * \param no Maximum number of pieces
     * \return Vector of split pieces.
     */
    std::vector<std::string> split(const std::string path, int no=-1);
    /*!
     * Expands '~' and '~user` to user home directory.
     * \param path Path to expand.
     * \return New path with expanded home directory.
     */
    std::string expanduser(const std::string &path);
    /*!
     * Expands shell variables of form $var and ${var}.
     * Unknown variables are left unchanged.
     * \param path Path to expand.
     * \return New path with expanded variables.
     */
    std::string expandvars(const std::string &path);
    /*!
     * Normalizes a path, e.g. a//b, a/./b and a/f/../b all become a/b.
     * Note that this may change the meaning of given path if it contains
     * symlinks.
     * \param path Path to normalize.
     * \return New, normalized path.
     */
    std::string normpath(const std::string &path);
    /*!
     * Expands env vars, tilde and '..'.
     * It is implemented by calling `expanduser`, `expandvars` and `normpath`.
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
