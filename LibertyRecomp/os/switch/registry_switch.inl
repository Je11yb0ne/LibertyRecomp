#include <os/registry.h>

// Switch audit stub: desktop registry settings need a real file-backed store later.
inline bool os::registry::Init()
{
    return false;
}

template<typename T>
bool os::registry::ReadValue(const std::string_view&, T&)
{
    return false;
}

template<typename T>
bool os::registry::WriteValue(const std::string_view&, const T&)
{
    return false;
}
