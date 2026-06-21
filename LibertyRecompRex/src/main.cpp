#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <rex/exception_handler.h>
#include <rex/kernel/init.h>
#include <rex/logging.h>
#include <rex/memory/utils.h>
#include <rex/ppc/function.h>
#include <rex/runtime.h>
#include <rex/system/export_resolver.h>
#include <rex/system/kernel_state.h>
#include <rex/system/user_module.h>
#include <rex/system/util/xex2_info.h>
#include <rex/system/xmemory.h>
#include <rex/system/xobject.h>
#include <rex/system/xthread.h>

#include "gta4_config.h"

namespace liberty_rex {
bool InstallForcedCtorTargets(rex::Runtime& runtime);
}

namespace {

constexpr const char* kDefaultXexVirtualPath = "game:\\default.xex";
constexpr std::chrono::milliseconds kFirstLaunchObservationWindow{2000};
constexpr int kFirstLaunchTimeoutExitCode = 8;

struct CommandLineOptions {
    std::filesystem::path game_root;
    bool audit_load_xex = false;
    bool audit_launch_module = false;
};

struct ExportCoverageCheck {
    const char* module_name;
    std::uint16_t ordinal;
    const char* expected_name;
    const char* direct_symbol;
    const char* bridge_scope;
    const char* source_scope;
};

struct HostPcLocation {
    std::string module_path = "unknown";
    std::uintptr_t module_base = 0;
    std::uintptr_t rva = 0;
    bool resolved = false;
};

const char* yes_no(const bool value) {
    return value ? "yes" : "no";
}

const char* exception_code_name(const rex::arch::Exception::Code code) {
    switch (code) {
    case rex::arch::Exception::Code::kAccessViolation:
        return "access_violation";
    case rex::arch::Exception::Code::kIllegalInstruction:
        return "illegal_instruction";
    default:
        return "unknown";
    }
}

const char* access_operation_name(const rex::arch::Exception::AccessViolationOperation op) {
    switch (op) {
    case rex::arch::Exception::AccessViolationOperation::kRead:
        return "read";
    case rex::arch::Exception::AccessViolationOperation::kWrite:
        return "write";
    default:
        return "unknown";
    }
}

void flush_rex_loggers() {
    for (const auto& category : rex::GetAllCategories()) {
        if (category.logger) {
            category.logger->flush();
        }
    }
}

HostPcLocation locate_host_pc(const std::uint64_t pc) {
    HostPcLocation location;

#ifdef _WIN32
    HMODULE module = nullptr;
    const auto flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
    if (pc != 0 && GetModuleHandleExA(flags, reinterpret_cast<LPCSTR>(pc), &module) &&
        module != nullptr) {
        std::array<char, MAX_PATH> module_path{};
        const auto path_length =
            GetModuleFileNameA(module, module_path.data(), static_cast<DWORD>(module_path.size()));
        location.module_base = reinterpret_cast<std::uintptr_t>(module);
        location.rva = static_cast<std::uintptr_t>(pc) - location.module_base;
        location.resolved = true;
        if (path_length > 0) {
            location.module_path.assign(module_path.data(), path_length);
        }
    }
#else
    (void)pc;
#endif

    return location;
}

void log_native_stack_trace() {
#ifdef _WIN32
    constexpr USHORT kStackFrameCount = 24;
    std::array<void*, kStackFrameCount> frames{};
    const USHORT frame_count =
        CaptureStackBackTrace(0, kStackFrameCount, frames.data(), nullptr);

    for (USHORT i = 0; i < frame_count; ++i) {
        const auto pc = reinterpret_cast<std::uintptr_t>(frames[i]);
        const auto pc_location = locate_host_pc(pc);
        REXLOG_ERROR(
            "First-launch audit: native-stack frame={} pc=0x{:016X} host_module={} module_base=0x{:016X} pc_rva=0x{:08X} resolved={}",
            static_cast<unsigned int>(i), static_cast<std::uint64_t>(pc),
            pc_location.module_path, static_cast<std::uint64_t>(pc_location.module_base),
            static_cast<std::uint64_t>(pc_location.rva), yes_no(pc_location.resolved));
    }
#endif
}

void log_host_registers(const rex::arch::Exception* ex) {
#if REX_ARCH_AMD64
    const auto* ctx = ex != nullptr ? ex->thread_context() : nullptr;
    if (ctx == nullptr) {
        REXLOG_ERROR("First-launch audit: host-registers unavailable=yes");
        return;
    }

    REXLOG_ERROR(
        "First-launch audit: host-registers rip=0x{:016X} rax=0x{:016X} rcx=0x{:016X} rdx=0x{:016X} rbx=0x{:016X} rsp=0x{:016X} rbp=0x{:016X} rsi=0x{:016X} rdi=0x{:016X} r8=0x{:016X} r9=0x{:016X} r10=0x{:016X} r11=0x{:016X} r12=0x{:016X} r13=0x{:016X} r14=0x{:016X} r15=0x{:016X}",
        static_cast<std::uint64_t>(ctx->rip), static_cast<std::uint64_t>(ctx->rax),
        static_cast<std::uint64_t>(ctx->rcx), static_cast<std::uint64_t>(ctx->rdx),
        static_cast<std::uint64_t>(ctx->rbx), static_cast<std::uint64_t>(ctx->rsp),
        static_cast<std::uint64_t>(ctx->rbp), static_cast<std::uint64_t>(ctx->rsi),
        static_cast<std::uint64_t>(ctx->rdi), static_cast<std::uint64_t>(ctx->r8),
        static_cast<std::uint64_t>(ctx->r9), static_cast<std::uint64_t>(ctx->r10),
        static_cast<std::uint64_t>(ctx->r11), static_cast<std::uint64_t>(ctx->r12),
        static_cast<std::uint64_t>(ctx->r13), static_cast<std::uint64_t>(ctx->r14),
        static_cast<std::uint64_t>(ctx->r15));
#else
    (void)ex;
#endif
}

void log_current_ppc_context_registers() {
    const auto* ctx = rex::g_current_ppc_context;
    if (ctx == nullptr) {
        REXLOG_ERROR("First-launch audit: ppc-registers unavailable=yes");
        return;
    }

    REXLOG_ERROR(
        "First-launch audit: ppc-registers lr=0x{:08X} ctr=0x{:08X} r1=0x{:08X} r3=0x{:08X} r4=0x{:08X} r5=0x{:08X} r6=0x{:08X} r7=0x{:08X} r8=0x{:08X} r9=0x{:08X} r10=0x{:08X} r11=0x{:08X} r12=0x{:08X} r13=0x{:08X}",
        static_cast<std::uint32_t>(ctx->lr), static_cast<std::uint32_t>(ctx->ctr.u64),
        static_cast<std::uint32_t>(ctx->r1.u64), static_cast<std::uint32_t>(ctx->r3.u64),
        static_cast<std::uint32_t>(ctx->r4.u64), static_cast<std::uint32_t>(ctx->r5.u64),
        static_cast<std::uint32_t>(ctx->r6.u64), static_cast<std::uint32_t>(ctx->r7.u64),
        static_cast<std::uint32_t>(ctx->r8.u64), static_cast<std::uint32_t>(ctx->r9.u64),
        static_cast<std::uint32_t>(ctx->r10.u64), static_cast<std::uint32_t>(ctx->r11.u64),
        static_cast<std::uint32_t>(ctx->r12.u64), static_cast<std::uint32_t>(ctx->r13.u64));
}

void log_thread_reference_snapshot() {
    const auto* ctx = rex::g_current_ppc_context;
    auto* kernel_state = rex::system::kernel_state();
    auto* memory = kernel_state != nullptr ? kernel_state->memory() : nullptr;
    if (ctx == nullptr || kernel_state == nullptr || memory == nullptr) {
        REXLOG_ERROR("First-launch audit: thread-ref unavailable=yes");
        return;
    }

    const auto stack_pointer = static_cast<std::uint32_t>(ctx->r1.u64);
    const auto out_thread_ptr_slot = stack_pointer + 80;
    const auto handle_slot = stack_pointer + 84;
    const auto thread_id_slot = stack_pointer + 96;

    const auto out_thread_ptr = rex::memory::load_and_swap<std::uint32_t>(
        memory->TranslateVirtual(out_thread_ptr_slot));
    const auto handle = rex::memory::load_and_swap<std::uint32_t>(
        memory->TranslateVirtual(handle_slot));
    const auto thread_id = rex::memory::load_and_swap<std::uint32_t>(
        memory->TranslateVirtual(thread_id_slot));

    const auto thread = kernel_state->object_table()->LookupObject<rex::system::XThread>(handle);
    const bool object_found = !!thread;
    const auto object_guest = object_found ? thread->guest_object() : 0;
    const auto object_id = object_found ? thread->thread_id() : 0;

    REXLOG_ERROR(
        "First-launch audit: thread-ref stack=0x{:08X} out_thread_slot=0x{:08X} out_thread=0x{:08X} handle_slot=0x{:08X} handle=0x{:08X} thread_id_slot=0x{:08X} thread_id=0x{:08X} object_found={} object_guest=0x{:08X} object_thread_id=0x{:08X}",
        stack_pointer, out_thread_ptr_slot, out_thread_ptr, handle_slot, handle, thread_id_slot,
        thread_id, yes_no(object_found), object_guest, object_id);
}

const char* export_type_name(const rex::runtime::Export::Type type) {
    switch (type) {
    case rex::runtime::Export::Type::kFunction:
        return "function";
    case rex::runtime::Export::Type::kVariable:
        return "variable";
    default:
        return "unknown";
    }
}

class FirstLaunchFailureCapture {
public:
    explicit FirstLaunchFailureCapture(std::filesystem::path log_path)
        : log_path_(std::move(log_path)) {}

