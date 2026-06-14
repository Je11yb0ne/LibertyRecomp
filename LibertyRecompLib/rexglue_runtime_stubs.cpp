#include <chrono>
#include <cstring>
#include <memory>
#include <mutex>

#if defined(__SWITCH__)
#include <strings.h>

#include <rex/chrono/clock.h>
#include <rex/logging/types.h>
#include <rex/ppc/context.h>
#include <rex/system/mmio_handler.h>
#include <rex/thread/mutex.h>
#endif

#include <rex/audio/audio_system.h>
#include <rex/audio/xma/decoder.h>
#include <rex/input/input_system.h>

#if defined(__SWITCH__)
// Switch audit stubs: these satisfy ReXGlue runtime symbols while the real
// Switch kernel/runtime layer is still being designed.
namespace rex::chrono {

uint64_t Clock::QueryGuestTickCount() {
  static const auto start = std::chrono::steady_clock::now();
  const auto elapsed = std::chrono::steady_clock::now() - start;
  const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
  return static_cast<uint64_t>(ns / 20);
}

}  // namespace rex::chrono

namespace rex::thread {

std::recursive_mutex& global_critical_region::mutex() {
  static std::recursive_mutex mutex;
  return mutex;
}

}  // namespace rex::thread

namespace rex {

thread_local PPCContext* g_current_ppc_context = nullptr;
thread_local uint8_t* g_memory_base = nullptr;

spdlog::logger* GetLoggerRaw(LogCategoryId) {
  return nullptr;
}

std::shared_ptr<spdlog::logger> GetLogger(LogCategoryId) {
  return {};
}

std::shared_ptr<spdlog::logger> GetLogger() {
  return {};
}

}  // namespace rex

namespace {

class SwitchAuditMMIOHandler final : public rex::runtime::MMIOHandler {
 public:
  SwitchAuditMMIOHandler()
      : MMIOHandler(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr) {}
};

}  // namespace

namespace rex::runtime {

MMIOHandler* MMIOHandler::global_handler_ = [] {
  static SwitchAuditMMIOHandler handler;
  return static_cast<MMIOHandler*>(&handler);
}();

std::unique_ptr<MMIOHandler> MMIOHandler::Install(uint8_t*, uint8_t*, uint8_t*,
                                                  HostToGuestVirtual, const void*,
                                                  AccessViolationCallback, void*) {
  return nullptr;
}

MMIOHandler::MMIOHandler(uint8_t* virtual_membase, uint8_t* physical_membase,
                         uint8_t* membase_end, HostToGuestVirtual host_to_guest_virtual,
                         const void* host_to_guest_virtual_context,
                         AccessViolationCallback access_violation_callback,
                         void* access_violation_callback_context)
    : virtual_membase_(virtual_membase),
      physical_membase_(physical_membase),
      memory_end_(membase_end),
      host_to_guest_virtual_(host_to_guest_virtual),
      host_to_guest_virtual_context_(host_to_guest_virtual_context),
      access_violation_callback_(access_violation_callback),
      access_violation_callback_context_(access_violation_callback_context) {}

MMIOHandler::~MMIOHandler() = default;

bool MMIOHandler::RegisterRange(uint32_t, uint32_t, uint32_t, void*, MMIOReadCallback,
                                MMIOWriteCallback) {
  return false;
}

MMIORange* MMIOHandler::LookupRange(uint32_t) {
  return nullptr;
}

bool MMIOHandler::CheckLoad(uint32_t, uint32_t* out_value) {
  if (out_value != nullptr) {
    *out_value = 0;
  }
  return false;
}

bool MMIOHandler::CheckStore(uint32_t, uint32_t) {
  return false;
}

}  // namespace rex::runtime

namespace {

constexpr uint32_t kSwitchAuditInvalidHandle = 0xFFFFFFFFu;

uint8_t* GuestPointer(uint8_t* base, uint32_t address) {
  return address != 0 ? base + address : nullptr;
}

char* GuestString(uint8_t* base, uint32_t address) {
  return reinterpret_cast<char*>(GuestPointer(base, address));
}

uint32_t GuestAddress(uint8_t* base, const void* pointer) {
  if (pointer == nullptr) {
    return 0;
  }
  return static_cast<uint32_t>(reinterpret_cast<const uint8_t*>(pointer) - base);
}

void ReturnU32(PPCContext& ctx, uint32_t value) {
  ctx.r3.u64 = value;
}

void StoreGuestU32(uint8_t* base, uint32_t address, uint32_t value) {
  if (auto* out = reinterpret_cast<uint32_t*>(GuestPointer(base, address))) {
    *out = value;
  }
}

void StoreGuestU64(uint8_t* base, uint32_t address, uint64_t value) {
  if (auto* out = reinterpret_cast<uint64_t*>(GuestPointer(base, address))) {
    *out = value;
  }
}

}  // namespace

