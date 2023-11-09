/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/
/* Based on luafilesystem
Copyright © 2003-2014 Kepler Project.

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <algorithm>
#include <string>

#include "modules/Filesystem.h"

using namespace DFHack;

static bool initialized = false;
static std::string initial_cwd;

void Filesystem::init ()
{
    if (!initialized)
    {
        initialized = true;
        initial_cwd = Filesystem::getcwd();
    }
}

bool Filesystem::chdir (std::string path)
{
    Filesystem::init();
    return ::chdir(path.c_str()) == 0;
}

std::string Filesystem::getcwd ()
{
    char *path;
    char buf[FILENAME_MAX];
    std::string result = "";
#ifdef _WIN32
    if ((path = ::_getcwd(buf, FILENAME_MAX)) != NULL)
#else
    if ((path = ::getcwd(buf, FILENAME_MAX)) != NULL)
#endif
    result = buf;
    return result;
}

bool Filesystem::restore_cwd ()
{
    return Filesystem::chdir(initial_cwd);
}

std::string Filesystem::get_initial_cwd ()
{
    Filesystem::init();
    return initial_cwd;
}

bool Filesystem::mkdir (std::string path)
{
    int fail;
#ifdef _WIN32
    fail = ::_mkdir(path.c_str());
#else
    fail = ::mkdir(path.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP |
                   S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH);
#endif
    return fail == 0;
}

static bool mkdir_recursive_impl (std::string path)
{
    size_t last_slash = path.find_last_of("/");
    if (last_slash != std::string::npos)
    {
        std::string parent_path = path.substr(0, last_slash);
        bool parent_exists = mkdir_recursive_impl(parent_path);
        if (!parent_exists)
        {
            return false;
        }
    }
    return (Filesystem::mkdir(path) || errno == EEXIST) && Filesystem::isdir(path);
}

bool Filesystem::mkdir_recursive (std::string path)
{
#ifdef _WIN32
    // normalize to forward slashes
    std::replace(path.begin(), path.end(), '\\', '/');
#endif

    if (path.size() > FILENAME_MAX)
    {
        // path too long
        return false;
    }

    return mkdir_recursive_impl(path);
}

bool Filesystem::rmdir (std::string path)
{
    int fail;
#ifdef _WIN32
    fail = ::_rmdir(path.c_str());
#else
    fail = ::rmdir(path.c_str());
#endif
    return fail == 0;
}

#ifdef _WIN32
_filetype mode2type (unsigned short mode) {
#else
_filetype mode2type (mode_t mode) {
#endif
    if (S_ISREG(mode))
        return FILETYPE_FILE;
    else if (S_ISDIR(mode))
        return FILETYPE_DIRECTORY;
    else if (S_ISLNK(mode))
        return FILETYPE_LINK;
    else if (S_ISSOCK(mode))
        return FILETYPE_SOCKET;
    else if (S_ISCHR(mode))
        return FILETYPE_CHAR_DEVICE;
    else if (S_ISBLK(mode))
        return FILETYPE_BLOCK_DEVICE;
    else
        return FILETYPE_UNKNOWN;
}

bool Filesystem::stat (std::string path, STAT_STRUCT &info)
{
    return (STAT_FUNC(path.c_str(), &info)) == 0;
}

bool Filesystem::exists (std::string path)
{
    STAT_STRUCT info;
    return Filesystem::stat(path, info);
}

_filetype Filesystem::filetype (std::string path)
{
    STAT_STRUCT info;
    if (!Filesystem::stat(path, info))
        return FILETYPE_NONE;
    return mode2type(info.st_mode);
}

bool Filesystem::isfile (std::string path)
{
    return Filesystem::exists(path) && Filesystem::filetype(path) == FILETYPE_FILE;
}

bool Filesystem::isdir (std::string path)
{
    return Filesystem::exists(path) && Filesystem::filetype(path) == FILETYPE_DIRECTORY;
}

#define DEFINE_STAT_TIME_WRAPPER(attr) \
int64_t Filesystem::attr (std::string path) \
{ \
    STAT_STRUCT info; \
    if (!Filesystem::stat(path, info)) \
        return -1; \
    return (int64_t)info.st_##attr; \
}

DEFINE_STAT_TIME_WRAPPER(atime)
DEFINE_STAT_TIME_WRAPPER(ctime)
DEFINE_STAT_TIME_WRAPPER(mtime)

#undef DEFINE_STAT_TIME_WRAPPER

int Filesystem::listdir (std::string dir, std::vector<std::string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL)
    {
        return errno;
    }
    while ((dirp = readdir(dp)) != NULL) {
        files.push_back(std::string(dirp->d_name));
    }
    closedir(dp);
    return 0;
}

// prefix is the top-level dir where we started recursing
// path is the relative path under the prefix; must be empty or end in a '/'
// files is the output list of files and directories (bool == true for dir)
// depth is the remaining dir depth to recurse into. function returns -1 if
//   we haven't finished recursing when we run out of depth.
// include_prefix controls whether the directory where we started recursing is
//   included in the filenames returned in files.
static int listdir_recursive_impl (std::string prefix, std::string path,
    std::map<std::string, bool> &files, int depth, bool include_prefix)
{
    if (depth < 0)
        return -1;
    std::string prefixed_path = prefix + "/" + path;
    std::vector<std::string> curdir_files;
    int err = Filesystem::listdir(prefixed_path, curdir_files);
    if (err)
        return err;
    bool out_of_depth = false;
    for (auto file = curdir_files.begin(); file != curdir_files.end(); ++file)
    {
        if (*file == "." || *file == "..")
            continue;
        std::string prefixed_file = prefixed_path + *file;
        std::string path_file = path + *file;
        if (Filesystem::isdir(prefixed_file))
        {
            files.insert(std::pair<std::string, bool>(include_prefix ? prefixed_file : path_file, true));

            if (depth == 0)
            {
                out_of_depth = true;
                continue;
            }

            err = listdir_recursive_impl(prefix, path_file + "/", files, depth - 1, include_prefix);
            if (err)
                return err;
        }
        else
        {
            files.insert(std::pair<std::string, bool>(include_prefix ? prefixed_file : path_file, false));
        }
    }
    return out_of_depth ? -1 : 0;
}

int Filesystem::listdir_recursive (std::string dir, std::map<std::string, bool> &files,
    int depth /* = 10 */, bool include_prefix /* = true */)
{
    if (dir.size() && dir[dir.size()-1] == '/')
        dir.resize(dir.size()-1);
    return listdir_recursive_impl(dir, "", files, depth, include_prefix);
}
