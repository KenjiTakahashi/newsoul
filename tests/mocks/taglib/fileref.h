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

#ifndef TAGLIB_FILEREF_H
#define TAGLIB_FILEREF_H

#include <CppUTestExt/MockSupport.h>

namespace TagLib {
    class AudioProperties {
    public:
        enum ReadStyle {
            Fast,
            Average,
            Accurate
        };

        virtual int length() const = 0;
        virtual int bitrate() const = 0;
    };

    class FileRef {
    public:
        FileRef(const char *fn, bool rap=true, AudioProperties::ReadStyle aps=AudioProperties::Average);
        ~FileRef();

        AudioProperties *audioProperties() const;
        bool isNull() const;
    };
}

#endif // TAGLIB_FILEREF_H
