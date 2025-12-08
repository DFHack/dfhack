#include <filesystem>
#include <fstream>
#include <vector>

#include "modules/DFSDL.h"
#include "Files.h"

#include "df/init.h"
#include "df/init_media_flags.h"

namespace DFHack
{
    namespace fs = std::filesystem;

    const fs::path getInstallPath() noexcept
    {
        return DFSDL::DFSDL_GetBasePath();
    };

    const fs::path getUserPath() noexcept
    {
        return DFSDL::DFSDL_GetPrefPath("Bay 12 Games", "Dwarf Fortress");
    };

    const bool isPortableMode() noexcept
    {
        return !df::global::init || df::global::init->media.flag.is_set(df::enums::init_media_flags::PORTABLE_MODE);
    }

    const fs::path getBasePath(bool portable) noexcept
    {
        return portable ? getInstallPath() : getUserPath();
    }

    const fs::path getBasePath() noexcept
    {
        return getBasePath(isPortableMode());
    }

    const fs::path getAltBasePath() noexcept
    {
        return getBasePath(!isPortableMode());
    }

    const fs::path getConfigPath(bool portable) noexcept
    {
        return getBasePath(portable) / "dfhack-config";
    };

    const fs::path getConfigDefaultsPath() noexcept
    {
        return getInstallPath() / "hack" / "data" / "dfhack-config-defaults";
    }

    const std::vector<fs::path> getPossiblePaths(fs::path filename) noexcept
    {
        bool portable = isPortableMode();
        return {getBasePath(portable) / filename, getBasePath(!portable) / filename};
    }

    std::ifstream openConfigFile(fs::path filename)
    {
        return openFile("dfhack-config" / filename);
    }

    std::ifstream openFile(fs::path filename)
    {
        std::ifstream file;
        fmt::print(stderr, "Trying to open file: {}\n", filename.string());
        for (const auto& path : getPossiblePaths(filename))
        {
            file.open(path);
            fmt::print(stderr, "Trying {}: {}\n", path.string(), file.is_open());
            if (file)
                break;
        }
        return file;
    }

}