extern "C" PPC_FUNC(rexcrt_strstr) {
  char* haystack = GuestString(base, ctx.r3.u32);
  char* needle = GuestString(base, ctx.r4.u32);
  ReturnU32(ctx, GuestAddress(base, haystack && needle ? std::strstr(haystack, needle) : nullptr));
}

extern "C" PPC_FUNC(rexcrt__stricmp) {
  const char* lhs = GuestString(base, ctx.r3.u32);
  const char* rhs = GuestString(base, ctx.r4.u32);
  if (lhs == nullptr || rhs == nullptr) {
    ReturnU32(ctx, lhs == rhs ? 0u : (lhs != nullptr ? 1u : static_cast<uint32_t>(-1)));
    return;
  }
  ReturnU32(ctx, static_cast<uint32_t>(strcasecmp(lhs, rhs)));
}

extern "C" PPC_FUNC(rexcrt_strchr) {
  char* str = GuestString(base, ctx.r3.u32);
  ReturnU32(ctx, GuestAddress(base, str ? std::strchr(str, static_cast<int>(ctx.r4.u32)) : nullptr));
}

extern "C" PPC_FUNC(rexcrt_strncpy) {
  char* dst = GuestString(base, ctx.r3.u32);
  const char* src = GuestString(base, ctx.r4.u32);
  if (dst != nullptr && src != nullptr) {
    std::strncpy(dst, src, ctx.r5.u32);
  }
  ReturnU32(ctx, ctx.r3.u32);
}

extern "C" PPC_FUNC(rexcrt_strncmp) {
  const char* lhs = GuestString(base, ctx.r3.u32);
  const char* rhs = GuestString(base, ctx.r4.u32);
  if (lhs == nullptr || rhs == nullptr) {
    ReturnU32(ctx, lhs == rhs ? 0u : (lhs != nullptr ? 1u : static_cast<uint32_t>(-1)));
    return;
  }
  ReturnU32(ctx, static_cast<uint32_t>(std::strncmp(lhs, rhs, ctx.r5.u32)));
}

extern "C" PPC_FUNC(rexcrt_strtok) {
  char* str = ctx.r3.u32 != 0 ? GuestString(base, ctx.r3.u32) : nullptr;
  const char* delimiters = GuestString(base, ctx.r4.u32);
  ReturnU32(ctx, GuestAddress(base, delimiters ? std::strtok(str, delimiters) : nullptr));
}

extern "C" PPC_FUNC(rexcrt_strrchr) {
  char* str = GuestString(base, ctx.r3.u32);
  ReturnU32(ctx, GuestAddress(base, str ? std::strrchr(str, static_cast<int>(ctx.r4.u32)) : nullptr));
}

extern "C" PPC_FUNC(rexcrt_memcpy) {
  void* dst = GuestPointer(base, ctx.r3.u32);
  const void* src = GuestPointer(base, ctx.r4.u32);
  if (dst != nullptr && src != nullptr) {
    std::memcpy(dst, src, ctx.r5.u32);
  }
  ReturnU32(ctx, ctx.r3.u32);
}

extern "C" PPC_FUNC(rexcrt_memset) {
  void* dst = GuestPointer(base, ctx.r3.u32);
  if (dst != nullptr) {
    std::memset(dst, static_cast<int>(ctx.r4.u32), ctx.r5.u32);
  }
  ReturnU32(ctx, ctx.r3.u32);
}

extern "C" PPC_FUNC(rexcrt_XMemCpy) {
  void* dst = GuestPointer(base, ctx.r3.u32);
  const void* src = GuestPointer(base, ctx.r4.u32);
  if (dst != nullptr && src != nullptr) {
    std::memcpy(dst, src, ctx.r5.u32);
  }
  ReturnU32(ctx, ctx.r3.u32);
}

extern "C" PPC_FUNC(rexcrt_CreateFileA) {
  ReturnU32(ctx, kSwitchAuditInvalidHandle);
}

extern "C" PPC_FUNC(rexcrt_CloseHandle) {
  ReturnU32(ctx, 0);
}

extern "C" PPC_FUNC(rexcrt_GetFileSize) {
  StoreGuestU32(base, ctx.r4.u32, 0);
  ReturnU32(ctx, kSwitchAuditInvalidHandle);
}

extern "C" PPC_FUNC(rexcrt_SetFilePointer) {
  ReturnU32(ctx, kSwitchAuditInvalidHandle);
}

extern "C" PPC_FUNC(rexcrt_ReadFile) {
  StoreGuestU32(base, ctx.r6.u32, 0);
  ReturnU32(ctx, 0);
}

extern "C" PPC_FUNC(rexcrt_WriteFile) {
  StoreGuestU32(base, ctx.r6.u32, 0);
  ReturnU32(ctx, 0);
}

extern "C" PPC_FUNC(rexcrt_GetFileSizeEx) {
  StoreGuestU64(base, ctx.r4.u32, 0);
  ReturnU32(ctx, 0);
}

extern "C" PPC_FUNC(rexcrt_SetFileTime) {
  ReturnU32(ctx, 0);
}

