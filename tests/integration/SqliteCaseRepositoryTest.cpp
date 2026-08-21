#include <gtest/gtest.h>
#include "infrastructure/sqlite/SqliteCaseRepository.hpp"
#include "infrastructure/sqlite/DatabaseContext.hpp"
#include "infrastructure/crypto/NativeHashCalculator.hpp"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

using namespace fvm::infrastructure::sqlite;
using namespace fvm::core::application::domain;
using namespace fvm::core::application::contracts;
using namespace fvm::domain;

class SqliteCaseRepositoryTest : public ::testing::Test {
protected:
    std::filesystem::path tempDir;

    void SetUp() override {
        tempDir = std::filesystem::temp_directory_path() / ("fvm_case_test_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(tempDir);
    }

    void TearDown() override {
        std::filesystem::remove_all(tempDir);
    }
};

TEST_F(SqliteCaseRepositoryTest, CreateAndLoadCase) {
    auto hasher = std::make_shared<fvm::infrastructure::crypto::NativeHashCalculator>();
    auto dbContext = std::make_shared<fvm::infrastructure::sqlite::DatabaseContext>(hasher);
    SqliteCaseRepository repo(dbContext);
    CaseMetadata meta{"Test Case", "Description", "Investigator", 100, 200};
    Case newCase(CaseId("case-1"), meta);
    
    newCase.addEvidenceId(EvidenceId("ev-1"));
    newCase.addVmId(VmId("vm-1"));

    auto res = repo.createCase(newCase, tempDir);
    ASSERT_TRUE(res.has_value()) << "Failed to create case";

    fvm::core::application::domain::AuditRecord audit;
    audit.eventId = "event-1";
    audit.timestampUnixMs = 12345;
    audit.eventType = "CaseCreated";
    audit.payload = "{}";
    
    repo.beginTransaction();
    repo.commitTransaction(audit);

    auto loadRes = repo.loadCase(tempDir);
    ASSERT_TRUE(loadRes.has_value()) << "Failed to load case";
    
    EXPECT_EQ(loadRes->getId().value(), "case-1");
    EXPECT_EQ(loadRes->getMetadata().name, "Test Case");
    EXPECT_EQ(loadRes->getMetadata().investigator, "Investigator");
    
    ASSERT_EQ(loadRes->getEvidenceIds().size(), 1);
    EXPECT_EQ(loadRes->getEvidenceIds()[0].value(), "ev-1");
    
    ASSERT_EQ(loadRes->getVmIds().size(), 1);
    EXPECT_EQ(loadRes->getVmIds()[0].value(), "vm-1");
}

TEST_F(SqliteCaseRepositoryTest, RelocateCase) {
    {
        auto hasher = std::make_shared<fvm::infrastructure::crypto::NativeHashCalculator>();
        auto dbContext = std::make_shared<fvm::infrastructure::sqlite::DatabaseContext>(hasher);
        SqliteCaseRepository repo(dbContext);
        CaseMetadata meta{"Relocatable", "Desc", "Inv", 1, 1};
        Case newCase(CaseId("case-2"), meta);

        auto res = repo.createCase(newCase, tempDir);
        ASSERT_TRUE(res.has_value());

        fvm::core::application::domain::AuditRecord audit;
        audit.eventId = "event-2";
        audit.timestampUnixMs = 12345;
        audit.eventType = "CaseCreated";
        audit.payload = "{}";
        
        repo.beginTransaction();
        repo.commitTransaction(audit);
    }

    std::filesystem::path newLocation = tempDir.parent_path() / (tempDir.filename().string() + "_moved");
    std::filesystem::rename(tempDir, newLocation);

    {
        auto hasher2 = std::make_shared<fvm::infrastructure::crypto::NativeHashCalculator>();
        auto dbContext2 = std::make_shared<fvm::infrastructure::sqlite::DatabaseContext>(hasher2);
        SqliteCaseRepository repo2(dbContext2);
        auto loadRes = repo2.loadCase(newLocation);
        ASSERT_TRUE(loadRes.has_value());
        EXPECT_EQ(loadRes->getId().value(), "case-2");
    }

    std::filesystem::remove_all(newLocation);
}
