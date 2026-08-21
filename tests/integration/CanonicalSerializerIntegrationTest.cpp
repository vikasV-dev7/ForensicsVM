#include <gtest/gtest.h>
#include "core/application/services/ForensicApplicationImpl.hpp"
#include "infrastructure/sqlite/SqliteCaseRepository.hpp"
#include "infrastructure/sqlite/DatabaseContext.hpp"
#include "infrastructure/crypto/NativeHashCalculator.hpp"
#include "vm/management/VmManager.hpp"
#include "vm/management/EvidenceRegistry.hpp"
#include "core/application/ApplicationBootstrap.hpp"
#include <filesystem>
#include <thread>
#include <chrono>
#include <sqlite3.h>

using namespace fvm::core::application::services;
using namespace fvm::core::application::domain;
using namespace fvm::infrastructure::sqlite;

class CanonicalSerializerIntegrationTest : public ::testing::Test {
protected:
    std::filesystem::path tempDir;
    std::shared_ptr<ForensicApplicationImpl> app;

    void SetUp() override {
        tempDir = std::filesystem::temp_directory_path() / ("fvm_canon_test_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(tempDir);

        std::shared_ptr<fvm::core::application::contracts::IForensicApplication> baseApp = fvm::core::application::ApplicationBootstrap::createApplication();
        app = std::dynamic_pointer_cast<ForensicApplicationImpl>(baseApp);
    }

    void TearDown() override {
        if (app) app->closeCase();
        app.reset();
        std::filesystem::remove_all(tempDir);
    }

    std::vector<std::string> getAuditPayloads() {
        std::vector<std::string> payloads;
        sqlite3* db = nullptr;
        std::string dbPath = (tempDir / "case.fvmcase").string();
        if (sqlite3_open(dbPath.c_str(), &db) == SQLITE_OK) {
            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(db, "SELECT payload FROM audit_log ORDER BY seq_num ASC;", -1, &stmt, nullptr) == SQLITE_OK) {
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    payloads.push_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
                }
                sqlite3_finalize(stmt);
            }
            sqlite3_close(db);
        }
        return payloads;
    }
};

TEST_F(CanonicalSerializerIntegrationTest, ProductionPayloadsAreDeterministicAndCanonical) {
    CaseMetadata meta{"Canon Test Case", "Desc", "Inv", 100, 200};
    auto res = app->createCase(tempDir, meta);
    ASSERT_TRUE(res.has_value());

    auto payloads = getAuditPayloads();
    ASSERT_EQ(payloads.size(), 1);
    
    std::string genesisPayload = payloads[0];
    
    // Check canonical formatting:
    // 1. No spaces outside of string values
    // 2. Alphabetical keys: "action" should come before "case_id", then "timestamp"
    // Since timestamp is variable in length but alphabetical rule is strict:
    
    EXPECT_EQ(genesisPayload.find(" "), std::string::npos) << "Payload contains whitespace";
    EXPECT_NE(genesisPayload.find("\"action\":\"CASE_CREATED\""), std::string::npos);
    EXPECT_NE(genesisPayload.find("\"case_id\":"), std::string::npos);
    EXPECT_NE(genesisPayload.find("\"timestamp\":"), std::string::npos);
    
    // Verify the order explicitly by searching indices
    size_t actionPos = genesisPayload.find("\"action\"");
    size_t caseIdPos = genesisPayload.find("\"case_id\"");
    size_t timestampPos = genesisPayload.find("\"timestamp\"");
    
    EXPECT_LT(actionPos, caseIdPos) << "Keys are not sorted alphabetically (action vs case_id)";
    EXPECT_LT(caseIdPos, timestampPos) << "Keys are not sorted alphabetically (case_id vs timestamp)";
}
