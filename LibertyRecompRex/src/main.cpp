#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <rex/kernel/init.h>
#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/system/util/xex2_info.h>

#include "gta4_config.h"

namespace {

constexpr const char* kDefaultXexVirtualPath = "game:\\default.xex";

struct CommandLineOptions {
    std::filesystem::path game_root;
    bool audit_load_xex = false;
};

const std::uint8_t* checked_range(const std::vector<std::uint8_t>& data,
                                  const std::size_t offset,
                                  const std::size_t size) {
    if (offset > data.size() || size > data.size() - offset) {
        return nullptr;
    }
    return data.data() + offset;
}

const rex::xex2_opt_header* find_opt_header(const rex::xex2_header& header,
                                            const rex::xex2_header_keys key) {
    const auto header_count = static_cast<std::uint32_t>(header.header_count);
    for (std::uint32_t i = 0; i < header_count; ++i) {
        const auto& opt_header = header.headers[i];
        if (static_cast<std::uint32_t>(opt_header.key) == static_cast<std::uint32_t>(key)) {
            return &opt_header;
        }
    }
    return nullptr;
}

bool find_opt_header_value(const rex::xex2_header& header, const rex::xex2_header_keys key,
                           std::uint32_t* out_value) {
    const auto* opt_header = find_opt_header(header, key);
    if (opt_header == nullptr) {
        return false;
    }

    const auto storage_type = static_cast<std::uint32_t>(key) & 0xFF;
    if (storage_type == 0x00 || storage_type == 0x01) {
        *out_value = static_cast<std::uint32_t>(opt_header->value);
        return true;
    }
    return false;
}

template <typename T>
const T* find_opt_header_data(const std::vector<std::uint8_t>& data,
                              const rex::xex2_header& header,
                              const rex::xex2_header_keys key,
                              std::size_t* out_offset = nullptr) {
    const auto* opt_header = find_opt_header(header, key);
    if (opt_header == nullptr) {
        return nullptr;
    }

    const auto storage_type = static_cast<std::uint32_t>(key) & 0xFF;
    if (storage_type == 0x00 || storage_type == 0x01) {
        return nullptr;
    }

    const auto offset = static_cast<std::size_t>(static_cast<std::uint32_t>(opt_header->offset));
    const auto* range = checked_range(data, offset, sizeof(T));
    if (range == nullptr) {
        return nullptr;
    }

    if (out_offset != nullptr) {
        *out_offset = offset;
    }
    return reinterpret_cast<const T*>(range);
}

bool count_import_libraries(const std::vector<std::uint8_t>& data,
                            const std::size_t import_offset,
                            const rex::xex2_opt_import_libraries& imports,
                            std::uint32_t* out_library_count,
                            std::uint32_t* out_import_count) {
    *out_library_count = 0;
    *out_import_count = 0;

    const auto imports_size = static_cast<std::size_t>(static_cast<std::uint32_t>(imports.size));
    const auto string_table_size =
        static_cast<std::size_t>(static_cast<std::uint32_t>(imports.string_table.size));
    if (imports_size < 12 || import_offset > data.size() ||
        imports_size > data.size() - import_offset || string_table_size > imports_size - 12) {
        return false;
    }

    std::size_t library_offset = 12 + string_table_size;
    constexpr std::size_t kImportTableOffset =
        offsetof(rex::xex2_import_library, import_table);

    while (library_offset < imports_size) {
        const auto absolute_offset = import_offset + library_offset;
        const auto* library = reinterpret_cast<const rex::xex2_import_library*>(
            checked_range(data, absolute_offset, kImportTableOffset));
        if (library == nullptr) {
            return false;
        }

        const auto library_size =
            static_cast<std::size_t>(static_cast<std::uint32_t>(library->size));
        if (library_size == 0) {
            break;
        }
        if (library_size < kImportTableOffset || library_size > imports_size - library_offset) {
            return false;
        }

        const auto import_count = static_cast<std::uint32_t>(library->count);
        const auto required_size =
            kImportTableOffset + static_cast<std::size_t>(import_count) * sizeof(rex::be<std::uint32_t>);
        if (required_size > library_size) {
            return false;
        }

        ++(*out_library_count);
        *out_import_count += import_count;
        library_offset += library_size;
    }

    return true;
}

std::filesystem::path executable_folder(const char* argv0) {
    std::error_code ec;
    auto path = std::filesystem::absolute(argv0, ec);
    if (ec) {
        return std::filesystem::current_path();
    }
    return path.parent_path();
}

CommandLineOptions parse_command_line(int argc, char** argv,
                                      const std::filesystem::path& exe_dir) {
    CommandLineOptions options;
    options.game_root = exe_dir / "assets";

    bool game_root_set = false;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i] != nullptr ? argv[i] : "";
        if (arg == "--audit-load-xex") {
            options.audit_load_xex = true;
            continue;
        }

