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

#pragma once
#include "Export.h"
#include <map>
#include <vector>

#include <filesystem>


namespace DFHack {
    namespace Filesystem {
        DFHACK_EXPORT void init();
        DFHACK_EXPORT bool chdir(std::filesystem::path path) noexcept;
        DFHACK_EXPORT std::filesystem::path getcwd();
        DFHACK_EXPORT bool restore_cwd();
        DFHACK_EXPORT std::filesystem::path get_initial_cwd();
        DFHACK_EXPORT bool mkdir(std::filesystem::path path) noexcept;
        // returns true on success or if directory already exists
        DFHACK_EXPORT bool mkdir_recursive(std::filesystem::path path) noexcept;
        DFHACK_EXPORT bool rmdir(std::filesystem::path path) noexcept;
        DFHACK_EXPORT bool stat(std::filesystem::path path, std::filesystem::file_status& info) noexcept;
        DFHACK_EXPORT bool exists(std::filesystem::path path) noexcept;
        DFHACK_EXPORT bool isfile(std::filesystem::path path) noexcept;
        DFHACK_EXPORT bool isdir(std::filesystem::path path) noexcept;
        DFHACK_EXPORT std::time_t mtime(std::filesystem::path path) noexcept;
        DFHACK_EXPORT int listdir(std::filesystem::path dir, std::vector<std::filesystem::path>& files) noexcept;
        // set include_prefix to false to prevent dir from being prepended to
        // paths returned in files
        DFHACK_EXPORT int listdir_recursive(std::filesystem::path dir, std::map<std::filesystem::path, bool>& files,
            int depth = 10, bool include_prefix = true) noexcept;
        DFHACK_EXPORT std::filesystem::path canonicalize(std::filesystem::path p) noexcept;
        inline std::string as_string(const std::filesystem::path path) noexcept
        {
            auto pStr = path.string();
            if constexpr (std::filesystem::path::preferred_separator != '/')
            {
                std::ranges::replace(pStr, std::filesystem::path::preferred_separator, '/');
            }
            return pStr;
        }
        DFHACK_EXPORT std::filesystem::path getInstallDir() noexcept;
        DFHACK_EXPORT std::filesystem::path getBaseDir() noexcept;

    }
}