    FirstLaunchFailureCapture(const FirstLaunchFailureCapture&) = delete;
    FirstLaunchFailureCapture& operator=(const FirstLaunchFailureCapture&) = delete;

    ~FirstLaunchFailureCapture() {
        if (installed_) {
            rex::arch::ExceptionHandler::Uninstall(&FirstLaunchFailureCapture::HandleException,
                                                   this);
        }
    }

    void Install() {
        if (installed_) {
            return;
        }

        rex::arch::ExceptionHandler::Install(&FirstLaunchFailureCapture::HandleException, this);
        installed_ = true;

        REXLOG_INFO(
            "First-launch audit: failure capture observer installed structured_exception=rex_arch_exception_handler handler_order=after-mmio action=continue-search last_log=flush-on-exception log_path={}",
            log_path_.string());
        flush_rex_loggers();
    }

private:
    static bool HandleException(rex::arch::Exception* ex, void* data) {
        return static_cast<FirstLaunchFailureCapture*>(data)->OnException(ex);
    }

    bool OnException(rex::arch::Exception* ex) {
        const auto sequence = exception_count_.fetch_add(1, std::memory_order_relaxed) + 1;
        const auto code = ex != nullptr ? ex->code() : rex::arch::Exception::Code::kInvalidException;
        const auto pc = ex != nullptr ? ex->pc() : 0;
        const auto fault = ex != nullptr ? ex->fault_address() : 0;
        const auto access_op =
            ex != nullptr ? ex->access_violation_operation()
                          : rex::arch::Exception::AccessViolationOperation::kUnknown;
        const auto pc_location = locate_host_pc(pc);

        REXLOG_ERROR(
            "First-launch audit: structured exception observed sequence={} code={} pc=0x{:016X} host_module={} module_base=0x{:016X} pc_rva=0x{:08X} resolved={} fault=0x{:016X} access={} action=continue-search",
            sequence, exception_code_name(code), pc, pc_location.module_path,
            pc_location.module_base, pc_location.rva, yes_no(pc_location.resolved), fault,
            access_operation_name(access_op));
        log_host_registers(ex);
        log_current_ppc_context_registers();
        log_thread_reference_snapshot();
        log_native_stack_trace();
        flush_rex_loggers();
        return false;
    }

