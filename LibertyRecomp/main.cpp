#include <cstdio>
#include <stdafx.h>
#ifdef __x86_64__
#include <cpuid.h>
#endif
#include <cpu/guest_thread.h>
#include <gpu/video.h>
#include <kernel/function.h>
#include <kernel/memory.h>
#include <kernel/heap.h>
#include <kernel/xam.h>
#include <kernel/save_system.h>
#include <kernel/io/file_system.h>
#include <kernel/vfs.h>
#include <file.h>
#include <vector>
#include <image.h>
#include <apu/audio.h>
#include <hid/hid.h>
#include <user/config.h>
#include <user/paths.h>
#include <user/registry.h>
#include <kernel/xdbf.h>
#include <install/installer.h>
#include <install/update_checker.h>
#include <os/logger.h>
#include <os/process.h>
#include <os/registry.h>
#include <ui/game_window.h>
#include <ui/installer_wizard.h>
#include <mod/mod_loader.h>
#include <preload_executable.h>
#include <iostream>
#include <app.h>

#ifdef _WIN32
#include <timeapi.h>
#endif

#ifdef __SWITCH__
#include <sys/stat.h>

extern "C" {
uint32_t fsdevMountSdmc(void);
int fsdevUnmountDevice(const char* name);
void* fsdevGetDeviceFileSystem(const char* name);
uint32_t romfsMountSelf(const char* name);
uint32_t romfsUnmount(const char* name);
uint32_t svcOutputDebugString(const char* str, uint64_t size);
void svcSleepThread(int64_t ns);
bool envIsNso(void);
void LibertySwitchShowAuditDiagnostic(const char* title, const char* status, const char* detailPath, const char* logPath);
}

static constexpr const char* SWITCH_AUDIT_LOG_PATH = "sdmc:/switch/LibertyRecomp/LibertyRecomp.log";
static constexpr const char* SWITCH_AUDIT_CONTENT_ROOT = "sdmc:/switch/LibertyRecomp";
static constexpr const char* SWITCH_AUDIT_GAME_ROOT = "sdmc:/switch/LibertyRecomp/game";
static constexpr const char* SWITCH_AUDIT_MODULE_PATH = "sdmc:/switch/LibertyRecomp/game/default.xex";

static void SwitchAuditDebugRaw(const char* message)
{
    svcOutputDebugString(message, strlen(message));
}

__attribute__((constructor(101)))
static void SwitchAuditPremainProbe()
{
    SwitchAuditDebugRaw("[Switch] premain constructor 101 entered.\n");
}

static bool SwitchAuditSdmcAvailable()
{
    if (fsdevGetDeviceFileSystem("sdmc") != nullptr)
        return true;

    struct stat sdmcStat;
    return stat("sdmc:/", &sdmcStat) == 0;
}

static bool SwitchAuditMountSdmc(uint32_t& mountResult, bool& mountedHere)
{
    mountedHere = false;
    if (SwitchAuditSdmcAvailable())
    {
        mountResult = 0;
        return true;
    }

    mountResult = fsdevMountSdmc();
    if (mountResult == 0)
    {
        mountedHere = true;
        return true;
    }

    return SwitchAuditSdmcAvailable();
}

static void SwitchAuditLog(const char* message, const std::filesystem::path& path = {})
{
    mkdir("sdmc:/switch", 0777);
    mkdir("sdmc:/switch/LibertyRecomp", 0777);
    mkdir("sdmc:/switch/LibertyRecomp/game", 0777);

    std::string line = std::string("[Switch] ") + message;
    if (!path.empty())
        line += " " + path.string();
    line += "\n";

    FILE* log = fopen(SWITCH_AUDIT_LOG_PATH, "a");
    if (log != nullptr)
    {
        fputs(line.c_str(), log);
        fclose(log);
    }

    svcOutputDebugString(line.c_str(), static_cast<uint64_t>(line.size()));
    fputs(line.c_str(), stderr);
}

static void SwitchAuditUnmount(bool romfsMounted, bool sdmcMounted)
{
    if (romfsMounted)
        romfsUnmount("romfs");

    if (sdmcMounted)
        fsdevUnmountDevice("sdmc");
}