extern "C" PPC_FUNC(rexcrt_GetFileAttributesA) {
  ReturnU32(ctx, kSwitchAuditInvalidHandle);
}

extern "C" PPC_FUNC(rexcrt_SetFileAttributesA) {
  ReturnU32(ctx, 0);
}

extern "C" PPC_FUNC(rexcrt_SetEndOfFile) {
  ReturnU32(ctx, 0);
}

extern "C" PPC_FUNC(rexcrt_DeleteFileA) {
  ReturnU32(ctx, 0);
}

extern "C" PPC_FUNC(rexcrt_MoveFileA) {
  ReturnU32(ctx, 0);
}

extern "C" PPC_FUNC(rexcrt_CreateDirectoryA) {
  ReturnU32(ctx, 0);
}

extern "C" PPC_FUNC(rexcrt_RtlSizeHeap) {
  ReturnU32(ctx, kSwitchAuditInvalidHandle);
}

extern "C" PPC_FUNC(rexcrt_RtlAllocateHeap) {
  ReturnU32(ctx, 0);
}

extern "C" PPC_FUNC(rexcrt_RtlFreeHeap) {
  ReturnU32(ctx, 0);
}

extern "C" PPC_FUNC(rexcrt_RtlReAllocateHeap) {
  ReturnU32(ctx, 0);
}

extern "C" PPC_FUNC(rexcrt_FlushFileBuffers) {
  ReturnU32(ctx, 0);
}

extern "C" PPC_FUNC(rexcrt_FindFirstFileA) {
  ReturnU32(ctx, kSwitchAuditInvalidHandle);
}

extern "C" PPC_FUNC(rexcrt_FindNextFileA) {
  ReturnU32(ctx, 0);
}

extern "C" PPC_FUNC(rexcrt_FindClose) {
  ReturnU32(ctx, 0);
}

extern "C" PPC_FUNC(rexcrt_RemoveDirectoryA) {
  ReturnU32(ctx, 0);
}

extern "C" PPC_FUNC(rexcrt_SetFilePointerEx) {
  StoreGuestU64(base, ctx.r6.u32, 0);
  ReturnU32(ctx, 0);
}

extern "C" PPC_FUNC(rexcrt_GetFileAttributesExA) {
  if (ctx.r5.u32 != 0) {
    std::memset(GuestPointer(base, ctx.r5.u32), 0, 36);
  }
  ReturnU32(ctx, 0);
}

extern "C" PPC_FUNC(rexcrt_CompareFileTime) {
  ReturnU32(ctx, 0);
}

extern "C" PPC_FUNC(rexcrt_CopyFileA) {
  ReturnU32(ctx, 0);
}

extern "C" PPC_FUNC(rexcrt_GetFileType) {
  ReturnU32(ctx, 0);
}

extern "C" PPC_FUNC(rexcrt_GetDiskFreeSpaceExA) {
  StoreGuestU64(base, ctx.r4.u32, 0);
  StoreGuestU64(base, ctx.r5.u32, 0);
  StoreGuestU64(base, ctx.r6.u32, 0);
  ReturnU32(ctx, 0);
}

#endif  // defined(__SWITCH__)

namespace rex::input {

X_RESULT InputSystem::GetCapabilities(uint32_t, uint32_t, X_INPUT_CAPABILITIES* out_caps) {
  if (out_caps != nullptr) {
    std::memset(out_caps, 0, sizeof(*out_caps));
  }
  return X_ERROR_DEVICE_NOT_CONNECTED;
}

X_RESULT InputSystem::GetState(uint32_t, X_INPUT_STATE* out_state) {
  if (out_state != nullptr) {
    std::memset(out_state, 0, sizeof(*out_state));
  }
  return X_ERROR_DEVICE_NOT_CONNECTED;
}

X_RESULT InputSystem::SetState(uint32_t, X_INPUT_VIBRATION*) {
  return X_ERROR_SUCCESS;
}

X_RESULT InputSystem::GetKeystroke(uint32_t, uint32_t, X_INPUT_KEYSTROKE* out_keystroke) {
  if (out_keystroke != nullptr) {
    std::memset(out_keystroke, 0, sizeof(*out_keystroke));
  }
  return X_ERROR_DEVICE_NOT_CONNECTED;
}

}  // namespace rex::input

namespace rex::audio {

X_STATUS AudioSystem::RegisterClient(uint32_t, uint32_t, size_t* out_index) {
  if (out_index != nullptr) {
    *out_index = 0;
  }
  return X_STATUS_SUCCESS;
}

void AudioSystem::UnregisterClient(size_t) {}

void AudioSystem::SubmitFrame(size_t, uint32_t) {}

uint32_t XmaDecoder::AllocateContext() {
  return 0;
}

void XmaDecoder::ReleaseContext(uint32_t) {}

bool XmaDecoder::BlockOnContext(uint32_t, bool) {
  return false;
}

void XmaDecoder::WriteRegister(uint32_t, uint32_t) {}

}  // namespace rex::audio
