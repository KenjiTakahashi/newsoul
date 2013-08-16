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
    private:
        int _length;
        int _bitrate;

    public:
        enum ReadStyle {
            Fast,
            Average,
            Accurate
        };

        AudioProperties(int length, int bitrate);
        virtual ~AudioProperties();

        virtual int length() const;
        virtual int bitrate() const;
    };

    class FileRef {
    private:
        TagLib::AudioProperties *ap;

    public:
        FileRef(const char *fn, bool rap=true, AudioProperties::ReadStyle aps=AudioProperties::Average);
        ~FileRef();

        AudioProperties *audioProperties();
        bool isNull() const;
    };
}

#endif // TAGLIB_FILEREF_H
