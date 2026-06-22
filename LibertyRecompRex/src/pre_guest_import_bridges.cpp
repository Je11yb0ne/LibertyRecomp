#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>

#include <rex/logging.h>
#include <rex/ppc/function.h>
#include <rex/ppc/types.h>

// Sidecar-only registered exports for the current codegen-only rexkernel.lib.
// Replace these with full ReXGlue SDK exports or GTA IV-specific overrides
// as each runtime caller proves it needs stricter behavior.

XAM_EXPORT_STUB(__imp__XamShowMessageBoxUIEx)
XAM_EXPORT_STUB(__imp__XamShowGamerCardUIForXUID)
XAM_EXPORT_STUB(__imp__XamShowPlayerReviewUI)
XAM_EXPORT_STUB(__imp__XamShowDeviceSelectorUI)
XAM_EXPORT_STUB(__imp__XamShowDirtyDiscErrorUI)

namespace {

NTSTATUS hash_sha1_input(BCRYPT_HASH_HANDLE hash, ppc_pvoid_t input, ppc_u32_t input_size) {
    if (!input || input_size == 0) {
        return static_cast<NTSTATUS>(0);
    }

    return BCryptHashData(hash, input.as<PUCHAR>(), static_cast<ULONG>(input_size), 0);
}

}  // namespace

void XeCryptSha_sidecar(ppc_pvoid_t input_1, ppc_u32_t input_1_size, ppc_pvoid_t input_2,
                        ppc_u32_t input_2_size, ppc_pvoid_t input_3, ppc_u32_t input_3_size,
                        ppc_pvoid_t output, ppc_u32_t output_size) {
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    ULONG bytes_written = 0;
    ULONG object_size = 0;
    ULONG hash_size = 0;

    auto status = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA1_ALGORITHM, nullptr, 0);
    if (!BCRYPT_SUCCESS(status)) {
        REXKRNL_ERROR("XeCryptSha sidecar bridge: BCryptOpenAlgorithmProvider failed status={:#x}",
                      static_cast<std::uint32_t>(status));
        return;
    }

    status = BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
                               reinterpret_cast<PUCHAR>(&object_size), sizeof(object_size),
                               &bytes_written, 0);
    if (BCRYPT_SUCCESS(status)) {
        status = BCryptGetProperty(algorithm, BCRYPT_HASH_LENGTH,
                                   reinterpret_cast<PUCHAR>(&hash_size), sizeof(hash_size),
                                   &bytes_written, 0);
    }

    std::vector<std::uint8_t> hash_object(object_size);
    if (BCRYPT_SUCCESS(status)) {
        status = BCryptCreateHash(algorithm, &hash, hash_object.data(),
                                  static_cast<ULONG>(hash_object.size()), nullptr, 0, 0);
    }

    if (BCRYPT_SUCCESS(status)) {
        status = hash_sha1_input(hash, input_1, input_1_size);
    }
    if (BCRYPT_SUCCESS(status)) {
        status = hash_sha1_input(hash, input_2, input_2_size);
    }
    if (BCRYPT_SUCCESS(status)) {
        status = hash_sha1_input(hash, input_3, input_3_size);
    }

    if (BCRYPT_SUCCESS(status)) {
        std::array<std::uint8_t, 20> digest{};
        status = BCryptFinishHash(hash, digest.data(),
                                  std::min<ULONG>(static_cast<ULONG>(digest.size()), hash_size),
                                  0);
        if (BCRYPT_SUCCESS(status) && output && output_size > 0) {
            std::copy_n(digest.data(), std::min<std::size_t>(digest.size(), output_size),
                        output.as<std::uint8_t*>());
        }
    }

    if (!BCRYPT_SUCCESS(status)) {
        REXKRNL_ERROR("XeCryptSha sidecar bridge: BCrypt hashing failed status={:#x}",
                      static_cast<std::uint32_t>(status));
    }

    if (hash != nullptr) {
        BCryptDestroyHash(hash);
    }
    if (algorithm != nullptr) {
        BCryptCloseAlgorithmProvider(algorithm, 0);
    }
}

ppc_u32_result_t XeKeysConsolePrivateKeySign_sidecar(ppc_pvoid_t hash, ppc_pvoid_t signature) {
    (void)hash;
    (void)signature;
    return 0;
}

ppc_u32_result_t XeKeysConsoleSignatureVerification_sidecar(ppc_pvoid_t hash,
                                                            ppc_pvoid_t signature,
                                                            ppc_pvoid_t pubkey) {
    (void)hash;
    (void)signature;
    (void)pubkey;
    return 0;
}

XBOXKRNL_EXPORT(__imp__XeCryptSha, XeCryptSha_sidecar)
XBOXKRNL_EXPORT(__imp__XeKeysConsoleSignatureVerification,
                XeKeysConsoleSignatureVerification_sidecar)
XBOXKRNL_EXPORT(__imp__XeKeysConsolePrivateKeySign, XeKeysConsolePrivateKeySign_sidecar)
