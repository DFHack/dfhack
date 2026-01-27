#include <filesystem>
#include <fstream>
#include <vector>

namespace DFHack {

    const std::filesystem::path getInstallPath() noexcept;
    const std::filesystem::path getUserPath() noexcept;

    const bool isPortableMode() noexcept;

    const std::filesystem::path getBasePath() noexcept;
    const std::filesystem::path getAltBasePath() noexcept;

    const std::filesystem::path getBasePath(bool portable) noexcept;
    const std::filesystem::path getConfigPath(bool portable = false) noexcept;

    const std::vector<std::filesystem::path> getPossiblePaths(const std::filesystem::path file) noexcept;

    const std::filesystem::path getConfigDefaultsPath() noexcept;

    const std::filesystem::path findConfigFile(const std::filesystem::path filename);
    std::ifstream openConfigFile(std::filesystem::path filename);

    const std::filesystem::path findFile(const std::filesystem::path filename);
    std::ifstream openFile(std::filesystem::path filename);



}
