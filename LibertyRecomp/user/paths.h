#pragma once

#include <mod/mod_loader.h>

#define USER_DIRECTORY "LibertyRecomp"

#ifndef GAME_INSTALL_DIRECTORY
#define GAME_INSTALL_DIRECTORY "."
#endif

extern std::filesystem::path g_executableRoot;
inline std::unordered_map<std::string, std::filesystem::path> g_pathCache;

bool CheckPortable();
std::filesystem::path BuildUserPath();
const std::filesystem::path& GetUserPath();

inline std::filesystem::path GetGamePath()
{
    // Returns the game install directory
    // macOS: ~/Library/Application Support/LibertyRecomp/
    // Structure:
    //   game/           - extracted game files (common/, xbox360/, audio/)
    //   game/default.xex - the executable
    //   shader_cache/   - compiled shaders
    //   saves/          - save files
    return GetUserPath();
}

inline std::filesystem::path GetSavePath(bool checkForMods)
{
    if (checkForMods && !ModLoader::s_saveFilePath.empty())
        return ModLoader::s_saveFilePath.parent_path();
    else
        return GetUserPath() / "save";
}

// Returned file name may not necessarily be
// equal to SYS-DATA as mods can assign anything.
inline std::filesystem::path GetSaveFilePath(bool checkForMods)
{
    if (checkForMods && !ModLoader::s_saveFilePath.empty())
        return ModLoader::s_saveFilePath;
    else
        return GetSavePath(false) / "GTA4SaveData.bin";
}

static std::string toLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
    return str;
};

inline size_t BuildPathCache(const std::string& gamePath) {
    std::error_code ec;
    const auto options = std::filesystem::directory_options::skip_permission_denied;
    std::filesystem::recursive_directory_iterator it(gamePath, options, ec);
    std::filesystem::recursive_directory_iterator end;

    if (ec) {
        return 0;
    }

    size_t entriesAdded = 0;
    while (it != end) {
        const std::filesystem::directory_entry& entry = *it;
        std::string fullPath = entry.path().string();
        std::string key = toLower(fullPath);
        g_pathCache[key] = entry.path();
        entriesAdded++;

        ec.clear();
        it.increment(ec);
        if (ec) {
            ec.clear();
        }
    }

    return entriesAdded;
}

inline std::filesystem::path FindInPathCache(const std::string& targetPath) {
    std::string key = toLower(targetPath);
    auto it = g_pathCache.find(key);
    if (it != g_pathCache.end()) {
        return it->second;
    }
    return {};
}
