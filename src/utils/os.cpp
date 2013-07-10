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

#include "os.h"
#include <iostream>
#include <cstring>
#include <errno.h>

bool os::mkdir(const std::string &path, bool recursive) {
    size_t pos = path.rfind(separator());
    if(recursive && pos != std::string::npos) {
        const std::string piece = path.substr(0, pos);
        mkdir(piece);
    }
    if(::mkdir((path + separator()).c_str(), 0777) == -1 && errno != EEXIST) {
        std::cout << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

const char os::separator() {
#ifndef _WIN32
    return '/';
#else
    return '\\';
#endif
}
