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
