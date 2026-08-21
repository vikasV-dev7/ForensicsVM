#pragma once
#include <string>
#include <filesystem>
#include <expected>
#include <system_error>

namespace fvm::contracts {

enum class HashError {
    FileNotFound,
    AccessDenied,
    ReadError,
    CryptoError
};

class IHashCalculator {
public:
    virtual ~IHashCalculator() = default;

    virtual std::expected<std::string, HashError> calculateSha256(const std::filesystem::path& path) = 0;
    virtual std::expected<std::string, HashError> calculateSha256(const std::string& data) = 0;
};

} // namespace fvm::contracts