static bool SwitchAuditFileExists(const char* path)
{
    FILE* file = fopen(path, "rb");
    if (file == nullptr)
        return false;

    fclose(file);
    return true;
}
#endif

#if defined(_WIN32) && defined(LIBERTY_RECOMP_D3D12)
static std::array<std::string_view, 3> g_D3D12RequiredModules =
{
    "D3D12/D3D12Core.dll",
    "dxcompiler.dll",
    "dxil.dll"
};
#endif

const size_t XMAIOBegin = 0x7FEA0000;
const size_t XMAIOEnd = XMAIOBegin + 0x0000FFFF;

Memory g_memory;
Heap g_userHeap;
XDBFWrapper g_xdbfWrapper;
std::unordered_map<uint16_t, GuestTexture*> g_xdbfTextureCache;

void HostStartup()
{
#ifdef _WIN32
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif

    hid::Init();
}

// Forward declarations from imports.cpp
void InitKernelMainThread();

// Xenon memory initialization
#include <kernel/xenon_memory.h>

// Name inspired from nt's entry point
void KiSystemStartup()
{
    // Initialize main thread ID for SDL event safety
    InitKernelMainThread();
    
    if (g_memory.base == nullptr)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, GameWindow::GetTitle(), Localise("System_MemoryAllocationFailed").c_str(), GameWindow::s_pWindow);
        std::_Exit(1);
    }

    g_userHeap.Init();
    
    // CRITICAL: Initialize Xbox 360 Xenon memory regions per memory contract
    // This zeros all regions the game assumes are pre-allocated and zeroed on boot.
    // Must happen BEFORE any game code executes to prevent corruption.
    InitializeXenonMemoryRegions(g_memory.base);

#if defined(__SWITCH__) && defined(LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_STOP_AFTER_XENON_INIT)
    SwitchAuditLog("KiSystemStartup heap and Xenon fixed memory regions initialized; returning before save/content/audio startup.");
    return;
