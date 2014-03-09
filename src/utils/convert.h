/*
 This is a part of newsoul @ http://github.com/KenjiTakahashi/newsoul
 Karol "Kenji Takahashi" Woźniak © 2014

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

#ifndef __UTILS_CONVERT_H__
#define __UTILS_CONVERT_H__

#include <cstdlib>
#include <string>
#include "string.h"

namespace convert {
    /*!
     * Converts string to bool, i.e. "true" is true and any other
     * string is false. Case insensitive.
     * \param str String to convert.
     * \return true or false.
     */
    bool string2bool(const std::string &str);
}

#endif /* __UTILS_CONVERT_H__ */