    std::filesystem::path log_path_;
    std::atomic_uint32_t exception_count_{0};
    bool installed_ = false;
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
        if (arg == "--audit-launch-module") {
            options.audit_launch_module = true;
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

bool log_loaded_xex_state(rex::Runtime& runtime) {
    auto* kernel_state = runtime.kernel_state();
    if (kernel_state == nullptr) {
        REXLOG_ERROR("XEX load audit: module-state missing kernel state");
        return false;
    }

    const auto module = kernel_state->GetExecutableModule();
    if (!module) {
        REXLOG_ERROR("XEX load audit: module-state missing executable module");
        return false;
    }

    const auto* xex_module = module->xex_module();
    const auto* security_info = xex_module->xex_security_info();
    const auto* import_libraries = xex_module->import_libraries();

    std::size_t import_count = 0;
    for (const auto& import_library : *import_libraries) {
        import_count += import_library.imports.size();
    }

    std::size_t executable_sections = 0;
    std::size_t writable_sections = 0;
    for (const auto& section : xex_module->pe_sections()) {
        if ((section.flags & kXEPESectionMemoryExecute) != 0) {
            ++executable_sections;
        }
        if ((section.flags & kXEPESectionMemoryWrite) != 0) {
            ++writable_sections;
        }
    }

    REXLOG_INFO(
        "XEX load audit: module-state name={} path={} title=0x{:08X} executable={} dll={} hmodule=0x{:08X} guest_xex_header=0x{:08X} entry=0x{:08X} stack=0x{:08X}",
        module->name(), module->path(), module->title_id(), module->is_executable() ? "yes" : "no",
        module->is_dll_module() ? "yes" : "no", module->hmodule_ptr(), module->guest_xex_header(),
        module->entry_point(), module->stack_size());
    REXLOG_INFO(
        "XEX load audit: module-state image_base=0x{:08X} image_size=0x{:08X} export=0x{:08X} pages={} sections={} executable_sections={} writable_sections={} import_libs={} imports={}",
        xex_module->base_address(), security_info->image_size, security_info->export_table,
        security_info->page_descriptor_count, xex_module->pe_sections().size(), executable_sections,
        writable_sections, import_libraries->size(), import_count);

    return true;
}

bool install_exthread_object_type_mapping(rex::Runtime& runtime) {
    auto* export_resolver = runtime.export_resolver();
    if (export_resolver == nullptr) {
        REXLOG_ERROR("XEX load audit: ExThreadObjectType mapping missing export resolver");
        return false;
    }

    auto* export_entry = export_resolver->GetExportByOrdinal("xboxkrnl.exe", 0x001B);
    if (export_entry == nullptr ||
        export_entry->type != rex::runtime::Export::Type::kVariable) {
        REXLOG_ERROR("XEX load audit: ExThreadObjectType mapping missing variable export row");
        return false;
    }

    if (export_entry->variable_ptr != 0) {
        REXLOG_INFO(
            "XEX load audit: sidecar variable mapping module=xboxkrnl.exe ordinal=0x001B expected=ExThreadObjectType variable=0x{:08X} source=existing",
            export_entry->variable_ptr);
        return true;
    }

    // ReXGlue's ObReferenceObjectByHandle_entry compares object-type imports
    // against the Xenia-style D###BEEF sentinel values, not an allocated guest
    // X_OBJECT_TYPE address. Ordinal 0x001B maps to D01BBEEF.
    constexpr std::uint32_t kExThreadObjectType = 0xD01BBEEF;
    export_resolver->SetVariableMapping("xboxkrnl.exe", 0x001B, kExThreadObjectType);
    REXLOG_INFO(
        "XEX load audit: sidecar variable mapping module=xboxkrnl.exe ordinal=0x001B expected=ExThreadObjectType variable=0x{:08X} source=xenia_dummy_type",
        kExThreadObjectType);
    return true;
}

bool log_first_launch_gate(rex::Runtime& runtime) {
    auto* kernel_state = runtime.kernel_state();
    if (kernel_state == nullptr) {
        REXLOG_ERROR("First-launch audit: requested=yes gate=enabled missing kernel state");
        return false;
    }

    const auto module = kernel_state->GetExecutableModule();
    if (!module) {
        REXLOG_ERROR("First-launch audit: requested=yes gate=enabled missing executable module");
        return false;
    }

    REXLOG_WARN("First-launch audit: requested=yes gate=enabled; Runtime::LaunchModule will be called");
    REXLOG_INFO(
        "First-launch audit: pre-launch module name={} entry=0x{:08X} stack=0x{:08X} hmodule=0x{:08X} title=0x{:08X}",
        module->name(), module->entry_point(), module->stack_size(), module->hmodule_ptr(),
        module->title_id());
    REXLOG_INFO(
        "First-launch audit: next proof target=host-thread-create-or-first-guest-pc capture=sidecar-log-and-process-exit");
    REXLOG_INFO(
        "First-launch audit: capture plan process_exit_code=caller-or-bounded-exit structured_exception=observer last_log_line=LibertyRecompRex.log guest_entry_pc=0x{:08X} host_thread_create=Runtime::LaunchModule first_import_call=bridge-or-generated-log",
        module->entry_point());
    return true;
}

void log_launch_thread_snapshot(std::string_view label, rex::system::XThread& thread) {
    const auto* params = thread.creation_params();
    auto* thread_state = thread.thread_state();
    auto* ctx = thread_state != nullptr ? thread_state->context() : nullptr;

    REXLOG_INFO(
        "First-launch audit: {} thread_id={} pcr=0x{:08X} start=0x{:08X} startup=0x{:08X} context=0x{:08X} stack_size=0x{:08X} flags=0x{:08X} running={} ctx_present={}",
        label, thread.thread_id(), thread.pcr_ptr(), params->start_address,
        params->xapi_thread_startup, params->start_context, params->stack_size,
        params->creation_flags, yes_no(thread.is_running()), yes_no(ctx != nullptr));

    if (ctx == nullptr) {
        return;
    }

    REXLOG_INFO(
        "First-launch audit: {} ppc_context lr=0x{:08X} ctr=0x{:08X} r1=0x{:08X} r3=0x{:08X} r13=0x{:08X}",
        label, static_cast<std::uint32_t>(ctx->lr), ctx->ctr.u32, ctx->r1.u32, ctx->r3.u32,
        ctx->r13.u32);
}

bool run_first_launch_attempt(rex::Runtime& runtime) {
    if (!log_first_launch_gate(runtime)) {
        return false;
    }

    REXLOG_WARN("First-launch audit: calling Runtime::LaunchModule");
    flush_rex_loggers();

    auto main_thread = runtime.LaunchModule();
    if (!main_thread) {
        REXLOG_ERROR("First-launch audit: Runtime::LaunchModule returned null");
        flush_rex_loggers();
        return false;
    }

    log_launch_thread_snapshot("launch-return", *main_thread);
    flush_rex_loggers();

    const auto observation_start = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(kFirstLaunchObservationWindow);
    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::steady_clock::now() - observation_start)
                                .count();
    log_launch_thread_snapshot("post-observation", *main_thread);

    if (main_thread->is_running()) {
        REXLOG_WARN(
            "First-launch audit: bounded observation elapsed_ms={} running=yes process_exit_code={} reason=guest-thread-left-running",
            elapsed_ms, kFirstLaunchTimeoutExitCode);
        flush_rex_loggers();
        std::_Exit(kFirstLaunchTimeoutExitCode);
    }

    REXLOG_INFO(
        "First-launch audit: bounded observation elapsed_ms={} running=no process_exit_code=caller",
        elapsed_ms);
    flush_rex_loggers();
    return true;
}

void log_export_coverage(rex::Runtime& runtime) {
    auto* export_resolver = runtime.export_resolver();
    if (export_resolver == nullptr) {
        REXLOG_ERROR("XEX load audit: export-coverage missing export resolver");
        return;
    }

    static constexpr std::array<ExportCoverageCheck, 9> kChecks{{
        {"xam.xex", 0x02DC, "XamShowMessageBoxUIEx", "__imp__XamShowMessageBoxUIEx",
         "sidecar_registered_stub", "ReXGlue xam/xam_ui.cpp"},
        {"xam.xex", 0x02D5, "XamShowGamerCardUIForXUID", "__imp__XamShowGamerCardUIForXUID",
         "sidecar_registered_stub", "ReXGlue xam/xam_ui.cpp"},
        {"xam.xex", 0x02C6, "XamShowPlayerReviewUI", "__imp__XamShowPlayerReviewUI",
         "sidecar_registered_stub", "ReXGlue xam/xam_ui.cpp"},
        {"xam.xex", 0x02CB, "XamShowDeviceSelectorUI", "__imp__XamShowDeviceSelectorUI",
         "sidecar_registered_stub", "ReXGlue xam/xam_ui.cpp"},
        {"xam.xex", 0x02D9, "XamShowDirtyDiscErrorUI", "__imp__XamShowDirtyDiscErrorUI",
         "sidecar_registered_stub", "ReXGlue xam/xam_ui.cpp"},
        {"xboxkrnl.exe", 0x0257, "XeKeysConsoleSignatureVerification",
         "__imp__XeKeysConsoleSignatureVerification", "sidecar_registered_stub",
         "ReXGlue xboxkrnl/xboxkrnl_crypt.cpp"},
        {"xboxkrnl.exe", 0x0192, "XeCryptSha", "__imp__XeCryptSha",
         "sidecar_registered_stub", "ReXGlue xboxkrnl/xboxkrnl_crypt.cpp"},
        {"xboxkrnl.exe", 0x0256, "XeKeysConsolePrivateKeySign",
         "__imp__XeKeysConsolePrivateKeySign", "sidecar_registered_stub",
         "ReXGlue xboxkrnl/xboxkrnl_crypt.cpp"},
        {"xboxkrnl.exe", 0x001B, "ExThreadObjectType", nullptr,
         "sidecar_variable_mapping", "ReXGlue xboxkrnl/xboxkrnl_module.cpp"},
    }};

    for (const auto& check : kChecks) {
        auto* export_entry = export_resolver->GetExportByOrdinal(check.module_name, check.ordinal);
        if (export_entry == nullptr) {
            REXLOG_WARN(
                "XEX load audit: export-coverage module={} ordinal=0x{:04X} expected={} resolver=missing bridge={} source={}",
                check.module_name, check.ordinal, check.expected_name, check.bridge_scope,
                check.source_scope);
            continue;
        }

        const bool name_matches = std::string_view(export_entry->name) == check.expected_name;
        const bool resolver_stub =
            (export_entry->tags & rex::runtime::ExportTag::kStub) == rex::runtime::ExportTag::kStub;
        const bool ppc_registered =
            check.direct_symbol != nullptr && rex::FindPPCFuncByName(check.direct_symbol) != nullptr;
        const std::uint32_t variable_ptr =
            export_entry->type == rex::runtime::Export::Type::kVariable ? export_entry->variable_ptr : 0;

        REXLOG_INFO(
            "XEX load audit: export-coverage module={} ordinal=0x{:04X} expected={} resolver_name={} name_match={} type={} resolver_implemented={} resolver_stub={} variable=0x{:08X} ppc_registered={} bridge={} source={}",
            check.module_name, check.ordinal, check.expected_name, export_entry->name,
            yes_no(name_matches), export_type_name(export_entry->type),
            yes_no(export_entry->is_implemented()), yes_no(resolver_stub), variable_ptr,
            yes_no(ppc_registered), check.bridge_scope, check.source_scope);
    }
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
    FirstLaunchFailureCapture first_launch_failure_capture(log_path);

    REXLOG_INFO("LibertyRecompRex sidecar starting");
    REXLOG_INFO("  Game root: {}", game_root_string);
    REXLOG_INFO("  Log path:  {}", log_path_string);
    REXLOG_INFO("  Audit LoadXexImage: {}", options.audit_load_xex ? "yes" : "no");
    REXLOG_INFO("  Audit LaunchModule: {}", options.audit_launch_module ? "requested-gated" : "no");

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

    if (!liberty_rex::InstallForcedCtorTargets(runtime)) {
        REXLOG_ERROR("Stopping before XEX preflight because forced ctor target registration failed");
        runtime.Shutdown();
        rex::ShutdownLogging();
        return 7;
    }

    if (!install_exthread_object_type_mapping(runtime)) {
        REXLOG_ERROR("Stopping before XEX preflight because ExThreadObjectType mapping failed");
        runtime.Shutdown();
        rex::ShutdownLogging();
        return 6;
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
        if (!log_loaded_xex_state(runtime)) {
            runtime.Shutdown();
            rex::ShutdownLogging();
            return 5;
        }
        log_export_coverage(runtime);
        if (options.audit_launch_module) {
            first_launch_failure_capture.Install();
            if (!run_first_launch_attempt(runtime)) {
                runtime.Shutdown();
                rex::ShutdownLogging();
                return 7;
            }
        } else {
            REXLOG_INFO("XEX load audit: LoadXexImage returned {:08X}; LaunchModule skipped",
                        load_status);
        }
    } else {
        REXLOG_INFO("XEX load audit: skipped; pass --audit-load-xex to cross module-load boundary");
    }

    REXLOG_INFO("ReXGlue runtime setup reached tool-mode pre-guest boundary");
    runtime.Shutdown();
    rex::ShutdownLogging();

    std::cout << "LibertyRecompRex pre-guest setup OK\n";
    return 0;
}
