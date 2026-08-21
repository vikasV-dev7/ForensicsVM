#pragma once
#include "vm/contracts/IHashCalculator.hpp"

namespace fvm::infrastructure::crypto {

class NativeHashCalculator : public fvm::contracts::IHashCalculator {
public:
    NativeHashCalculator();
    ~NativeHashCalculator() override;

    std::expected<std::string, fvm::contracts::HashError> calculateSha256(const std::filesystem::path& path) override;
    std::expected<std::string, fvm::contracts::HashError> calculateSha256(const std::string& data) override;
};

} // namespace fvm::infrastructure::crypto