#endif

    // Initialize save system early - creates directories and registers save content
    SaveSystem::Initialize();

    const auto gameContent = XamMakeContent(XCONTENTTYPE_RESERVED, "Game");
    const std::string gamePath = (const char*)(GetGamePath() / "game").u8string().c_str();

    BuildPathCache(gamePath);

    // Also cache extracted assets living under "RPF DUMP" (nested RPFS, audio packs, etc.).
    // This improves hit rate when the title requests files that are not present in game/.
    const std::string rpfDumpPath = (const char*)(GetGamePath() / "RPF DUMP").u8string().c_str();
    if (std::filesystem::exists(rpfDumpPath))
        BuildPathCache(rpfDumpPath);

    XamRegisterContent(gameContent, gamePath);

    // Register and mount update content (CRITICAL for file override)
    const auto updateContent = XamMakeContent(XCONTENTTYPE_RESERVED, "Update");
    const std::filesystem::path updateRoot = GetGamePath() / "update";
    const std::string updatePath = (const char*)updateRoot.u8string().c_str();

    if (std::filesystem::exists(updateRoot))
    {
        XamRegisterContent(updateContent, updatePath);
        XamContentCreateEx(0, "update", &updateContent, OPEN_EXISTING, nullptr, nullptr, 0, 0, nullptr);
        
        // Create root mappings for update paths (case variations)
        XamRootCreate("update", updatePath);
        XamRootCreate("Update", updatePath);
        
        LOGF_IMPL(Utility, "Main", "Registered update: -> {}", updatePath);
    }
    else
    {
        LOGF_IMPL(Utility, "Main", "No update directory found (this is normal if no Title Update installed)");
    }

    // Note: Save system initialization already handled by SaveSystem::Initialize() above
    // This section is kept for backwards compatibility with old save file format
    const auto saveFilePath = GetSaveFilePath(true);
    bool saveFileExists = std::filesystem::exists(saveFilePath);

    if (!saveFileExists)
    {
        // Copy base save data to modded save as fallback.
        std::error_code ec;
        std::filesystem::create_directories(saveFilePath.parent_path(), ec);

        if (!ec)
        {
            std::filesystem::copy_file(GetSaveFilePath(false), saveFilePath, ec);
            saveFileExists = !ec;
        }
    }

    if (saveFileExists)
    {
        std::u8string savePathU8 = saveFilePath.parent_path().u8string();
        XamRegisterContent(XamMakeContent(XCONTENTTYPE_SAVEDATA, "GTA4SaveData.bin"), (const char*)(savePathU8.c_str()));
    }

    // Mount game
    XamContentCreateEx(0, "game", &gameContent, OPEN_EXISTING, nullptr, nullptr, 0, 0, nullptr);

    // OS mounts game data to D:
    XamContentCreateEx(0, "D", &gameContent, OPEN_EXISTING, nullptr, nullptr, 0, 0, nullptr);

    // GTA IV uses "common:" and "platform:" root paths
    // All game files are in: GetGamePath() / "game" / (common/, xbox360/, audio/)
    // Structure: ~/Library/Application Support/LibertyRecomp/game/common/, etc.
    const auto gameRoot = GetGamePath() / "game";
    const std::string commonPath = (const char*)(gameRoot / "common").u8string().c_str();
    const std::string platformPath = (const char*)(gameRoot / "xbox360").u8string().c_str();
    const std::string audioPath = (const char*)(gameRoot / "audio").u8string().c_str();
    
    // Register main root paths
    XamRootCreate("common", commonPath);
    XamRootCreate("platform", platformPath);
    XamRootCreate("audio", audioPath);
    
    // Also register alternate names the game might use
    XamRootCreate("xbox360", platformPath);  // Some code uses xbox360: instead of platform:
    
    LOGF_IMPL(Utility, "Main", "Game root: {}", gameRoot.string());
    LOGF_IMPL(Utility, "Main", "Registered common: -> {}", commonPath);
    LOGF_IMPL(Utility, "Main", "Registered platform: -> {}", platformPath);
    LOGF_IMPL(Utility, "Main", "Registered audio: -> {}", audioPath);

    // Initialize Virtual File System for direct file serving
    // This bypasses complex RPF offset-based reading that causes stream issues
    VFS::Initialize(gameRoot);
    LOGF_IMPL(Utility, "Main", "VFS initialized with root: {}", gameRoot.string());

    std::error_code ec;
    for (auto& file : std::filesystem::directory_iterator(GetGamePath() / "dlc", ec))
    {
        if (file.is_directory())
        {
            std::u8string fileNameU8 = file.path().filename().u8string();
            std::u8string filePathU8 = file.path().u8string();
            const char* fileName = (const char*)(fileNameU8.c_str());
            const char* filePath = (const char*)(filePathU8.c_str());
            
            // Register DLC content
            XamRegisterContent(XamMakeContent(XCONTENTTYPE_DLC, fileName), filePath);
            
            // Mount DLC to virtual path
            auto dlcContent = XamMakeContent(XCONTENTTYPE_DLC, fileName);
            XamContentCreateEx(0, fileName, &dlcContent, OPEN_EXISTING, nullptr, nullptr, 0, 0, nullptr);
            
            // Create root mapping for DLC-specific paths
            XamRootCreate(fileName, filePath);
            
            LOGF_IMPL(Utility, "Main", "Registered DLC: {} -> {}", fileName, filePath);
        }
    }

    XAudioInitializeSystem();
}

