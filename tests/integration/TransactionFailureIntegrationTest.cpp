#include <gtest/gtest.h>
#include "core/application/services/ForensicApplicationImpl.hpp"
#include "infrastructure/sqlite/DatabaseContext.hpp"
#include "infrastructure/sqlite/SqliteCaseRepository.hpp"
#include "infrastructure/sqlite/SqliteVmRepository.hpp"
#include "infrastructure/sqlite/SqliteEvidenceRepository.hpp"
#include "infrastructure/crypto/NativeHashCalculator.hpp"
#include "infrastructure/qemu/QemuBackend.hpp"
#include "infrastructure/qemu/QemuLocator.hpp"
#include "infrastructure/qemu/QemuCommandBuilder.hpp"
#include "infrastructure/qemu/image/QemuImgLocator.hpp"
#include "infrastructure/qemu/image/QemuImageTool.hpp"
#include "vm/contracts/IHashCalculator.hpp"
#include "vm/management/VmManager.hpp"
#include "vm/management/EvidenceRegistry.hpp"
#include <filesystem>
#include <thread>
#include <chrono>
#include <atomic>
#include <fstream>

using namespace fvm::core::application::services;
using namespace fvm::core::application::domain;
using namespace fvm::infrastructure::sqlite;

class FailingHashCalculator : public fvm::contracts::IHashCalculator {
public:
    std::atomic<bool> shouldFail{false};

    std::expected<std::string, fvm::contracts::HashError> calculateSha256(const std::string& data) override {
        (void)data;
        if (shouldFail) {
            return std::unexpected(fvm::contracts::HashError::CryptoError);
        }
        return "fake_hash_value";
    }

    std::expected<std::string, fvm::contracts::HashError> calculateSha256(const std::filesystem::path& path) override {
        (void)path;
        if (shouldFail) {
            return std::unexpected(fvm::contracts::HashError::CryptoError);
        }
        return "fake_hash_value";
    }
};

class TransactionFailureIntegrationTest : public ::testing::Test {
protected:
    std::filesystem::path tempDir;
    std::shared_ptr<FailingHashCalculator> hasher;
    std::shared_ptr<ForensicApplicationImpl> app;

    void SetUp() override {
        tempDir = std::filesystem::temp_directory_path() / ("fvm_tx_fail_test_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(tempDir);

        hasher = std::make_shared<FailingHashCalculator>();
        auto dbContext = std::make_shared<DatabaseContext>(hasher);
        auto repo = std::make_shared<SqliteCaseRepository>(dbContext);
        auto evRepo = std::make_unique<fvm::infrastructure::sqlite::SqliteEvidenceRepository>(dbContext);
        
        auto evidence = std::make_shared<fvm::management::EvidenceRegistry>(
            std::move(evRepo), 
            std::make_unique<fvm::infrastructure::crypto::NativeHashCalculator>()
        );
        
        auto qemuLocator = std::make_unique<fvm::infrastructure::qemu::DefaultQemuLocator>();
        auto qemuImgLocator = std::make_unique<fvm::infrastructure::qemu::image::DefaultQemuImgLocator>();
        auto qemuImgTool = std::make_unique<fvm::infrastructure::qemu::image::QemuImageTool>(std::move(qemuImgLocator));
        auto qemuCommandBuilder = std::make_unique<fvm::infrastructure::qemu::DefaultQemuCommandBuilder>();
        auto backend = std::make_unique<fvm::infrastructure::qemu::QemuBackend>(
            std::move(qemuLocator),
            std::move(qemuCommandBuilder),
            std::move(qemuImgTool)
        );
        auto vmRepo = std::make_unique<fvm::infrastructure::sqlite::SqliteVmRepository>(dbContext);
        auto manager = std::make_shared<fvm::management::VmManager>(std::move(vmRepo), std::move(backend), evidence);
        
        app = std::make_shared<ForensicApplicationImpl>(repo, manager, evidence);
    }

    void TearDown() override {
        if (app) app->closeCase();
        app.reset();
        std::filesystem::remove_all(tempDir);
    }
};

TEST_F(TransactionFailureIntegrationTest, CommitFailure_FailsClosedAndRollsBackMemoryState) {
    // 1. Initial valid state
    hasher->shouldFail = false;
    CaseMetadata meta{"Tx Fail Test Case", "Desc", "Inv", 100, 200};
    auto res = app->createCase(tempDir, meta);
    ASSERT_TRUE(res.has_value()) << "Failed to create case";

    auto activeCase = app->getActiveCase();
    ASSERT_TRUE(activeCase.has_value());
    size_t initialEvidenceCount = activeCase->getEvidenceIds().size();
    
    // 2. Trigger transaction failure on import
    hasher->shouldFail = true;
    
    std::filesystem::path dummyEvidence = tempDir / "dummy.raw";
    std::ofstream(dummyEvidence) << "dummy";
    
    auto importRes = app->importEvidence(dummyEvidence);
    ASSERT_TRUE(importRes.has_value());
    OperationId opId = *importRes;
    
    bool completed = false;
    OperationState finalState = OperationState::Queued;
    
    for (int i = 0; i < 50; ++i) {
        auto status = app->getOperationStatus(opId);
        if (status->state == OperationState::Failed || status->state == OperationState::Completed) {
            completed = true;
            finalState = status->state;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    ASSERT_TRUE(completed) << "Operation timed out";
    
    // 3. Assert Fail-Closed: The operation must report failure, not success.
    EXPECT_EQ(finalState, OperationState::Failed) << "Operation ignored transaction failure and returned success!";
    
    // 4. Assert In-Memory State Rollback: The case should not have the evidence ID added.
    auto afterCase = app->getActiveCase();
    ASSERT_TRUE(afterCase.has_value());
    EXPECT_EQ(afterCase->getEvidenceIds().size(), initialEvidenceCount) << "In-memory state was mutated despite transaction failure!";
}
