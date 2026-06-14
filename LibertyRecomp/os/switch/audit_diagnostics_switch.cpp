extern "C" {
#include <switch.h>
}

#include <cstdio>

extern "C" void LibertySwitchShowAuditDiagnostic(const char* title, const char* status, const char* detailPath, const char* logPath)
{
    consoleInit(nullptr);
    consoleDebugInit(debugDevice_SVC);

    std::printf("\x1b[2J");
    std::printf("Liberty Recomp - Switch Audit\n");
    std::printf("========================================\n\n");
    std::printf("%s\n\n", title != nullptr ? title : "Audit stop");

    if (status != nullptr && status[0] != '\0')
        std::printf("%s\n\n", status);

    if (detailPath != nullptr && detailPath[0] != '\0')
        std::printf("Path:\n%s\n\n", detailPath);

    if (logPath != nullptr && logPath[0] != '\0')
        std::printf("Log:\n%s\n\n", logPath);

    std::printf("This is not a playable build yet.\n");
    std::printf("This audit screen does not auto exit.\n");
    std::printf("Use HOME/close software to leave this screen.\n");
    consoleUpdate(nullptr);

    for (;;)
    {
        consoleUpdate(nullptr);
        svcSleepThread(16666666);
    }

    consoleExit(nullptr);
}
