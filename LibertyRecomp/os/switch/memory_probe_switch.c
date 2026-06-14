#include <switch.h>

#include <errno.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static const char* const kProbeLogPath = "sdmc:/switch/LibertyRecomp/memory_probe.log";

static FILE* g_log;

size_t __nx_heap_size = 96ull * 1024 * 1024;

static void probe_debug(const char* message)
{
    svcOutputDebugString(message, strlen(message));
    if (g_log != NULL)
    {
        fputs(message, g_log);
        fflush(g_log);
    }
}

static void probe_logf(const char* format, ...)
{
    char line[512];
    va_list args;
    va_start(args, format);
    vsnprintf(line, sizeof(line), format, args);
    va_end(args);
    line[sizeof(line) - 1] = '\0';

    printf("%s", line);
    probe_debug(line);
}

static bool probe_sdmc_available(void)
{
    if (fsdevGetDeviceFileSystem("sdmc") != NULL)
        return true;

    struct stat sdmc_stat;
    errno = 0;
    return stat("sdmc:/", &sdmc_stat) == 0;
}

static void probe_mount_sdmc(void)
{
    const bool sdmc_before_mount = probe_sdmc_available();
    const Result sdmc_result = fsdevMountSdmc();
    const bool sdmc_after_mount = probe_sdmc_available();

    if (R_SUCCEEDED(sdmc_result) || sdmc_after_mount)
    {
        mkdir("sdmc:/switch", 0777);
        mkdir("sdmc:/switch/LibertyRecomp", 0777);
        g_log = fopen(kProbeLogPath, "a");
    }

    probe_logf(
        "[SwitchMemoryProbe] sdmc_before=%u fsdevMountSdmc=0x%08x sdmc_after=%u log=%s\n",
        sdmc_before_mount ? 1u : 0u,
        (unsigned int)sdmc_result,
        sdmc_after_mount ? 1u : 0u,
        g_log != NULL ? "ok" : "none");
}

static void query_memory(const char* label, const void* address)
{
    MemoryInfo meminfo;
    uint32_t pageinfo = 0;
    memset(&meminfo, 0, sizeof(meminfo));

    const Result rc = svcQueryMemory(&meminfo, &pageinfo, (uint64_t)(uintptr_t)address);
    probe_logf(
        "%s query addr=%p rc=0x%08x base=0x%016llx size=0x%016llx type=0x%02x attr=0x%08x perm=0x%08x page=0x%08x\n",
        label,
        address,
        (unsigned int)rc,
        (unsigned long long)meminfo.addr,
        (unsigned long long)meminfo.size,
        (unsigned int)meminfo.type,
        (unsigned int)meminfo.attr,
        (unsigned int)meminfo.perm,
        (unsigned int)pageinfo);
}

static void get_info_u64(const char* label, InfoType type)
{
    uint64_t value = 0;
    const Result rc = svcGetInfo(&value, (uint32_t)type, CUR_PROCESS_HANDLE, 0);
    probe_logf("%s rc=0x%08x value=0x%016llx (%llu)\n",
        label,
        (unsigned int)rc,
        (unsigned long long)value,
        (unsigned long long)value);
}

static uint64_t get_info_value(InfoType type)
{
    uint64_t value = 0;
    const Result rc = svcGetInfo(&value, (uint32_t)type, CUR_PROCESS_HANDLE, 0);
    return R_SUCCEEDED(rc) ? value : 0;
}

static void get_info_u64_sub(const char* label, InfoType type, uint64_t subtype)
{
    uint64_t value = 0;
    const Result rc = svcGetInfo(&value, (uint32_t)type, CUR_PROCESS_HANDLE, subtype);
    probe_logf("%s subtype=0x%llx rc=0x%08x value=0x%016llx (%llu)\n",
        label,
        (unsigned long long)subtype,
        (unsigned int)rc,
        (unsigned long long)value,
        (unsigned long long)value);
}

static void touch_memory(void* address, size_t size)
{
    volatile uint8_t* bytes = (volatile uint8_t*)address;
    bytes[0] = 0x4C;
    bytes[size - 1] = 0x52;
    probe_logf("touch ok addr=%p first=0x%02x last=0x%02x\n",
        address,
        (unsigned int)bytes[0],
        (unsigned int)bytes[size - 1]);
}

