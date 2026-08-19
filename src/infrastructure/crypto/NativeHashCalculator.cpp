#include "NativeHashCalculator.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#ifdef _MSC_VER
#pragma comment(lib, "bcrypt.lib")
#endif
#endif

namespace fvm::infrastructure::crypto {

NativeHashCalculator::NativeHashCalculator() {}
NativeHashCalculator::~NativeHashCalculator() {}

std::expected<std::string, fvm::contracts::HashError> NativeHashCalculator::calculateSha256(const std::filesystem::path& path) {
#ifdef _WIN32
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    DWORD cbData = 0, cbHash = 0, cbHashObject = 0;
    std::vector<BYTE> pbHashObject;
    std::vector<BYTE> pbHash;

    if (!std::filesystem::exists(path)) {
        return std::unexpected(fvm::contracts::HashError::FileNotFound);
    }

    if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0))) {
        return std::unexpected(fvm::contracts::HashError::CryptoError);
    }

    if (!BCRYPT_SUCCESS(BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbHashObject, sizeof(DWORD), &cbData, 0))) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return std::unexpected(fvm::contracts::HashError::CryptoError);
    }
    
    pbHashObject.resize(cbHashObject);

    if (!BCRYPT_SUCCESS(BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&cbHash, sizeof(DWORD), &cbData, 0))) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return std::unexpected(fvm::contracts::HashError::CryptoError);
    }

    pbHash.resize(cbHash);

    if (!BCRYPT_SUCCESS(BCryptCreateHash(hAlg, &hHash, pbHashObject.data(), cbHashObject, NULL, 0, 0))) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return std::unexpected(fvm::contracts::HashError::CryptoError);
    }

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return std::unexpected(fvm::contracts::HashError::ReadError);
    }

    const size_t bufferSize = 1024 * 1024; // 1MB chunks
    std::vector<char> buffer(bufferSize);

    while (file.read(buffer.data(), bufferSize) || file.gcount() > 0) {
        if (!BCRYPT_SUCCESS(BCryptHashData(hHash, (PBYTE)buffer.data(), static_cast<ULONG>(file.gcount()), 0))) {
            BCryptDestroyHash(hHash);
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return std::unexpected(fvm::contracts::HashError::CryptoError);
        }
    }

    if (!BCRYPT_SUCCESS(BCryptFinishHash(hHash, pbHash.data(), cbHash, 0))) {
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return std::unexpected(fvm::contracts::HashError::CryptoError);
    }

    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (BYTE b : pbHash) {
        ss << std::setw(2) << static_cast<int>(b);
    }

    return ss.str();
#else
    return std::unexpected(fvm::contracts::HashError::CryptoError);
#endif
}

} // namespace fvm::infrastructure::crypto