uint32_t LdrLoadModule(const std::filesystem::path &path)
{
    const auto loadResult = LoadFile(path);
    if (loadResult.empty())
    {
        assert("Failed to load module" && false);
        return 0;
    }

    const auto image = Image::ParseImage(loadResult.data(), loadResult.size());

    memcpy(g_memory.Translate(image.base), image.data.get(), image.size);
    g_xdbfWrapper = XDBFWrapper(static_cast<uint8_t*>(g_memory.Translate(image.resource_offset)), image.resource_size);

    // GTA IV Memory Layout Collision Fix
    // Address 0x82003890 aliases with "common.rpf" string in image .rdata section.
    // The game expects this to be heap-backed stream object storage.
    // Xbox 360 kept these regions separate, but native recompilation does not.
    // Fix: Zero the collision range and fully initialize stream struct.
    {
        uint8_t* collisionBase = static_cast<uint8_t*>(g_memory.Translate(0x82003880));
        memset(collisionBase, 0, 0x80);  // Zero 0x82003880 - 0x82003900
        
        // Fully initialize stream struct at 0x82003890
        // Stream struct layout (28 bytes / 7 dwords):
        //   [0] = Object pointer (vtable holder)
        //   [1] = Context/parameter
        //   [2] = Buffer pointer
        //   [3] = File position
        //   [4] = Buffer cursor
        //   [5] = Buffer end/limit
        //   [6] = Buffer capacity
        be<uint32_t>* streamPtr = reinterpret_cast<be<uint32_t>*>(g_memory.Translate(0x82003890));
        streamPtr[0] = 0;  // Null object - stream ops will return early gracefully
        streamPtr[1] = 0;           // Context (null is safe)
        streamPtr[2] = 0;           // Buffer (null = no buffer)
        streamPtr[3] = 0;           // File position
        streamPtr[4] = 0;           // Buffer cursor
        streamPtr[5] = 0;           // Buffer end (0 = empty)
        streamPtr[6] = 0;           // Capacity (0 = no buffer)
    }
    
    // Worker Thread Global Memory Initialization
    // Address range 0x830F5000-0x830F8000 contains worker-related global structures.
    // Workers read semaphore handles from this region (e.g., 0x830F7684 = base+9860).
    // Without zeroing, workers read stale data from previous runs causing infinite polling.
    // Fix: Zero worker globals so uninitialized handles read as NULL.
    {
        uint8_t* workerGlobals = static_cast<uint8_t*>(g_memory.Translate(0x830F5000));
        if (workerGlobals) {
            memset(workerGlobals, 0, 0x3000);  // Zero 12KB of worker globals
            printf("[LdrLoadModule] Zeroed worker globals 0x830F5000-0x830F8000\n");
        }
    }

    return image.entry_point;
}

#ifdef __x86_64__
__attribute__((constructor(101), target("no-avx,no-avx2"), noinline))
void init()
{
    uint32_t eax, ebx, ecx, edx;

    // Execute CPUID for processor info and feature bits.
    __get_cpuid(1, &eax, &ebx, &ecx, &edx);

    // Check for AVX support.
    if ((ecx & (1 << 28)) == 0)
    {
        printf("[*] CPU does not support the AVX instruction set.\n");

#ifdef _WIN32
        MessageBoxA(nullptr, "Your CPU does not meet the minimum system requirements.", "Liberty Recompiled", MB_ICONERROR);
#endif

        std::_Exit(1);
    }
}
#endif