static void run_map_test(const char* label, size_t reserve_size, size_t map_offset, size_t map_size, bool add_reservation, bool map_while_locked)
{
    probe_logf("\n-- %s reserve=0x%zx map_offset=0x%zx map_size=0x%zx add_res=%u locked_map=%u --\n",
        label,
        reserve_size,
        map_offset,
        map_size,
        add_reservation ? 1u : 0u,
        map_while_locked ? 1u : 0u);

    VirtmemReservation* reservation = NULL;
    void* base = NULL;
    Result map_rc = 0xFFFFFFFF;

    virtmemLock();
    base = virtmemFindAslr(reserve_size, 0x1000);
    if (base != NULL && add_reservation)
        reservation = virtmemAddReservation(base, reserve_size);

    if (base != NULL)
    {
        query_memory("before", (uint8_t*)base + map_offset);
        if (map_while_locked)
            map_rc = svcMapPhysicalMemory((uint8_t*)base + map_offset, map_size);
    }
    virtmemUnlock();

    probe_logf("find base=%p reservation=%p\n", base, reservation);
    if (base == NULL)
        return;

    if (!map_while_locked)
        map_rc = svcMapPhysicalMemory((uint8_t*)base + map_offset, map_size);

    probe_logf("svcMapPhysicalMemory rc=0x%08x\n", (unsigned int)map_rc);
    query_memory("after map", (uint8_t*)base + map_offset);

    if (R_SUCCEEDED(map_rc))
    {
        touch_memory((uint8_t*)base + map_offset, map_size);
        const Result unmap_rc = svcUnmapPhysicalMemory((uint8_t*)base + map_offset, map_size);
        probe_logf("svcUnmapPhysicalMemory rc=0x%08x\n", (unsigned int)unmap_rc);
        query_memory("after unmap", (uint8_t*)base + map_offset);
    }

    if (reservation != NULL)
    {
        virtmemLock();
        virtmemRemoveReservation(reservation);
        virtmemUnlock();
        probe_logf("reservation removed\n");
    }
}

