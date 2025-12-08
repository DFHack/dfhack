#include <filesystem>
#include <vector>

#include "Modules/DFSDL.h"
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

    const std::vector<fs::path> getPossiblePaths(fs::path file) noexcept
    {
        bool portable = isPortableMode();
        return {getBasePath(portable) / file, getBasePath(!portable) / file};
    }

}