int main(int argc, char *argv[])
{
#ifdef _WIN32
    timeBeginPeriod(1);
#endif

#ifdef __SWITCH__
    SwitchAuditDebugRaw("[Switch] main entered.\n");
    bool switchRomfsMounted = false;
    bool switchSdmcMounted = false;

    const bool switchRunningAsNso = envIsNso();
    uint32_t romfsResult = 0;
    if (switchRunningAsNso)
    {
        SwitchAuditDebugRaw("[Switch] envIsNso reported true; skipping romfsMountSelf for SD-only audit package.\n");
    }
    else
    {
        SwitchAuditDebugRaw("[Switch] mounting romfs.\n");
        romfsResult = romfsMountSelf("romfs");
        if (romfsResult != 0)
            fprintf(stderr, "[Switch] romfsInit failed: 0x%08X\n", romfsResult);
        else
            switchRomfsMounted = true;
    }
    {
        char status[128];
        snprintf(status, sizeof(status), "[Switch] romfsMountSelf status 0x%08X (nso=%u).\n", romfsResult, switchRunningAsNso ? 1u : 0u);
        SwitchAuditDebugRaw(status);
    }

    SwitchAuditDebugRaw("[Switch] ensuring sdmc.\n");
    uint32_t sdmcResult = 0;
    const bool switchSdmcUsable = SwitchAuditMountSdmc(sdmcResult, switchSdmcMounted);
    if (!switchSdmcUsable)
    {
        fprintf(stderr, "[Switch] fsdevMountSdmc failed and sdmc is unavailable: 0x%08X\n", sdmcResult);
        char status[128];
        snprintf(status, sizeof(status), "fsdevMountSdmc failed and sdmc is unavailable: 0x%08X", sdmcResult);
        SwitchAuditDebugRaw(status);
        SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
        for (;;)
            svcSleepThread(1000000000);
    }

    if (sdmcResult != 0)
    {
        char status[128];
        snprintf(status, sizeof(status), "fsdevMountSdmc returned 0x%08X; existing sdmc device is usable", sdmcResult);
        SwitchAuditDebugRaw(status);
    }

    SwitchAuditLog("Switch audit package startup; continuing to content preflight.");

#if defined(LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_STOP)
    if (g_memory.base != nullptr)
    {
        SwitchAuditLog("Switch guest memory audit mapped successfully; stopping before content preflight and host startup.");
        LibertySwitchShowAuditDiagnostic(
            "Guest memory audit",
            "Guest memory mapped successfully.\n"
            "Content preflight, host startup, and guest code were skipped.",
            SWITCH_AUDIT_CONTENT_ROOT,
            SWITCH_AUDIT_LOG_PATH);
    }
    else
    {
        SwitchAuditLog("Switch guest memory audit failed; Memory::base is null; stopping before content preflight and host startup.");
        LibertySwitchShowAuditDiagnostic(
            "Guest memory audit failed",
            "Memory::base is null after startup.\n"
            "Content preflight, host startup, and guest code were skipped.",
            SWITCH_AUDIT_CONTENT_ROOT,
            SWITCH_AUDIT_LOG_PATH);
    }
    SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
    return 0;
#endif

#if defined(LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_STOP_AFTER_XENON_INIT)
    if (g_memory.base == nullptr)
    {
        SwitchAuditLog("Switch startup-memory audit failed; Memory::base is null; stopping before host startup.");
        LibertySwitchShowAuditDiagnostic(
            "Startup memory audit failed",
            "Memory::base is null after startup.\n"
            "Host startup and guest code were skipped.",
            SWITCH_AUDIT_CONTENT_ROOT,
            SWITCH_AUDIT_LOG_PATH);
        SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
        return 0;
    }

    SwitchAuditLog("Switch startup-memory audit mapped guest memory; running KiSystemStartup heap/Xenon initialization only.");
    KiSystemStartup();
    SwitchAuditLog("KiSystemStartup heap/Xenon memory initialization completed; stopping before host config, content, module load, and guest code.");
    LibertySwitchShowAuditDiagnostic(
        "Startup memory audit",
        "KiSystemStartup heap/Xenon memory initialization completed.\n"
        "Host config, content checks, module loading, and guest code were skipped.",
        SWITCH_AUDIT_CONTENT_ROOT,
        SWITCH_AUDIT_LOG_PATH);
    SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
    return 0;
#else
    if (!SwitchAuditFileExists(SWITCH_AUDIT_MODULE_PATH))
    {
        SwitchAuditLog("Early preflight missing game executable:", SWITCH_AUDIT_MODULE_PATH);
        SwitchAuditLog("Expected SD layout root:", SWITCH_AUDIT_CONTENT_ROOT);
        SwitchAuditLog("Expected game content root:", SWITCH_AUDIT_GAME_ROOT);
        LibertySwitchShowAuditDiagnostic(
            "Missing game content",
            "Expected SD root: sdmc:/switch/LibertyRecomp\n"
            "Expected module: game/default.xex\n"
            "Host startup and guest code were skipped.",
            SWITCH_AUDIT_MODULE_PATH,
            SWITCH_AUDIT_LOG_PATH);
        SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
        return 0;
    }
#endif

    if (g_memory.base == nullptr)
    {
        SwitchAuditLog("default.xex was found, but Switch guest memory is disabled; stopping before host startup:", SWITCH_AUDIT_MODULE_PATH);
        LibertySwitchShowAuditDiagnostic(
            "Guest memory disabled",
            "default.xex is present, but this audit build has no guest memory backend.\n"
            "Host startup and guest code were skipped.",
            SWITCH_AUDIT_MODULE_PATH,
            SWITCH_AUDIT_LOG_PATH);
        SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
        return 0;
    }
#endif

    os::process::CheckConsole();

    if (!os::registry::Init())
        LOGN_WARNING("OS does not support registry.");

    os::logger::Init();

    PreloadContext preloadContext;
    preloadContext.PreloadExecutable();

    bool forceInstaller = false;
    bool forceDLCInstaller = false;
    bool useDefaultWorkingDirectory = false;
    bool forceInstallationCheck = false;
    bool graphicsApiRetry = false;
    const char *sdlVideoDriver = nullptr;

    for (uint32_t i = 1; i < argc; i++)
    {
        forceInstaller = forceInstaller || (strcmp(argv[i], "--install") == 0);
        forceDLCInstaller = forceDLCInstaller || (strcmp(argv[i], "--install-dlc") == 0);
        useDefaultWorkingDirectory = useDefaultWorkingDirectory || (strcmp(argv[i], "--use-cwd") == 0);
        forceInstallationCheck = forceInstallationCheck || (strcmp(argv[i], "--install-check") == 0);
        graphicsApiRetry = graphicsApiRetry || (strcmp(argv[i], "--graphics-api-retry") == 0);
        App::s_isSkipLogos = App::s_isSkipLogos || (strcmp(argv[i], "--skip-logos") == 0);

        if (strcmp(argv[i], "--sdl-video-driver") == 0)
        {
            if ((i + 1) < argc)
                sdlVideoDriver = argv[++i];
            else
                LOGN_WARNING("No argument was specified for --sdl-video-driver. Option will be ignored.");
        }
    }

    if (!useDefaultWorkingDirectory)
    {
        // Set the current working directory to the executable's path.
        std::error_code ec;
        std::filesystem::current_path(os::process::GetExecutablePath().parent_path(), ec);
    }

    Config::Load();

    if (forceInstallationCheck)
    {
        // Create the console to show progress to the user, otherwise it will seem as if the game didn't boot at all.
        os::process::ShowConsole();

        Journal journal;
        double lastProgressMiB = 0.0;
        double lastTotalMib = 0.0;
        Installer::checkInstallIntegrity(GAME_INSTALL_DIRECTORY, journal, [&]()
        {
            constexpr double MiBDivisor = 1024.0 * 1024.0;
            constexpr double MiBProgressThreshold = 128.0;
            double progressMiB = double(journal.progressCounter) / MiBDivisor;
            double totalMiB = double(journal.progressTotal) / MiBDivisor;
            if (journal.progressCounter > 0)
            {
                if ((progressMiB - lastProgressMiB) > MiBProgressThreshold)
                {
                    fprintf(stdout, "Checking files: %0.2f MiB / %0.2f MiB\n", progressMiB, totalMiB);
                    lastProgressMiB = progressMiB;
                }
            }
            else
            {
                if ((totalMiB - lastTotalMib) > MiBProgressThreshold)
                {
                    fprintf(stdout, "Scanning files: %0.2f MiB\n", totalMiB);
                    lastTotalMib = totalMiB;
                }
            }

            return true;
        });

        char resultText[512];
        uint32_t messageBoxStyle;
        if (journal.lastResult == Journal::Result::Success)
        {
            snprintf(resultText, sizeof(resultText), "%s", Localise("IntegrityCheck_Success").c_str());
            fprintf(stdout, "%s\n", resultText);
            messageBoxStyle = SDL_MESSAGEBOX_INFORMATION;
        }
        else
        {
            snprintf(resultText, sizeof(resultText), Localise("IntegrityCheck_Failed").c_str(), journal.lastErrorMessage.c_str());
            fprintf(stderr, "%s\n", resultText);
            messageBoxStyle = SDL_MESSAGEBOX_ERROR;
        }

        SDL_ShowSimpleMessageBox(messageBoxStyle, GameWindow::GetTitle(), resultText, GameWindow::s_pWindow);
        std::_Exit(int(journal.lastResult));
    }

#if defined(_WIN32) && defined(LIBERTY_RECOMP_D3D12)
    for (auto& dll : g_D3D12RequiredModules)
    {
        if (!std::filesystem::exists(g_executableRoot / dll))
        {
            char text[512];
            snprintf(text, sizeof(text), Localise("System_Win32_MissingDLLs").c_str(), dll.data());
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, GameWindow::GetTitle(), text, GameWindow::s_pWindow);
            std::_Exit(1);
        }
    }
