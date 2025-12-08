#include <filesystem>
#include <vector>

namespace DFHack {

    const std::filesystem::path getInstallPath() noexcept;
    const std::filesystem::path getUserPath() noexcept;

    const bool isPortableMode() noexcept;

    const std::filesystem::path getBasePath() noexcept;
    const std::filesystem::path getAltBasePath() noexcept;

    const std::filesystem::path getBasePath(bool portable) noexcept;
    const std::filesystem::path getConfigPath(bool portable = false) noexcept;

    const std::vector<std::filesystem::path> getPossiblePaths(std::filesystem::path file) noexcept;

    const std::filesystem::path getConfigDefaultsPath() noexcept;

}
