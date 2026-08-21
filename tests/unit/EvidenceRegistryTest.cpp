#include <gtest/gtest.h>
#include "vm/management/EvidenceRegistry.hpp"
#include "infrastructure/inmemory/InMemoryEvidenceRepository.hpp"
#include "vm/contracts/IHashCalculator.hpp"
#include <fstream>
#include <filesystem>

using namespace fvm::management;
using namespace fvm::infrastructure::inmemory;
using namespace fvm::contracts;
using namespace fvm::domain;

class MockHashCalculator : public IHashCalculator {
public:
    std::string fixedHash = "mocked-hash";
    bool shouldFail = false;

    std::expected<std::string, HashError> calculateSha256(const std::filesystem::path&) override {
        if (shouldFail) return std::unexpected(HashError::CryptoError);
        return fixedHash;
    }
    
    std::expected<std::string, HashError> calculateSha256(const std::string&) override {
        if (shouldFail) return std::unexpected(HashError::CryptoError);
        return fixedHash;
    }
};

class EvidenceRegistryTest : public ::testing::Test {
protected:
    std::filesystem::path tempFile_;

    void SetUp() override {
        tempFile_ = std::filesystem::temp_directory_path() / "fvm_registry_test.dat";
        std::ofstream out(tempFile_, std::ios::binary);
        out << "registry test";
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove(tempFile_, ec);
    }
};

TEST_F(EvidenceRegistryTest, IngestSuccessfully) {
    auto repo = std::make_unique<InMemoryEvidenceRepository>();
    auto hasher = std::make_unique<MockHashCalculator>();
    hasher->fixedHash = "deadbeef";
    
    EvidenceRegistry registry(std::move(repo), std::move(hasher));
    
    auto idRes = registry.ingest(tempFile_, DiskFormat::Raw);
    ASSERT_TRUE(idRes.has_value());
    
    auto record = registry.getEvidence(idRes.value());
    ASSERT_TRUE(record.has_value());
    EXPECT_EQ(record->status(), EvidenceStatus::Verified);
    EXPECT_EQ(record->sha256(), "deadbeef");
    EXPECT_EQ(record->path(), tempFile_);
}

TEST_F(EvidenceRegistryTest, IngestHashFailure) {
    auto repo = std::make_unique<InMemoryEvidenceRepository>();
    auto hasher = std::make_unique<MockHashCalculator>();
    hasher->shouldFail = true;
    
    EvidenceRegistry registry(std::move(repo), std::move(hasher));
    
    auto idRes = registry.ingest(tempFile_, DiskFormat::Raw);
    ASSERT_FALSE(idRes.has_value());
    EXPECT_EQ(idRes.error(), "Failed to compute SHA-256 hash");
}

TEST_F(EvidenceRegistryTest, IngestMissingFile) {
    auto repo = std::make_unique<InMemoryEvidenceRepository>();
    auto hasher = std::make_unique<MockHashCalculator>();
    EvidenceRegistry registry(std::move(repo), std::move(hasher));
    
    auto idRes = registry.ingest("C:\\nonexistent_fvm_registry.dat", DiskFormat::Raw);
    ASSERT_FALSE(idRes.has_value());
}

TEST_F(EvidenceRegistryTest, MissingAfterVerification) {
    auto repo = std::make_unique<InMemoryEvidenceRepository>();
    auto hasher = std::make_unique<MockHashCalculator>();
    EvidenceRegistry registry(std::move(repo), std::move(hasher));
    
    auto idRes = registry.ingest(tempFile_, DiskFormat::Raw);
    ASSERT_TRUE(idRes.has_value());
    
    std::error_code ec;
    std::filesystem::remove(tempFile_, ec);
    
    auto record = registry.getEvidence(idRes.value());
    ASSERT_TRUE(record.has_value());
    EXPECT_EQ(record->status(), EvidenceStatus::Missing);
}
