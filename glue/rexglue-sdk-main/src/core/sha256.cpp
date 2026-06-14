/**
 * SHA256 hashing utilities implementation
 *
 * Uses the Windows CNG BCrypt provider for SHA256 computation.
 */

#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>

#include <rex/crypto/sha256.h>

namespace rex::crypto {

std::string sha256(std::string_view data) {
  BCRYPT_ALG_HANDLE algorithm = nullptr;
  BCRYPT_HASH_HANDLE hash = nullptr;
  std::array<uint8_t, 32> digest{};

  if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0) {
    return "";
  }

  if (BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) < 0) {
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return "";
  }

  auto* bytes = reinterpret_cast<PUCHAR>(const_cast<char*>(data.data()));
  if (BCryptHashData(hash, bytes, static_cast<ULONG>(data.size()), 0) < 0 ||
      BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0) < 0) {
    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return "";
  }

  BCryptDestroyHash(hash);
  BCryptCloseAlgorithmProvider(algorithm, 0);

  std::ostringstream out;
  out << std::hex << std::setfill('0');
  for (uint8_t byte : digest) {
    out << std::setw(2) << static_cast<int>(byte);
  }
  return out.str();
}

std::string sha256_file(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return "";
  }
  std::ostringstream ss;
  ss << file.rdbuf();
  return sha256(ss.str());
}

}  // namespace rex::crypto