#endif

    // Check the time since the last time an update was checked. Store the new time if the difference is more than six hours.
    constexpr double TimeBetweenUpdateChecksInSeconds = 6 * 60 * 60;
    time_t timeNow = std::time(nullptr);
    double timeDifferenceSeconds = difftime(timeNow, Config::LastChecked);
    if (timeDifferenceSeconds > TimeBetweenUpdateChecksInSeconds)
    {
        UpdateChecker::initialize();
        UpdateChecker::start();
        Config::LastChecked = timeNow;
        Config::Save();
    }

    if (Config::ShowConsole)
        os::process::ShowConsole();
    LOGN_WARNING("Host Startup");
    HostStartup();

    std::filesystem::path modulePath;
    bool isGameInstalled = Installer::checkGameInstall(GetGamePath(), modulePath);
    bool runInstallerWizard = forceInstaller || forceDLCInstaller || !isGameInstalled;

#ifdef __SWITCH__
#if !defined(LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_STOP_AFTER_XENON_INIT)
    if (!isGameInstalled)
    {
        SwitchAuditLog("Game install is missing; expected module:", modulePath);
        SwitchAuditLog("Expected SD layout root: sdmc:/switch/LibertyRecomp");
        SwitchAuditLog("Expected game content root: sdmc:/switch/LibertyRecomp/game");
        SwitchAuditLog("This NRO is an audit container only and will exit before guest startup.");
        const std::string modulePathText = modulePath.string();
        LibertySwitchShowAuditDiagnostic(
            "Missing game content",
            "default.xex was not found. Guest startup was skipped.",
            modulePathText.c_str(),
            SWITCH_AUDIT_LOG_PATH);
        SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
        return 0;
    }
