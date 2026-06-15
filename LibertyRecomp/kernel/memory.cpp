#include <stdafx.h>
#include "memory.h"
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <new>

#if defined(LIBERTY_RECOMP_SWITCH)
extern "C" {
uint32_t svcGetInfo(uint64_t* out, uint32_t id, uint32_t handle, uint64_t sub);
uint32_t svcMapPhysicalMemory(void* address, uint64_t size);
uint32_t svcOutputDebugString(const char* str, uint64_t size);
}

extern "C" {
size_t __nx_heap_size = 16ull * 1024 * 1024;
}

#if !defined(LIBERTY_RECOMP_SWITCH_GUEST_LOW_MEMORY_AUDIT_SIZE)
#define LIBERTY_RECOMP_SWITCH_GUEST_LOW_MEMORY_AUDIT_SIZE 0x4000000ull
#endif
#if !defined(LIBERTY_RECOMP_SWITCH_GUEST_PHYSICAL_HEAP_AUDIT_SIZE)
#define LIBERTY_RECOMP_SWITCH_GUEST_PHYSICAL_HEAP_AUDIT_SIZE 0x2000000ull
#endif

static constexpr size_t kSwitchPageSize = 0x1000;
static constexpr size_t kSwitchGuestLowMemorySize = LIBERTY_RECOMP_SWITCH_GUEST_LOW_MEMORY_AUDIT_SIZE;
static constexpr size_t kSwitchGuestPhysicalHeapBegin = 0x80000000ull;
static constexpr size_t kSwitchGuestPhysicalHeapMaxSize = PPC_IMAGE_BASE - kSwitchGuestPhysicalHeapBegin;
static constexpr size_t kSwitchGuestPhysicalHeapSize = LIBERTY_RECOMP_SWITCH_GUEST_PHYSICAL_HEAP_AUDIT_SIZE;
static constexpr size_t kSwitchXmaIoBegin = 0x7FEA0000ull;
static constexpr size_t kSwitchXmaIoSize = 0x10000ull;
static constexpr uint32_t kSwitchCurrentProcessHandle = 0xFFFF8001;
static constexpr uint32_t kSwitchInfoAliasRegionAddress = 2;
static constexpr uint32_t kSwitchInfoAliasRegionSize = 3;
static constexpr uint32_t kSwitchInfoSystemResourceSizeTotal = 16;
static constexpr uint32_t kSwitchInfoSystemResourceSizeUsed = 17;

static void SwitchMemoryDebug(const char* message) noexcept
{
    svcOutputDebugString(message, std::strlen(message));
}

static void SwitchMemoryDebugf(const char* format, ...) noexcept
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    buffer[sizeof(buffer) - 1] = '\0';
    SwitchMemoryDebug(buffer);
}

static void LogSwitchSystemResource(const char* phase, const char* label) noexcept
{
    uint64_t systemResourceTotal = 0;
    uint64_t systemResourceUsed = 0;
    const uint32_t totalRc = svcGetInfo(&systemResourceTotal, kSwitchInfoSystemResourceSizeTotal, kSwitchCurrentProcessHandle, 0);
    const uint32_t usedRc = svcGetInfo(&systemResourceUsed, kSwitchInfoSystemResourceSizeUsed, kSwitchCurrentProcessHandle, 0);

    SwitchMemoryDebugf(
        "[Switch][Memory] system resource %s %s rc=(0x%08X,0x%08X) total=0x%llX used=0x%llX\n",
        phase,
        label,
        totalRc,
        usedRc,
        static_cast<unsigned long long>(systemResourceTotal),
        static_cast<unsigned long long>(systemResourceUsed));
}
#endif

static constexpr size_t AlignDown(size_t value, size_t alignment) noexcept
{
    return value & ~(alignment - 1);
}

static constexpr size_t AlignUp(size_t value, size_t alignment) noexcept
{
    return (value + (alignment - 1)) & ~(alignment - 1);
}

