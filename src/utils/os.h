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

#ifndef __UTILS_OS_H__
#define __UTILS_OS_H__

#include <cstdlib>
#include <string>
#ifndef _WIN32
#include <sys/stat.h>
#else
#include <direct.h>
#include <shlobj.h>
#include <string.h>
#define mkdir(path, mode) _mkdir(path)
#endif

namespace os {
    /*!
     * Creates specified filesystem path, if doesn't exist.
     * By default, tries to create all missing directories on the way.
     * \param path Path to create.
     * \param recursive When false, creates only last dir or fails.
     * \return True on success, false otherwise.
     */
    bool mkdir(const std::string &path, bool recursive=true);
    /*!
     * Returns filesystem path separator used by OS.
     */
    const char separator();
    /*!
     * Returns OS specific config directory.
     * On *nices, tries to get XDG_CONFIG_HOME and fallbacks
     * to $HOME/.config.
     * On Windows, gets FOLDERID_RoamingAppData.
     *
     * It does not perform any security checks, so if user has
     * a weird $HOME, he might get his cat eaten.
     */
    const std::string config();
}

#endif // __UTILS_OS_H__
