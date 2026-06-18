#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <utility>

#include <rex/kernel/init.h>
#include <rex/logging.h>
#include <rex/runtime.h>

#include "gta4_config.h"

namespace {

constexpr const char* kDefaultXexVirtualPath = "game:\\default.xex";

std::filesystem::path executable_folder(const char* argv0) {
    std::error_code ec;
    auto path = std::filesystem::absolute(argv0, ec);
    if (ec) {
        return std::filesystem::current_path();
    }
    return path.parent_path();
}

bool preflight_default_xex(const std::filesystem::path& game_root, rex::Runtime& runtime) {
    const auto host_path = game_root / "default.xex";
    std::error_code ec;

    if (!std::filesystem::is_regular_file(host_path, ec)) {
        REXLOG_ERROR("XEX preflight: {} missing at host path {}", kDefaultXexVirtualPath,
                     host_path.string());
        return false;
    }

    const auto file_size = std::filesystem::file_size(host_path, ec);
    if (ec) {
        REXLOG_ERROR("XEX preflight: {} size query failed for {}: {}",
                     kDefaultXexVirtualPath, host_path.string(), ec.message());
        return false;
    }

    auto* vfs_entry = runtime.file_system()->ResolvePath(kDefaultXexVirtualPath);
    if (vfs_entry == nullptr) {
        REXLOG_ERROR("XEX preflight: {} did not resolve through ReXGlue VFS",
                     kDefaultXexVirtualPath);
        return false;
    }

    std::array<char, 4> magic{};
    std::ifstream file(host_path, std::ios::binary);
    file.read(magic.data(), static_cast<std::streamsize>(magic.size()));
    if (file.gcount() != static_cast<std::streamsize>(magic.size())) {
        REXLOG_ERROR("XEX preflight: {} could not read 4-byte magic from {}",
                     kDefaultXexVirtualPath, host_path.string());
        return false;
    }

    const bool is_xex1 = magic == std::array<char, 4>{'X', 'E', 'X', '1'};
    const bool is_xex2 = magic == std::array<char, 4>{'X', 'E', 'X', '2'};
    if (!is_xex1 && !is_xex2) {
        REXLOG_ERROR("XEX preflight: {} has unexpected magic {:02X} {:02X} {:02X} {:02X}",
                     kDefaultXexVirtualPath, static_cast<unsigned char>(magic[0]),
                     static_cast<unsigned char>(magic[1]),
                     static_cast<unsigned char>(magic[2]),
                     static_cast<unsigned char>(magic[3]));
        return false;
    }

    REXLOG_INFO("XEX preflight: {} host={} size={} vfs_size={} magic={}",
                kDefaultXexVirtualPath, host_path.string(),
                static_cast<unsigned long long>(file_size),
                static_cast<unsigned long long>(vfs_entry->size()), is_xex2 ? "XEX2" : "XEX1");
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    const auto exe_dir = executable_folder(argc > 0 ? argv[0] : "LibertyRecompRex.exe");
    const auto game_root =
        argc > 1 ? std::filesystem::path(argv[1]) : exe_dir / "assets";
    const auto log_path = exe_dir / "LibertyRecompRex.log";
    const auto game_root_string = game_root.string();
    const auto log_path_string = log_path.string();

    auto log_config = rex::BuildLogConfig(log_path_string.c_str(), "debug", {});
    rex::InitLogging(log_config);

    REXLOG_INFO("LibertyRecompRex sidecar starting");
    REXLOG_INFO("  Game root: {}", game_root_string);
    REXLOG_INFO("  Log path:  {}", log_path_string);

    rex::RuntimeConfig config;
    config.tool_mode = true;
    config.kernel_init = rex::kernel::InitializeKernel;

    rex::Runtime runtime(game_root);
    REXLOG_INFO("  PPC code:  0x{:08X}-0x{:08X}", PPCImageConfig.code_base,
                PPCImageConfig.code_base + PPCImageConfig.code_size);
    REXLOG_INFO("  PPC image: 0x{:08X}-0x{:08X}", PPCImageConfig.image_base,
                PPCImageConfig.image_base + PPCImageConfig.image_size);

    const rex::X_STATUS status =
        runtime.Setup(PPCImageConfig.code_base, PPCImageConfig.code_size,
                      PPCImageConfig.image_base, PPCImageConfig.image_size,
                      PPCImageConfig.func_mappings, std::move(config));
    if (XFAILED(status)) {
        REXLOG_ERROR("ReXGlue runtime setup failed: {:08X}", status);
        rex::ShutdownLogging();
        return 2;
    }

    if (!preflight_default_xex(game_root, runtime)) {
        REXLOG_ERROR("Stopping before LoadXexImage because XEX preflight failed");
        runtime.Shutdown();
        rex::ShutdownLogging();
        return 3;
    }

    REXLOG_INFO("ReXGlue runtime setup reached tool-mode pre-guest boundary");
    runtime.Shutdown();
    rex::ShutdownLogging();

    std::cout << "LibertyRecompRex pre-guest setup OK\n";
    return 0;
}
