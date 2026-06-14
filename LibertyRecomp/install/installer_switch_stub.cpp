#include "installer.h"
#include "embedded_assets.h"

#include <cstdio>

bool Installer::checkGameInstall(const std::filesystem::path &baseDirectory, std::filesystem::path &modulePath)
{
    modulePath = EmbeddedAssets::IsAvailable()
        ? EmbeddedAssets::GetGameRoot() / "default.xex"
        : baseDirectory / "game" / "default.xex";

    FILE* moduleFile = fopen(modulePath.string().c_str(), "rb");
    const bool exists = moduleFile != nullptr;
    if (moduleFile != nullptr)
        fclose(moduleFile);
    else
        fprintf(stderr, "[Switch] Missing game executable: %s\n", modulePath.string().c_str());

    return exists;
}

bool Installer::checkDLCInstall(const std::filesystem::path &, DLC)
{
    return true;
}

bool Installer::checkAllDLC(const std::filesystem::path &)
{
    return true;
}

bool Installer::checkInstallIntegrity(const std::filesystem::path &, Journal &, const std::function<bool()> &)
{
    return true;
}

bool Installer::computeTotalSize(std::span<const FilePair>, const uint64_t *, VirtualFileSystem &, Journal &, uint64_t &totalSize)
{
    totalSize = 0;
    return true;
}

bool Installer::checkFiles(std::span<const FilePair>, const uint64_t *, const std::filesystem::path &, Journal &, const std::function<bool()> &, bool)
{
    return true;
}

bool Installer::copyFiles(std::span<const FilePair>, const uint64_t *, VirtualFileSystem &, const std::filesystem::path &, const std::string &, bool, Journal &, const std::function<bool()> &)
{
    return false;
}

bool Installer::parseContent(const std::filesystem::path &, std::unique_ptr<VirtualFileSystem> &, Journal &)
{
    return false;
}

bool Installer::parseSources(const Input &, Journal &, Sources &)
{
    return false;
}

bool Installer::install(const Sources &, const std::filesystem::path &, bool, Journal &, std::chrono::seconds, const std::function<bool()> &)
{
    return false;
}

void Installer::rollback(Journal &)
{
}

bool Installer::parseGame(const std::filesystem::path &)
{
    return false;
}

bool Installer::parseUpdate(const std::filesystem::path &)
{
    return false;
}

TitleUpdate Installer::detectUpdateVersion(const std::filesystem::path &)
{
    return TitleUpdate::Unknown;
}

DLC Installer::parseDLC(const std::filesystem::path &)
{
    return DLC::Unknown;
}
