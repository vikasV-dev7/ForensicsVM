#include <gtest/gtest.h>
#include "infrastructure/sqlite/SqliteCaseRepository.hpp"
#include "infrastructure/sqlite/DatabaseContext.hpp"
#include "infrastructure/crypto/NativeHashCalculator.hpp"
#include <filesystem>
#include <chrono>
#include <sqlite3.h>
#include "core/application/ApplicationBootstrap.hpp"
#include <fstream>
#include <thread>

using namespace fvm::infrastructure::sqlite;
using namespace fvm::core::application::domain;
using namespace fvm::core::application::contracts;
using namespace fvm::domain;

class ForensicTamperDetectionIntegrationTest : public ::testing::Test {
protected:
    std::filesystem::path tempDir;
    std::filesystem::path dbPath;

    void SetUp() override {
        tempDir = std::filesystem::temp_directory_path() / ("fvm_tamper_test_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(tempDir);
        dbPath = tempDir / "case.fvmcase";
    }

    void TearDown() override {
        std::filesystem::remove_all(tempDir);
    }

    void executeRawSql(const std::string& sql) {
        sqlite3* db = nullptr;
        ASSERT_EQ(sqlite3_open(dbPath.string().c_str(), &db), SQLITE_OK);
        char* errMsg = nullptr;
        int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
        if (errMsg) {
            std::cerr << "SQL Error: " << errMsg << "\n";
            sqlite3_free(errMsg);
        }
        ASSERT_EQ(rc, SQLITE_OK);
        sqlite3_close(db);
    }

    void createValidCaseAndRecords() {
        auto app = fvm::core::application::ApplicationBootstrap::createApplication();
        CaseMetadata meta{"Tamper Test Case", "Description", "Investigator", 100, 200};
        
        auto res = app->createCase(tempDir, meta);
        ASSERT_TRUE(res.has_value()) << "Failed to create initial valid case";
        
        // Create an evidence to generate another audit record
        std::ofstream ofs(tempDir / "dummy.raw");
        ofs << "dummy evidence";
        ofs.close();
        
        auto importRes = app->importEvidence(tempDir / "dummy.raw");
        ASSERT_TRUE(importRes.has_value());
        OperationId opId = *importRes;
        
        bool completed = false;
        for (int i = 0; i < 50; ++i) {
            auto status = app->getOperationStatus(opId);
            if (status->state == OperationState::Completed) {
                completed = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        ASSERT_TRUE(completed) << "Evidence import did not complete in time";
        
        app->closeCase();
        // At this point DatabaseContext is closed and sqlite3 connection is released
    }
};

TEST_F(ForensicTamperDetectionIntegrationTest, PositiveControl_ValidCaseOpens) {
    createValidCaseAndRecords();

    auto hasher = std::make_shared<fvm::infrastructure::crypto::NativeHashCalculator>();
    auto dbContext = std::make_shared<fvm::infrastructure::sqlite::DatabaseContext>(hasher);
    SqliteCaseRepository repo(dbContext);
    
    auto loadRes = repo.loadCase(tempDir);
    ASSERT_TRUE(loadRes.has_value()) << "Valid untampered case failed to open";
}

TEST_F(ForensicTamperDetectionIntegrationTest, GenesisRecordTampering_DetectsCorruption) {
    createValidCaseAndRecords();
    executeRawSql("UPDATE audit_log SET payload = '{\"tampered\": true}' WHERE seq_num = (SELECT MIN(seq_num) FROM audit_log);");

    auto hasher = std::make_shared<fvm::infrastructure::crypto::NativeHashCalculator>();
    auto dbContext = std::make_shared<fvm::infrastructure::sqlite::DatabaseContext>(hasher);
    SqliteCaseRepository repo(dbContext);
    
    auto loadRes = repo.loadCase(tempDir);
    ASSERT_FALSE(loadRes.has_value());
    EXPECT_EQ(loadRes.error(), CaseError::DatabaseCorruption);
}

TEST_F(ForensicTamperDetectionIntegrationTest, EmptyAuditLog_DetectsCorruption) {
    createValidCaseAndRecords();
    executeRawSql("DELETE FROM audit_log;");

    auto hasher = std::make_shared<fvm::infrastructure::crypto::NativeHashCalculator>();
    auto dbContext = std::make_shared<fvm::infrastructure::sqlite::DatabaseContext>(hasher);
    SqliteCaseRepository repo(dbContext);
    
    auto loadRes = repo.loadCase(tempDir);
    ASSERT_FALSE(loadRes.has_value());
    EXPECT_EQ(loadRes.error(), CaseError::DatabaseCorruption);
}

TEST_F(ForensicTamperDetectionIntegrationTest, PayloadTampering_DetectsCorruption) {
    createValidCaseAndRecords();
    executeRawSql("UPDATE audit_log SET payload = '{\"tampered\": true}' WHERE seq_num = (SELECT MAX(seq_num) FROM audit_log);");

    auto hasher = std::make_shared<fvm::infrastructure::crypto::NativeHashCalculator>();
    auto dbContext = std::make_shared<fvm::infrastructure::sqlite::DatabaseContext>(hasher);
    SqliteCaseRepository repo(dbContext);
    
    auto loadRes = repo.loadCase(tempDir);
    ASSERT_FALSE(loadRes.has_value());
    EXPECT_EQ(loadRes.error(), CaseError::DatabaseCorruption);
}

TEST_F(ForensicTamperDetectionIntegrationTest, CurrentHashTampering_DetectsCorruption) {
    createValidCaseAndRecords();
    executeRawSql("UPDATE audit_log SET current_hash = 'badhash' WHERE seq_num = (SELECT MAX(seq_num) FROM audit_log);");

    auto hasher = std::make_shared<fvm::infrastructure::crypto::NativeHashCalculator>();
    auto dbContext = std::make_shared<fvm::infrastructure::sqlite::DatabaseContext>(hasher);
    SqliteCaseRepository repo(dbContext);
    
    auto loadRes = repo.loadCase(tempDir);
    ASSERT_FALSE(loadRes.has_value());
    EXPECT_EQ(loadRes.error(), CaseError::DatabaseCorruption);
}

TEST_F(ForensicTamperDetectionIntegrationTest, PreviousHashTampering_DetectsCorruption) {
    createValidCaseAndRecords();
    executeRawSql("UPDATE audit_log SET previous_hash = 'badhash' WHERE seq_num = (SELECT MAX(seq_num) FROM audit_log);");

    auto hasher = std::make_shared<fvm::infrastructure::crypto::NativeHashCalculator>();
    auto dbContext = std::make_shared<fvm::infrastructure::sqlite::DatabaseContext>(hasher);
    SqliteCaseRepository repo(dbContext);
    
    auto loadRes = repo.loadCase(tempDir);
    ASSERT_FALSE(loadRes.has_value());
    EXPECT_EQ(loadRes.error(), CaseError::DatabaseCorruption);
}

TEST_F(ForensicTamperDetectionIntegrationTest, IntermediateRecordDeletion_DetectsCorruption) {
    createValidCaseAndRecords();
    
    // We expect there to be more than 1 record (Genesis + case creation). Let's delete the genesis record or sequence 0.
    executeRawSql("DELETE FROM audit_log WHERE seq_num = (SELECT MIN(seq_num) FROM audit_log);");

    auto hasher = std::make_shared<fvm::infrastructure::crypto::NativeHashCalculator>();
    auto dbContext = std::make_shared<fvm::infrastructure::sqlite::DatabaseContext>(hasher);
    SqliteCaseRepository repo(dbContext);
    
    auto loadRes = repo.loadCase(tempDir);
    ASSERT_FALSE(loadRes.has_value());
    EXPECT_EQ(loadRes.error(), CaseError::DatabaseCorruption);
}

TEST_F(ForensicTamperDetectionIntegrationTest, SequenceIntegrity_DetectsCorruption) {
    createValidCaseAndRecords();
    
    // Alter sequence number ordering. We will swap seq_num 0 and 1, or just change MAX(seq_num) to something else.
    // However, if we just swap them, previous_hash chaining will fail. Let's just modify the seq_num to be out of order.
    // Actually, ORDER BY seq_num ASC is used during verification. If we change seq_num 1 to 999, the order is still preserved if there's no intermediate, but if we change 0 to 999, the chain validation expects expectedPrevHash = GENESIS but gets the hash from record 1!
    executeRawSql("UPDATE audit_log SET seq_num = 999 WHERE seq_num = (SELECT MIN(seq_num) FROM audit_log);");

    auto hasher = std::make_shared<fvm::infrastructure::crypto::NativeHashCalculator>();
    auto dbContext = std::make_shared<fvm::infrastructure::sqlite::DatabaseContext>(hasher);
    SqliteCaseRepository repo(dbContext);
    
    auto loadRes = repo.loadCase(tempDir);
    ASSERT_FALSE(loadRes.has_value());
    EXPECT_EQ(loadRes.error(), CaseError::DatabaseCorruption);
}