#endif
#endif

    // TEMPORARY: Force installer UI to always show for preview
    // TODO: Remove this line after UI preview is done
    // runInstallerWizard = true;  // DISABLED - respect actual install check
    
     if (runInstallerWizard)
     {
         if (!Video::CreateHostDevice(sdlVideoDriver, graphicsApiRetry))
         {
             SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, GameWindow::GetTitle(), Localise("Video_BackendError").c_str(), GameWindow::s_pWindow);
             std::_Exit(1);
         }

         if (!InstallerWizard::Run(GetGamePath(), isGameInstalled && forceDLCInstaller))
         {
             std::_Exit(0);
         }
     }

    // ModLoader::Init();

    printf("[Main] Calling KiSystemStartup...\n"); fflush(stdout);
    KiSystemStartup();
    printf("[Main] KiSystemStartup done\n"); fflush(stdout);

    printf("[Main] Loading module: %s\n", modulePath.string().c_str()); fflush(stdout);
    uint32_t entry = LdrLoadModule(modulePath);
    printf("[Main] Module loaded, entry=0x%08X\n", entry); fflush(stdout);

#ifdef __SWITCH__
    if (entry == 0)
    {
        SwitchAuditLog("Failed to load guest module; stopping before GuestThread::Start:", modulePath);
        const std::string modulePathText = modulePath.string();
        LibertySwitchShowAuditDiagnostic(
            "Guest module load failed",
            "LdrLoadModule returned entry=0. Guest startup was skipped.",
            modulePathText.c_str(),
            SWITCH_AUDIT_LOG_PATH);
        SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
        return 1;
    }
#endif

    if (!runInstallerWizard)
    {
        printf("[Main] Creating video device...\n"); fflush(stdout);
        if (!Video::CreateHostDevice(sdlVideoDriver, graphicsApiRetry))
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, GameWindow::GetTitle(), Localise("Video_BackendError").c_str(), GameWindow::s_pWindow);
            std::_Exit(1);
        }
        printf("[Main] Video device created\n"); fflush(stdout);
    }
    LOGN_WARNING("Start Guest Thread");
    LOGN_WARNING(modulePath.string());
    // Video::StartPipelinePrecompilation();

    GuestThread::Start({ entry, 0, 0, 0 });

#ifdef __SWITCH__
    SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
#endif

    return 0;
}

GUEST_FUNCTION_STUB(__imp__vsprintf);
GUEST_FUNCTION_STUB(__imp___vsnprintf);
GUEST_FUNCTION_STUB(__imp__sprintf);
GUEST_FUNCTION_STUB(__imp___snprintf);
GUEST_FUNCTION_STUB(__imp___snwprintf);
GUEST_FUNCTION_STUB(__imp__vswprintf);
GUEST_FUNCTION_STUB(__imp___vscwprintf);
GUEST_FUNCTION_STUB(__imp__swprintf);
