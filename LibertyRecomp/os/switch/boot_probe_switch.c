#include <switch.h>

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static const char* const kProbeLogPath = "sdmc:/switch/LibertyRecomp/boot_probe.log";

static void probe_debug(const char* message)
{
    svcOutputDebugString(message, strlen(message));
}

static bool probe_sdmc_available(void)
{
    if (fsdevGetDeviceFileSystem("sdmc") != NULL)
        return true;

    struct stat sdmc_stat;
    errno = 0;
    return stat("sdmc:/", &sdmc_stat) == 0;
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    consoleInit(NULL);
    consoleDebugInit(debugDevice_SVC);

    printf("\x1b[2J");
    printf("Liberty Boot Probe - SD Diagnostics\n");
    printf("===================================\n\n");

    const bool sdmc_before_mount = probe_sdmc_available();
    const Result sdmc_result = fsdevMountSdmc();
    const bool sdmc_after_mount = probe_sdmc_available();
    const bool sdmc_usable = R_SUCCEEDED(sdmc_result) || sdmc_after_mount;

    char line[256];
    snprintf(line, sizeof(line),
        "[SwitchBootProbe] entered main; sdmc_before=%u fsdevMountSdmc=0x%08x sdmc_after=%u\n",
        sdmc_before_mount ? 1u : 0u,
        (unsigned int)sdmc_result,
        sdmc_after_mount ? 1u : 0u);
    probe_debug(line);
    printf("sdmc before mount: %s\n", sdmc_before_mount ? "yes" : "no");
    printf("fsdevMountSdmc: 0x%08x\n", (unsigned int)sdmc_result);
    printf("sdmc after mount: %s\n", sdmc_after_mount ? "yes" : "no");

    if (sdmc_usable)
    {
        errno = 0;
        const int mkdir_switch = mkdir("sdmc:/switch", 0777);
        const int mkdir_switch_errno = errno;

        errno = 0;
        const int mkdir_root = mkdir("sdmc:/switch/LibertyRecomp", 0777);
        const int mkdir_root_errno = errno;

        struct stat root_stat;
        errno = 0;
        const int stat_root = stat("sdmc:/switch/LibertyRecomp", &root_stat);
        const int stat_root_errno = errno;

        errno = 0;
        FILE* log = fopen(kProbeLogPath, "a");
        const int fopen_errno = errno;

        printf("mkdir sdmc:/switch: %d errno=%d\n", mkdir_switch, mkdir_switch_errno);
        printf("mkdir LibertyRecomp: %d errno=%d\n", mkdir_root, mkdir_root_errno);
        printf("stat LibertyRecomp: %d errno=%d\n", stat_root, stat_root_errno);
        printf("fopen boot_probe.log: %s errno=%d\n", log != NULL ? "ok" : "failed", fopen_errno);

        if (log != NULL)
        {
            fputs(line, log);
            fputs("[SwitchBootProbe] sleeping indefinitely; close manually with HOME.\n", log);
            fflush(log);
            printf("ferror boot_probe.log: %d\n", ferror(log));
            fclose(log);
        }
    }
    else
    {
        printf("SD device unavailable; file write skipped.\n");
    }

    printf("\nNo game code is running.\n");
    printf("Use HOME / close software to exit.\n");

    for (;;)
    {
        consoleUpdate(NULL);
        svcSleepThread(1000000000LL);
    }
}