#if defined(LIBERTY_RECOMP_SWITCH)
static bool MapSwitchGuestRange(uint8_t* guestBase, size_t offset, size_t size, const char* label) noexcept
{
    const size_t mapBegin = AlignDown(offset, kSwitchPageSize);
    const size_t mapEnd = AlignUp(offset + size, kSwitchPageSize);

    SwitchMemoryDebugf(
        "[Switch][Memory] map begin %s guest=0x%08zX size=0x%zX\n",
        label,
        mapBegin,
        mapEnd > mapBegin ? mapEnd - mapBegin : 0);

    if (mapEnd <= mapBegin || mapEnd > PPC_MEMORY_SIZE)
        return false;

    LogSwitchSystemResource("before", label);
    const uint32_t rc = svcMapPhysicalMemory(guestBase + mapBegin, mapEnd - mapBegin);
    if (rc != 0)
    {
        std::fprintf(stderr,
            "[Switch][Memory] svcMapPhysicalMemory failed for %s: guest=0x%08zX size=0x%zX rc=0x%08X\n",
            label,
            mapBegin,
            mapEnd - mapBegin,
            rc);
        std::fflush(stderr);
        SwitchMemoryDebugf("[Switch][Memory] map failed %s rc=0x%08X\n", label, rc);
        LogSwitchSystemResource("after-failed", label);
        return false;
    }

    std::memset(guestBase + mapBegin, 0, mapEnd - mapBegin);
    SwitchMemoryDebugf("[Switch][Memory] map ok %s\n", label);
    LogSwitchSystemResource("after", label);
    return true;
}

static size_t GetSwitchImageAndFunctionTableSize() noexcept
{
    constexpr size_t kFunctionTableBegin = PPC_IMAGE_BASE + PPC_IMAGE_SIZE;
    constexpr size_t kFunctionTableSize = (PPC_CODE_SIZE * 2) + sizeof(PPCFunc*);
    constexpr size_t kImageAndFunctionTableBegin = AlignDown(PPC_IMAGE_BASE, kSwitchPageSize);
    constexpr size_t kImageAndFunctionTableEnd = AlignUp(kFunctionTableBegin + kFunctionTableSize, kSwitchPageSize);

#if defined(LIBERTY_RECOMP_SWITCH_GUEST_IMAGE_TABLE_AUDIT_SIZE)
    return LIBERTY_RECOMP_SWITCH_GUEST_IMAGE_TABLE_AUDIT_SIZE;
#elif defined(LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_STOP_AFTER_XENON_INIT)
    constexpr size_t kXenonFixedEnd = AlignUp(0x831F0000ull, kSwitchPageSize);
    const size_t mappedSize = kXenonFixedEnd - kImageAndFunctionTableBegin;
    SwitchMemoryDebugf("[Switch][Memory] Xenon fixed memory mapped_size=0x%zX\n", mappedSize);
    return mappedSize;
#elif defined(LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_SKIP_FUNCTION_MAPPINGS)
    return kImageAndFunctionTableEnd - kImageAndFunctionTableBegin;
#else
    size_t maxTableEnd = kFunctionTableBegin + kFunctionTableSize;
    uint32_t maxGuest = static_cast<uint32_t>(PPC_CODE_BASE + PPC_CODE_SIZE);

    for (size_t i = 0; PPCFuncMappings[i].guest != 0; i++)
    {
        const uint32_t guest = PPCFuncMappings[i].guest;
        if (guest < PPC_CODE_BASE)
            continue;

        const size_t tableEnd = kFunctionTableBegin + ((static_cast<size_t>(guest) - PPC_CODE_BASE) * 2) + sizeof(PPCFunc*);
        if (tableEnd > maxTableEnd)
        {
            maxTableEnd = tableEnd;
            maxGuest = guest;
        }
    }

    const size_t mappedEnd = AlignUp(maxTableEnd, kSwitchPageSize);
    SwitchMemoryDebugf(
        "[Switch][Memory] function table max guest=0x%08X mapped_size=0x%zX\n",
        maxGuest,
        mappedEnd - kImageAndFunctionTableBegin);
    return mappedEnd - kImageAndFunctionTableBegin;
#endif
}

