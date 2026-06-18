#include <rex/ppc/function.h>

namespace {
constexpr const char* kBridgeNote =
    "pre-guest ReXGlue sidecar import bridge; replace with full SDK export before launch";
}

PPC_STUB_LOG(__imp__XamShowMessageBoxUIEx, kBridgeNote)
PPC_STUB_LOG(__imp__XamShowGamerCardUIForXUID, kBridgeNote)
PPC_STUB_LOG(__imp__XamShowPlayerReviewUI, kBridgeNote)
PPC_STUB_LOG(__imp__XamShowDeviceSelectorUI, kBridgeNote)
PPC_STUB_LOG(__imp__XamShowDirtyDiscErrorUI, kBridgeNote)

PPC_STUB_LOG(__imp__XeKeysConsoleSignatureVerification, kBridgeNote)
PPC_STUB_LOG(__imp__XeCryptSha, kBridgeNote)
PPC_STUB_LOG(__imp__XeKeysConsolePrivateKeySign, kBridgeNote)
