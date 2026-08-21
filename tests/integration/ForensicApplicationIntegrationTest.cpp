#include <gtest/gtest.h>
#include "core/application/services/ForensicApplicationImpl.hpp"
#include "infrastructure/sqlite/SqliteCaseRepository.hpp"
#include "core/application/ApplicationBootstrap.hpp"
#include "vm/domain/EvidenceSource.hpp"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

using namespace fvm::core::application::services;
using namespace fvm::infrastructure::sqlite;
using namespace fvm::core::application::domain;
using namespace fvm::core::application::contracts;
using namespace fvm::domain;

class ForensicApplicationIntegrationTest : public ::testing::Test {
protected:
    std::filesystem::path tempDir;
    std::filesystem::path caseRoot;
    std::filesystem::path externalEvidence;

    void SetUp() override {
        tempDir = std::filesystem::temp_directory_path() / ("fvm_app_test_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(tempDir);
        caseRoot = tempDir / "case";
        externalEvidence = tempDir / "test.raw";
        
        std::ofstream ofs(externalEvidence);
        ofs << "dummy evidence";
    }

    void TearDown() override {
        std::filesystem::remove_all(tempDir);
    }
};

TEST_F(ForensicApplicationIntegrationTest, CaseLifecycle) {
    auto app = fvm::core::application::ApplicationBootstrap::createApplication();

    CaseMetadata meta{"App Test Case", "Integration Test", "Automated", 0, 0};
    auto createRes = app->createCase(caseRoot, meta);
    ASSERT_TRUE(createRes.has_value());
    EXPECT_TRUE(app->isCaseOpen());

    app->closeCase();
    EXPECT_FALSE(app->isCaseOpen());

    auto openRes = app->openCase(caseRoot);
    ASSERT_TRUE(openRes.has_value());
    EXPECT_TRUE(app->isCaseOpen());
}

TEST_F(ForensicApplicationIntegrationTest, ImportEvidenceAsync) {
    auto app = fvm::core::application::ApplicationBootstrap::createApplication();

    CaseMetadata meta{"Test Case", "", "", 0, 0};
    ASSERT_TRUE(app->createCase(caseRoot, meta).has_value());

    auto importRes = app->importEvidence(externalEvidence);
    ASSERT_TRUE(importRes.has_value());
    OperationId opId = *importRes;

    // Wait for the background operation to complete
    bool completed = false;
    for (int i = 0; i < 50; ++i) {
        auto status = app->getOperationStatus(opId);
        ASSERT_TRUE(status.has_value());
        if (status->state == OperationState::Completed) {
            completed = true;
            break;
        }
        if (status->state == OperationState::Failed) {
            FAIL() << "Operation failed: " << status->error;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    EXPECT_TRUE(completed) << "Operation did not complete in time";

    auto activeCase = app->getActiveCase();
    ASSERT_TRUE(activeCase.has_value());
    EXPECT_EQ(activeCase->getEvidenceIds().size(), 1);

    // Verify copy
    EXPECT_TRUE(std::filesystem::exists(caseRoot / "evidence" / "test.raw"));
}

TEST_F(ForensicApplicationIntegrationTest, PathSecurity) {
    auto app = fvm::core::application::ApplicationBootstrap::createApplication();
    CaseMetadata meta{"Security Test", "", "", 0, 0};
    ASSERT_TRUE(app->createCase(caseRoot, meta).has_value());
#ifdef _WIN32
    auto res = app->createCase("Z:\\NonExistentDrive\\Case", meta);
    EXPECT_FALSE(res.has_value());
#endif
}

TEST_F(ForensicApplicationIntegrationTest, Cancellation) {
    auto app = fvm::core::application::ApplicationBootstrap::createApplication();
    CaseMetadata meta{"Cancel Case", "", "", 0, 0};
    ASSERT_TRUE(app->createCase(caseRoot, meta).has_value());

    VmConfig config{VmId{"test-vm-1"}, "test OS", "test desc", 
        CpuConfig{CpuCount{1}, 1, 1, 1}, 
        MemoryConfig{Megabytes{1024}, Megabytes{0}, Megabytes{0}}, 
        std::vector<StorageAttachment>{}, 
        std::vector<NetworkConfig>{}, 
        FirmwareConfig{FirmwareType::BIOS, false, false}, 
        DisplayConfig{}
    };
    
    // launchSession isn't fully mocked for this in the integration test easily, 
    // but we can test acquireDiskDelta which relies on VmManager.
    // Wait, the InMemoryBackend needs a running VM. 
    // Let's just create a VM and start it, then cancel.
    // The problem is `launchSession` needs a valid EvidenceId if the VmConfig demands it, 
    // but we can pass an empty config.
    auto launchRes = app->launchSession(config);
    ASSERT_TRUE(launchRes.has_value());
    OperationId launchOp = *launchRes;
    
    // Wait for launch to finish
    for (int i = 0; i < 50; ++i) {
        if (app->getOperationStatus(launchOp)->state == OperationState::Completed) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    auto acquireRes = app->acquireDiskDelta(VmId{"test-vm-1"});
    ASSERT_TRUE(acquireRes.has_value());
    OperationId acqOp = *acquireRes;
    
    // Immediately cancel
    auto cancelRes = app->cancelOperation(acqOp);
    ASSERT_TRUE(cancelRes.has_value());
    
    // Wait for status to reflect cancellation
    bool cancelled = false;
    for (int i = 0; i < 50; ++i) {
        auto status = app->getOperationStatus(acqOp);
        if (status->state == OperationState::Cancelled) {
            cancelled = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    EXPECT_TRUE(cancelled) << "Operation was not cancelled successfully";
}

TEST_F(ForensicApplicationIntegrationTest, ApplicationDestruction) {
    auto start = std::chrono::steady_clock::now();
    {
        auto app = fvm::core::application::ApplicationBootstrap::createApplication();
        CaseMetadata meta{"Shutdown Case", "", "", 0, 0};
        app->createCase(caseRoot, meta);
        
        VmConfig config{VmId{"test-vm-2"}, "test OS", "test desc", 
            CpuConfig{CpuCount{1}, 1, 1, 1}, 
            MemoryConfig{Megabytes{1024}, Megabytes{0}, Megabytes{0}}, 
            std::vector<StorageAttachment>{}, 
            std::vector<NetworkConfig>{}, 
            FirmwareConfig{FirmwareType::BIOS, false, false}, 
            DisplayConfig{}
        };
        auto opId = app->launchSession(config);
        ASSERT_TRUE(opId.has_value());
        
        // Wait for launch to finish completely so we don't have lock contention on VmManager mutex
        for (int i = 0; i < 100; ++i) {
            auto status = app->getOperationStatus(*opId);
            if (status->state == OperationState::Completed) break;
            if (status->state == OperationState::Failed) {
                FAIL() << "launchSession failed: " << status->error;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Start an acquisition and immediately destroy app
        app->acquireDiskDelta(VmId{"test-vm-2"});
    }
    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Destruction should be near instantaneous because of stop_token, but real QEMU process shutdown can take up to 3 seconds.
    EXPECT_LT(elapsed.count(), 5000) << "Application destruction took too long, likely deadlocked";
}