static uint8_t* AllocateSwitchGuestMemory() noexcept
{
    SwitchMemoryDebug("[Switch][Memory] allocate begin\n");

    uint64_t aliasRegionAddress = 0;
    uint64_t aliasRegionSize = 0;
    uint64_t systemResourceTotal = 0;
    uint64_t systemResourceUsed = 0;
    const uint32_t aliasAddressRc = svcGetInfo(&aliasRegionAddress, kSwitchInfoAliasRegionAddress, kSwitchCurrentProcessHandle, 0);
    const uint32_t aliasSizeRc = svcGetInfo(&aliasRegionSize, kSwitchInfoAliasRegionSize, kSwitchCurrentProcessHandle, 0);
    const uint32_t systemResourceTotalRc = svcGetInfo(&systemResourceTotal, kSwitchInfoSystemResourceSizeTotal, kSwitchCurrentProcessHandle, 0);
    const uint32_t systemResourceUsedRc = svcGetInfo(&systemResourceUsed, kSwitchInfoSystemResourceSizeUsed, kSwitchCurrentProcessHandle, 0);

    SwitchMemoryDebugf(
        "[Switch][Memory] alias rc=(0x%08X,0x%08X) base=0x%llX size=0x%llX\n",
        aliasAddressRc,
        aliasSizeRc,
        static_cast<unsigned long long>(aliasRegionAddress),
        static_cast<unsigned long long>(aliasRegionSize));
    SwitchMemoryDebugf(
        "[Switch][Memory] system resource rc=(0x%08X,0x%08X) total=0x%llX used=0x%llX\n",
        systemResourceTotalRc,
        systemResourceUsedRc,
        static_cast<unsigned long long>(systemResourceTotal),
        static_cast<unsigned long long>(systemResourceUsed));

    if (aliasAddressRc != 0 || aliasSizeRc != 0 || aliasRegionAddress == 0 || aliasRegionSize < PPC_MEMORY_SIZE)
    {
        std::fprintf(stderr,
            "[Switch][Memory] Failed to locate a 4 GiB Alias region for sparse PPC memory.\n");
        std::fflush(stderr);
        SwitchMemoryDebug("[Switch][Memory] alias region failed\n");
        return nullptr;
    }

    if (systemResourceTotal == 0)
    {
        std::fprintf(stderr,
            "[Switch][Memory] NPDM SystemResourceSize is zero; sparse PPC memory cannot use svcMapPhysicalMemory.\n");
        std::fflush(stderr);
        SwitchMemoryDebug("[Switch][Memory] missing system resource\n");
        return nullptr;
    }

    auto* guestBase = reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(aliasRegionAddress));

    constexpr size_t kImageAndFunctionTableBegin = AlignDown(PPC_IMAGE_BASE, kSwitchPageSize);
    const size_t kImageAndFunctionTableSize = GetSwitchImageAndFunctionTableSize();
#if defined(LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_STOP_AFTER_XENON_INIT)
    const char* imageMappingLabel = "Xenon fixed memory";
#else
    const char* imageMappingLabel = "image/function table";
#endif

    if (!MapSwitchGuestRange(guestBase, 0, kSwitchGuestLowMemorySize, "low guest heap"))
        return nullptr;

    if (!MapSwitchGuestRange(guestBase, kSwitchXmaIoBegin, kSwitchXmaIoSize, "XMA I/O window"))
        return nullptr;

    if (kSwitchGuestPhysicalHeapSize > kSwitchGuestPhysicalHeapMaxSize)
    {
        SwitchMemoryDebug("[Switch][Memory] physical guest heap audit size exceeds PPC physical heap window\n");
        return nullptr;
    }

    if (!MapSwitchGuestRange(guestBase, kSwitchGuestPhysicalHeapBegin, kSwitchGuestPhysicalHeapSize, "physical guest heap"))
        return nullptr;

    if (!MapSwitchGuestRange(guestBase,
            kImageAndFunctionTableBegin,
            kImageAndFunctionTableSize,
            imageMappingLabel))
        return nullptr;

    std::fprintf(stderr,
        "[Switch][Memory] Sparse guest memory reserved at %p; mapped low=0x%zX physical=0x%zX image_table=0x%zX.\n",
        guestBase,
        kSwitchGuestLowMemorySize,
        kSwitchGuestPhysicalHeapSize,
        kImageAndFunctionTableSize);
    std::fflush(stderr);

    SwitchMemoryDebug("[Switch][Memory] allocate success\n");
    return guestBase;
}
#endif

