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
#include <filesystem>
#include <chrono>

#include "modules/Filesystem.h"

using namespace DFHack;

static bool initialized = false;
static std::filesystem::path initial_cwd;

void Filesystem::init ()
{
    if (!initialized)
    {
        initialized = true;
        initial_cwd = Filesystem::getcwd();
    }
}

bool Filesystem::chdir (std::filesystem::path path)
{
    Filesystem::init();
    try
    {
        std::filesystem::current_path(path);
        return true;
    }
    catch (std::filesystem::filesystem_error&)
    {
        return false;
    }
}

std::filesystem::path Filesystem::getcwd ()
{
    return std::filesystem::current_path();
}

bool Filesystem::restore_cwd ()
{
    return Filesystem::chdir(initial_cwd);
}

std::filesystem::path Filesystem::get_initial_cwd ()
{
    Filesystem::init();
    return initial_cwd;
}

bool Filesystem::mkdir (std::filesystem::path path)
{
    try
    {
        std::filesystem::create_directory(path);
        return true;
    }
    catch (std::filesystem::filesystem_error&)
    {
        return false;
    }
}

bool Filesystem::mkdir_recursive (std::filesystem::path path)
{
    try
    {
        std::filesystem::create_directories(path);
        return true;
    }
    catch (std::filesystem::filesystem_error&)
    {
        return false;
    }
}

bool Filesystem::rmdir (std::filesystem::path path)
{
    try
    {
        std::filesystem::remove(path);
        return true;
    }
    catch (std::filesystem::filesystem_error&)
    {
        return false;
    }
}

bool Filesystem::stat (std::filesystem::path path, std::filesystem::file_status &info)
{
    try
    {
        info = std::filesystem::status(path);
        return true;
    }
    catch (std::filesystem::filesystem_error&)
    {
        return false;
    }
}

bool Filesystem::exists (std::filesystem::path path)
{
    return std::filesystem::exists(path);
}

bool Filesystem::isfile(std::filesystem::path path)
{
    return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
}

bool Filesystem::isdir (std::filesystem::path path)
{
    return std::filesystem::exists(path) && std::filesystem::is_directory(path);
}

std::time_t Filesystem::mtime (std::filesystem::path path)
{
    try
    {
        auto ftime = std::filesystem::last_write_time(path);
        return ftime.time_since_epoch().count();
    }
    catch (std::filesystem::filesystem_error&)
    {
        return 0;
    }
}

int Filesystem::listdir (std::filesystem::path dir, std::vector<std::filesystem::path > &files)
{
    try {
        for (auto const& dirent : std::filesystem::directory_iterator(dir))
        {
            files.push_back(dirent.path().filename());
        }
        return 0;
    }
    catch (std::filesystem::filesystem_error&)
    {
        return 1;
    }
}

int Filesystem::listdir_recursive (std::filesystem::path dir, std::map<std::filesystem::path, bool> &files,
    int depth /* = 10 */, bool include_prefix /* = true */)
{
    try {
        for (auto i = std::filesystem::recursive_directory_iterator(dir);
            i != std::filesystem::recursive_directory_iterator();
            ++i)
        {
            if (i->is_directory() && i.depth() >= depth)
                i.disable_recursion_pending();
            auto dirent = include_prefix ? *i : std::filesystem::relative(dir, *i);
            files.emplace(dirent, i->is_directory());
        }
        return 0;
    }
    catch (std::filesystem::filesystem_error&)
    {
        return 1;
    }
}

std::filesystem::path Filesystem::canonicalize(std::filesystem::path p)
{
    return std::filesystem::canonical(p);
}