        if (!game_root_set) {
            options.game_root = std::filesystem::path(arg);
            game_root_set = true;
            continue;
        }
    }

    return options;
}

bool preflight_xex_metadata(const std::filesystem::path& host_path,
                            const std::uintmax_t expected_file_size) {
    if (expected_file_size > static_cast<std::uintmax_t>(std::numeric_limits<std::size_t>::max())) {
        REXLOG_ERROR("XEX metadata preflight: file is too large to inspect: {}",
                     host_path.string());
        return false;
    }

    std::ifstream file(host_path, std::ios::binary);
    if (!file) {
        REXLOG_ERROR("XEX metadata preflight: could not open {}", host_path.string());
        return false;
    }

    std::vector<std::uint8_t> data(static_cast<std::size_t>(expected_file_size));
    file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (file.gcount() != static_cast<std::streamsize>(data.size())) {
        REXLOG_ERROR("XEX metadata preflight: read {} of {} bytes from {}",
                     static_cast<long long>(file.gcount()),
                     static_cast<unsigned long long>(data.size()), host_path.string());
        return false;
    }

    const auto* header = reinterpret_cast<const rex::xex2_header*>(
        checked_range(data, 0, sizeof(rex::xex2_header)));
    if (header == nullptr || static_cast<std::uint32_t>(header->magic) != 0x58455832U) {
        REXLOG_ERROR("XEX metadata preflight: missing XEX2 header in {}", host_path.string());
        return false;
    }

    const auto header_size = static_cast<std::uint32_t>(header->header_size);
    const auto security_offset = static_cast<std::uint32_t>(header->security_offset);
    const auto header_count = static_cast<std::uint32_t>(header->header_count);
    if (header_count == 0) {
        REXLOG_ERROR("XEX metadata preflight: header has no optional headers");
        return false;
    }
    const auto minimum_opt_header_end =
        offsetof(rex::xex2_header, headers) +
        static_cast<std::size_t>(header_count) * sizeof(rex::xex2_opt_header);
    if (header_size > data.size() || minimum_opt_header_end > header_size) {
        REXLOG_ERROR("XEX metadata preflight: invalid header bounds headerSize=0x{:X} count={}",
                     header_size, header_count);
        return false;
    }

    const auto* security = reinterpret_cast<const rex::xex2_security_info*>(
        checked_range(data, security_offset, sizeof(rex::xex2_security_info)));
    if (security == nullptr) {
        REXLOG_ERROR("XEX metadata preflight: invalid security offset 0x{:X}",
                     security_offset);
        return false;
    }

    const auto image_size = static_cast<std::uint32_t>(security->image_size);
    const auto load_address = static_cast<std::uint32_t>(security->load_address);
    const auto page_count = static_cast<std::uint32_t>(security->page_descriptor_count);
    const auto security_size =
        offsetof(rex::xex2_security_info, page_descriptors) +
        static_cast<std::size_t>(page_count) * sizeof(rex::xex2_page_descriptor);
    if (checked_range(data, security_offset, security_size) == nullptr) {
        REXLOG_ERROR("XEX metadata preflight: invalid page descriptor bounds pages={}",
                     page_count);
        return false;
    }

    std::uint32_t image_base = 0;
    std::uint32_t entry_point = 0;
    if (!find_opt_header_value(*header, rex::XEX_HEADER_IMAGE_BASE_ADDRESS, &image_base) ||
        !find_opt_header_value(*header, rex::XEX_HEADER_ENTRY_POINT, &entry_point)) {
        REXLOG_ERROR("XEX metadata preflight: missing image base or entry point header");
        return false;
    }

    std::size_t file_format_offset = 0;
    const auto* file_format = find_opt_header_data<rex::xex2_opt_file_format_info>(
        data, *header, rex::XEX_HEADER_FILE_FORMAT_INFO, &file_format_offset);
    if (file_format == nullptr) {
        REXLOG_ERROR("XEX metadata preflight: missing file format header");
        return false;
    }
    const auto file_format_size = static_cast<std::uint32_t>(file_format->info_size);
    if (checked_range(data, file_format_offset, file_format_size) == nullptr) {
        REXLOG_ERROR("XEX metadata preflight: invalid file format bounds size=0x{:X}",
                     file_format_size);
        return false;
    }

    std::uint32_t resource_count = 0;
    std::size_t resource_offset = 0;
    const auto* resources = find_opt_header_data<rex::xex2_opt_resource_info>(
        data, *header, rex::XEX_HEADER_RESOURCE_INFO, &resource_offset);
    if (resources != nullptr) {
        const auto resource_size = static_cast<std::uint32_t>(resources->size);
        if (resource_size >= 4 && checked_range(data, resource_offset, resource_size) != nullptr) {
            resource_count =
                (resource_size - 4U) / static_cast<std::uint32_t>(sizeof(rex::xex2_resource));
        }
    }

    std::uint32_t import_library_count = 0;
    std::uint32_t import_count = 0;
    std::size_t imports_offset = 0;
    const auto* imports = find_opt_header_data<rex::xex2_opt_import_libraries>(
        data, *header, rex::XEX_HEADER_IMPORT_LIBRARIES, &imports_offset);
    if (imports != nullptr &&
        !count_import_libraries(data, imports_offset, *imports, &import_library_count,
                                &import_count)) {
        REXLOG_ERROR("XEX metadata preflight: invalid import library bounds");
        return false;
    }

    const bool image_matches_config =
        image_base == PPCImageConfig.image_base && image_size == PPCImageConfig.image_size;
    REXLOG_INFO(
        "XEX metadata preflight: moduleFlags=0x{:08X} headerSize=0x{:X} security=0x{:X} "
        "optHeaders={} imageSize=0x{:X} load=0x{:08X} imageBase=0x{:08X} entry=0x{:08X} "
        "fileFormat enc={} comp={} resources={} importLibs={} imports={} pages={} "
        "matchesPPCImageConfig={}",
        static_cast<std::uint32_t>(header->module_flags), header_size, security_offset,
        header_count, image_size, load_address, image_base, entry_point,
        static_cast<std::uint32_t>(file_format->encryption_type),
        static_cast<std::uint32_t>(file_format->compression_type), resource_count,
        import_library_count, import_count, page_count, image_matches_config ? "yes" : "no");

    if (!image_matches_config) {
        REXLOG_ERROR("XEX metadata preflight: image metadata does not match PPCImageConfig");
        return false;
    }

    return true;
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
    return preflight_xex_metadata(host_path, file_size);
}

}  // namespace