static void run_fixed_map_test(const char* label, void* base, size_t map_offset, size_t map_size, bool add_reservation)
{
    probe_logf("\n-- %s fixed_base=%p map_offset=0x%zx map_size=0x%zx add_res=%u --\n",
        label,
        base,
        map_offset,
        map_size,
        add_reservation ? 1u : 0u);

    VirtmemReservation* reservation = NULL;
    if (base == NULL)
        return;

    if (add_reservation)
    {
        virtmemLock();
        reservation = virtmemAddReservation(base, map_size + map_offset);
        virtmemUnlock();
        probe_logf("fixed reservation=%p\n", reservation);
    }

    void* map_address = (uint8_t*)base + map_offset;
    query_memory("before fixed", map_address);
    const Result map_rc = svcMapPhysicalMemory(map_address, map_size);
    probe_logf("svcMapPhysicalMemory fixed rc=0x%08x\n", (unsigned int)map_rc);
    query_memory("after fixed map", map_address);

    if (R_SUCCEEDED(map_rc))
    {
        touch_memory(map_address, map_size);
        const Result unmap_rc = svcUnmapPhysicalMemory(map_address, map_size);
        probe_logf("svcUnmapPhysicalMemory fixed rc=0x%08x\n", (unsigned int)unmap_rc);
        query_memory("after fixed unmap", map_address);
    }

    if (reservation != NULL)
    {
        virtmemLock();
        virtmemRemoveReservation(reservation);
        virtmemUnlock();
        probe_logf("fixed reservation removed\n");
    }
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    consoleInit(NULL);
    consoleDebugInit(debugDevice_SVC);

    printf("\x1b[2J");
    printf("Liberty Memory Probe - Switch Audit\n");
    printf("===================================\n\n");

    probe_mount_sdmc();

    probe_logf("[SwitchMemoryProbe] entered main\n");
    get_info_u64("Info AliasRegionAddress", InfoType_AliasRegionAddress);
    get_info_u64("Info AliasRegionSize", InfoType_AliasRegionSize);
    get_info_u64("Info AslrRegionAddress", InfoType_AslrRegionAddress);
    get_info_u64("Info AslrRegionSize", InfoType_AslrRegionSize);
    get_info_u64("Info HeapRegionAddress", InfoType_HeapRegionAddress);
    get_info_u64("Info HeapRegionSize", InfoType_HeapRegionSize);
    get_info_u64("Info TotalMemorySize", InfoType_TotalMemorySize);
    get_info_u64("Info UsedMemorySize", InfoType_UsedMemorySize);
    get_info_u64("Info SystemResourceSizeTotal", InfoType_SystemResourceSizeTotal);
    get_info_u64("Info SystemResourceSizeUsed", InfoType_SystemResourceSizeUsed);
    get_info_u64("Info TotalNonSystemMemorySize", InfoType_TotalNonSystemMemorySize);
    get_info_u64("Info UsedNonSystemMemorySize", InfoType_UsedNonSystemMemorySize);
    get_info_u64_sub("Info IsSvcPermitted svcMapPhysicalMemory", InfoType_IsSvcPermitted, 0x2C);
    get_info_u64_sub("Info IsSvcPermitted svcUnmapPhysicalMemory", InfoType_IsSvcPermitted, 0x2D);
    get_info_u64_sub("Info IsSvcPermitted svcMapPhysicalMemoryUnsafe", InfoType_IsSvcPermitted, 0x48);

    const uint64_t heap_region_address = get_info_value(InfoType_HeapRegionAddress);
    const uint64_t heap_region_size = get_info_value(InfoType_HeapRegionSize);
    const uint64_t heap_free_base = heap_region_address + __nx_heap_size;
    const uint64_t heap_free_end = heap_region_address + heap_region_size;
    const uint64_t alias_region_address = get_info_value(InfoType_AliasRegionAddress);
    const uint64_t alias_region_size = get_info_value(InfoType_AliasRegionSize);
    const uint64_t alias_region_end = alias_region_address + alias_region_size;

    run_map_test("small unlocked no reservation", 0x200000, 0, 0x200000, false, false);
    run_map_test("small locked no reservation", 0x200000, 0, 0x200000, false, true);
    run_map_test("small reservation", 0x200000, 0, 0x200000, true, false);
    run_map_test("four-gib first page", 0x100000000ULL, 0, 0x200000, true, false);
    run_map_test("four-gib image window", 0x100000000ULL, 0x82000000ULL, 0x200000, true, false);

    probe_logf("\nHeap free candidate base=0x%016llx end=0x%016llx\n",
        (unsigned long long)heap_free_base,
        (unsigned long long)heap_free_end);
    run_fixed_map_test("heap free first block", (void*)(uintptr_t)heap_free_base, 0, 0x200000, false);
    run_fixed_map_test("heap free first block reserved", (void*)(uintptr_t)(heap_free_base + 0x200000), 0, 0x200000, true);
    run_fixed_map_test("heap four-gib first block", (void*)(uintptr_t)heap_free_base, 0, 0x200000, false);
    run_fixed_map_test("heap four-gib image window", (void*)(uintptr_t)heap_free_base, 0x82000000ULL, 0x200000, false);

    probe_logf("\nAlias candidate base=0x%016llx end=0x%016llx\n",
        (unsigned long long)alias_region_address,
        (unsigned long long)alias_region_end);
    run_fixed_map_test("alias first block", (void*)(uintptr_t)alias_region_address, 0, 0x200000, false);
    run_fixed_map_test("alias first block reserved", (void*)(uintptr_t)(alias_region_address + 0x200000), 0, 0x200000, true);
    run_fixed_map_test("alias four-gib first block", (void*)(uintptr_t)alias_region_address, 0, 0x200000, false);
    run_fixed_map_test("alias four-gib image window", (void*)(uintptr_t)alias_region_address, 0x82000000ULL, 0x200000, false);

    probe_logf("\nNo game code is running.\n");
    probe_logf("Use HOME / close software to exit.\n");

    for (;;)
    {
        consoleUpdate(NULL);
        svcSleepThread(1000000000LL);
    }
}
