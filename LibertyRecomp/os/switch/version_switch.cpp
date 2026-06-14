#include <os/version.h>

extern "C" {
#include <switch/runtime/hosversion.h>
}

os::version::OSVersion os::version::GetOSVersion()
{
    const u32 version = hosversionGet();

    return {
        static_cast<uint32_t>(HOSVER_MAJOR(version)),
        static_cast<uint32_t>(HOSVER_MINOR(version)),
        static_cast<uint32_t>(HOSVER_MICRO(version)),
    };
}
