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

#include "path.h"

std::string path::join(std::initializer_list<const std::string> paths) {
    std::string result;
    char sep = os::separator();

    for(const std::string &path : paths) {
        if(result.empty()) {
            result = path;
            continue;
        }
        if(result.back() != sep && path.front() != sep) {
            result += sep + path;
        } else if(result.back() == sep && path.front() == sep) {
            result += std::string(path.begin() + 1, path.end());
        } else {
            result += path;
        }
    }

    return result;
}

std::string path::expand(const std::string &path) {
#ifndef _WIN32
    wordexp_t ep;
    if(wordexp(path.c_str(), &ep, WRDE_NOCMD) != 0) {
        return path;
    }
    std::string result(ep.we_wordv[0]);
    wordfree(&ep);
    return result;
#else
    return path; //TODO: Real implementation (using ExpandEnvironmentStrings?)
#endif
}
