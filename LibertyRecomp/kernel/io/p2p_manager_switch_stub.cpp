#include <cstdio>

#include <steam/isteamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#include <steam/steamnetworkingsockets.h>

#if defined(__SWITCH__)
// Switch audit stub: GameNetworkingSockets is not ported/linked for Switch yet.
extern "C" bool GameNetworkingSockets_Init(const SteamNetworkingIdentity*, SteamNetworkingErrMsg& errMsg)
{
    std::snprintf(errMsg, sizeof(SteamNetworkingErrMsg), "GameNetworkingSockets is disabled on Switch audit builds");
    return false;
}

extern "C" void GameNetworkingSockets_Kill()
{
}

extern "C" ISteamNetworkingSockets* SteamNetworkingSockets_LibV12()
{
    return nullptr;
}

extern "C" ISteamNetworkingUtils* SteamNetworkingUtils_LibV4()
{
    return nullptr;
}
#endif
