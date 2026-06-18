#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <utility>

#include <rex/kernel/init.h>
#include <rex/logging.h>
#include <rex/runtime.h>

namespace {

std::filesystem::path executable_folder(const char* argv0) {
    std::error_code ec;
    auto path = std::filesystem::absolute(argv0, ec);
    if (ec) {
        return std::filesystem::current_path();
    }
    return path.parent_path();
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
    const rex::X_STATUS status = runtime.Setup(std::move(config));
    if (XFAILED(status)) {
        REXLOG_ERROR("ReXGlue runtime setup failed: {:08X}", status);
        rex::ShutdownLogging();
        return 2;
    }

    REXLOG_INFO("ReXGlue runtime setup reached tool-mode pre-guest boundary");
    runtime.Shutdown();
    rex::ShutdownLogging();

    std::cout << "LibertyRecompRex pre-guest setup OK\n";
    return 0;
}
