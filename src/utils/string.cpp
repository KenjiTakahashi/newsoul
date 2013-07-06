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

#include "string.h"

std::string string::tolower(const std::string &s) {
    std::string lowered(s);

    std::transform(s.begin(), s.end(), lowered.begin(), static_cast<int(*)(int)>(std::tolower));

    return lowered;
}

std::vector<std::string> string::split(const std::string &s, const std::string &delim) {
    std::vector<std::string> result;

    std::string::size_type last = s.find_first_not_of(delim, 0);
    std::string::size_type curr = s.find_first_of(delim, last);
    while(std::string::npos != curr || std::string::npos != last) {
        result.push_back(s.substr(last, curr - last));
        last = s.find_first_not_of(delim, curr);
        curr = s.find_first_of(delim, last);
    }

    return result;
}

std::string string::replace(const std::string &s, char from, char to) {
    std::string replaced(s);

    std::replace_copy(s.begin(), s.end(), replaced.begin(), from, to);

    return replaced;
}

std::string string::replace(const std::string &s, const std::string &from, const std::string &to) {
    std::string replaced(s);

    std::string::size_type pos = 0;
    while((pos = replaced.find(from, pos)) != std::string::npos) {
        replaced.replace(pos, from.length(), to);
        pos += to.length();
    }

    return replaced;
}
