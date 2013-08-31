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

#include "fileref.h"

TagLib::AudioProperties::AudioProperties(int length, int bitrate) {
    this->_length = length;
    this->_bitrate = bitrate;
}
TagLib::AudioProperties::~AudioProperties() { }
int TagLib::AudioProperties::length() const {
    return this->_length;
}
int TagLib::AudioProperties::bitrate() const {
    return this->_bitrate;
}

TagLib::FileRef::FileRef(const char *fn, bool rap, TagLib::AudioProperties::ReadStyle aps) {
    mock().actualCall("FileRef::FileRef").withParameter("1", fn);
    this->ap = NULL;
}
TagLib::FileRef::~FileRef() {
    if(this->ap != NULL) {
        delete this->ap;
    }
}
TagLib::AudioProperties *TagLib::FileRef::audioProperties() {
    if(this->ap == NULL) {
        this->ap = new TagLib::AudioProperties(10, 1);
    }
    return this->ap;
}
bool TagLib::FileRef::isNull() const {
    return mock().getData("TagLib::isNull").getIntValue();
}