Memory::Memory()
{
#ifdef _WIN32
    base = (uint8_t*)VirtualAlloc((void*)0x100000000ull, PPC_MEMORY_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if (base == nullptr)
        base = (uint8_t*)VirtualAlloc(nullptr, PPC_MEMORY_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if (base == nullptr)
        return;

    // Some titles (e.g., GTA IV) legitimately touch low memory (including address 0).
    // Do not install a null-page guard in that case.
#elif defined(LIBERTY_RECOMP_SWITCH)
    SwitchMemoryDebug("[Switch][Memory] constructor enter\n");
#if !defined(LIBERTY_RECOMP_SWITCH_ENABLE_GUEST_MEMORY_AUDIT)
    // Current Switch startup audit builds stop in main() before guest code runs.
    // Avoid pre-main guest-memory SVC mapping until the real page-backed design is ready.
    base = nullptr;
    SwitchMemoryDebug("[Switch][Memory] guest memory disabled for startup audit; base=null\n");
    return;
#else
    base = AllocateSwitchGuestMemory();

    if (base == nullptr)
    {
        SwitchMemoryDebug("[Switch][Memory] constructor base=null\n");
        return;
    }
#endif
#else
    base = (uint8_t*)mmap((void*)0x100000000ull, PPC_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

    if (base == (uint8_t*)MAP_FAILED)
        base = (uint8_t*)mmap(NULL, PPC_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

    if (base == (uint8_t*)MAP_FAILED)
    {
        base = nullptr;
        return;
    }

    // Some titles (e.g., GTA IV) legitimately touch low memory (including address 0).
    // Do not install a null-page guard in that case.
#endif

#if defined(LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_STOP_AFTER_XENON_INIT)
    SwitchMemoryDebug("[Switch][Memory] startup-Xenon audit; skipping function mappings\n");
    return;
#elif defined(LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_SKIP_FUNCTION_MAPPINGS)
    SwitchMemoryDebug("[Switch][Memory] audit build; skipping function mappings\n");
    return;
#endif

#if defined(LIBERTY_RECOMP_SWITCH)
    SwitchMemoryDebug("[Switch][Memory] inserting function mappings\n");
#endif

    for (size_t i = 0; PPCFuncMappings[i].guest != 0; i++)
    {
        if (PPCFuncMappings[i].host != nullptr)
            InsertFunction(PPCFuncMappings[i].guest, PPCFuncMappings[i].host);
    }

#if defined(LIBERTY_RECOMP_SWITCH)
    SwitchMemoryDebug("[Switch][Memory] constructor complete\n");
#endif

    // Protect the recomp function lookup table from guest writes.
    // If the game overwrites these host function pointers, it can lead to crashes that look like
    // invalid indirect branches / pointer-authentication failures on arm64e.
    constexpr size_t kPageSize = 0x1000;
    constexpr size_t kFuncTableOffset = PPC_IMAGE_BASE + PPC_IMAGE_SIZE;
    constexpr size_t kFuncTableSize = (PPC_CODE_SIZE * 2) + sizeof(PPCFunc*);
    const size_t protectBegin = AlignDown(kFuncTableOffset, kPageSize);
    const size_t protectEnd = AlignUp(kFuncTableOffset + kFuncTableSize, kPageSize);
    if (protectEnd > protectBegin)
    {
#ifdef _WIN32
        DWORD oldProtect{};
        VirtualProtect(base + protectBegin, protectEnd - protectBegin, PAGE_READONLY, &oldProtect);
#elif defined(LIBERTY_RECOMP_SWITCH)
        // libnx does not expose desktop mprotect here; keep the backing store writable for now.
#else
        mprotect(base + protectBegin, protectEnd - protectBegin, PROT_READ);
#endif
    }
}

void* MmGetHostAddress(uint32_t ptr)
{
    return g_memory.Translate(ptr);
}