int main(int argc, char** argv) {
    const auto exe_dir = executable_folder(argc > 0 ? argv[0] : "LibertyRecompRex.exe");
    const auto options = parse_command_line(argc, argv, exe_dir);
    const auto game_root = options.game_root;
    const auto log_path = exe_dir / "LibertyRecompRex.log";
    const auto game_root_string = game_root.string();
    const auto log_path_string = log_path.string();

    auto log_config = rex::BuildLogConfig(log_path_string.c_str(), "debug", {});
    rex::InitLogging(log_config);

    REXLOG_INFO("LibertyRecompRex sidecar starting");
    REXLOG_INFO("  Game root: {}", game_root_string);
    REXLOG_INFO("  Log path:  {}", log_path_string);
    REXLOG_INFO("  Audit LoadXexImage: {}", options.audit_load_xex ? "yes" : "no");

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

    if (options.audit_load_xex) {
        REXLOG_INFO("XEX load audit: calling LoadXexImage({}) without LaunchModule",
                    kDefaultXexVirtualPath);
        const rex::X_STATUS load_status = runtime.LoadXexImage(kDefaultXexVirtualPath);
        if (XFAILED(load_status)) {
            REXLOG_ERROR("XEX load audit: LoadXexImage failed with status {:08X}",
                         load_status);
            runtime.Shutdown();
            rex::ShutdownLogging();
            return 4;
        }
        REXLOG_INFO("XEX load audit: LoadXexImage returned {:08X}; LaunchModule skipped",
                    load_status);
    } else {
        REXLOG_INFO("XEX load audit: skipped; pass --audit-load-xex to cross module-load boundary");
    }

    REXLOG_INFO("ReXGlue runtime setup reached tool-mode pre-guest boundary");
    runtime.Shutdown();
    rex::ShutdownLogging();

    std::cout << "LibertyRecompRex pre-guest setup OK\n";
    return 0;
}
