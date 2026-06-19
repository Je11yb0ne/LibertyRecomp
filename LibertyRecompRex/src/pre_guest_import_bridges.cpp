#include <rex/ppc/function.h>

// Sidecar-only registered exports for the current codegen-only rexkernel.lib.
// Replace these with full ReXGlue SDK exports or GTA IV-specific overrides
// before allowing LaunchModule().

XAM_EXPORT_STUB(__imp__XamShowMessageBoxUIEx)
XAM_EXPORT_STUB(__imp__XamShowGamerCardUIForXUID)
XAM_EXPORT_STUB(__imp__XamShowPlayerReviewUI)
XAM_EXPORT_STUB(__imp__XamShowDeviceSelectorUI)
XAM_EXPORT_STUB(__imp__XamShowDirtyDiscErrorUI)

XBOXKRNL_EXPORT_STUB(__imp__XeKeysConsoleSignatureVerification)
XBOXKRNL_EXPORT_STUB(__imp__XeCryptSha)
XBOXKRNL_EXPORT_STUB(__imp__XeKeysConsolePrivateKeySign)
