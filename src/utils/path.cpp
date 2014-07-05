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

std::vector<std::string> path::split(const std::string path, int no) {
    std::vector<std::string> results;
    std::string dirname(path);
    size_t index;
    int it = 0;
    do {
        if(no > 0 && it++ >= no) {
            index = std::string::npos;
        } else {
            index = dirname.rfind(os::separator());
        }

        std::string basename = dirname.substr(index + 1);
        if(!basename.empty()) {
            results.push_back(basename);
        }

        dirname = dirname.substr(0, index);
    } while(index != std::string::npos);
    std::reverse(results.begin(), results.end());
    return results;
}

std::string path::expanduser(const std::string &path) {
    if(path[0] != '~') {
        return std::string(path);
    }
    size_t i = path.find('/');
    if(i == std::string::npos) {
        i = path.length();
    }
    std::string home;
    if(i == 1) {
        char *_home = getenv("HOME");
        if(_home != NULL) {
            home = std::string(_home);
        } else {
            home = std::string(getpwuid(getuid())->pw_dir);
        }
    } else {
        struct passwd *pwent = getpwnam(path.substr(1, i).c_str());
        if(pwent != NULL) {
            home = std::string(pwent->pw_dir);
        } else {
            i = 0;
        }
    }
    return std::string(home) + path.substr(i);
}

std::string path::expandvars(const std::string &path) {
    std::string new_path(path);
    if(new_path.find('$') == std::string::npos) {
        return new_path;
    }
    pcrecpp::StringPiece path_re(new_path);
    pcrecpp::RE expandvars_re(".*\\$(\\w+|\\{[^}]*\\})");
    int offset = 0;
    std::string _;
    std::string varname;
    while(expandvars_re.Consume(&path_re, &varname)) {
        int varname_size = varname.size();
        if(varname.find('{') == 0 && varname.find('}') == varname.size() - 1) {
            varname = varname.substr(1, varname.length() - 2);
        }
        char *_varval = getenv(varname.c_str());
        if(_varval != NULL) {
            std::string varval(_varval);
            offset = new_path.size() - path_re.size();
            std::string tail(new_path.substr(offset));
            new_path = new_path.substr(0, offset - varname_size - 1) + varval;
            offset = new_path.size();
            new_path += tail;
        } else {
            offset = new_path.size() - path_re.size();
        }
    }
    return new_path;
}

std::string path::normpath(const std::string &path) {
    std::string dot(".");
    if(path.empty()) {
        return dot;
    }
    bool init_slashes = path[0] == '/';
    // POSIX allows two initial slashes, but three or more are treated as one.
    bool two_slashes = false;
    if(init_slashes && path[1] == '/' && path[2] != '/') {
        two_slashes = true;
    }
    auto comps = newsoul::string::split(path, "/");
    std::vector<std::string> new_comps;
    for(std::string &comp : comps) {
        if(comp.empty() || comp == ".") {
            continue;
        }
        bool append = (
            comp != ".." ||
            (!init_slashes && new_comps.empty()) ||
            (!new_comps.empty() && new_comps.back() == "..")
        );
        if(append) {
            new_comps.push_back(comp);
        } else if(!new_comps.empty()) {
            new_comps.pop_back();
        }
    }
    std::string new_path = newsoul::string::join(new_comps, "/");
    if(init_slashes) {
        new_path = "/" + new_path;
    }
    if(two_slashes) {
        new_path = "/" + new_path;
    }
    if(new_path.empty()) {
        return dot;
    }
    return new_path;
}

std::string path::expand(const std::string &path) {
    return normpath(expandvars(expanduser(path)));
}

bool path::isAbsolute(const std::string &path) {
#ifndef _WIN32
    return !path.empty() && path[0] == '/';
#else
    return path.length() >= 3 && path.substr(1, 2) == ":\\";
#endif
}
