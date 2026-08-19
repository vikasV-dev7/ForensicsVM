#include <gtest/gtest.h>
#include "core/application/domain/OperationRecord.hpp"
#include "core/application/domain/ApplicationError.hpp"

using namespace fvm::core::application::domain;

TEST(ApplicationOperationTest, ValidateOperationRecordCreation) {
    OperationId id("op-123");
    OperationRecord rec{id, OperationType::ImportEvidence, OperationState::Queued, 1000, 0, 0, "Initial", "", ""};
    
    EXPECT_EQ(rec.id.value(), "op-123");
    EXPECT_EQ(rec.type, OperationType::ImportEvidence);
    EXPECT_EQ(rec.state, OperationState::Queued);
    EXPECT_EQ(rec.createdAtUnixSeconds, 1000);
}

TEST(ApplicationOperationTest, ValidateApplicationErrors) {
    ApplicationError err("General error");
    EXPECT_STREQ(err.what(), "General error");
    
    PathSecurityError pathErr("Path escaped");
    EXPECT_STREQ(pathErr.what(), "Path escaped");
    
    CaseNotOpenError caseErr;
    EXPECT_STREQ(caseErr.what(), "No case is currently open");
}
