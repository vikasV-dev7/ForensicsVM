#include <gtest/gtest.h>
#include "vm/domain/EvidenceRecord.hpp"

using namespace fvm::domain;

TEST(EvidenceRecordTest, Initialization) {
    EvidenceRecord record(EvidenceId("test-id"), "C:\\test.img", DiskFormat::Raw, 1024);
    EXPECT_EQ(record.id().value(), "test-id");
    EXPECT_EQ(record.path(), "C:\\test.img");
    EXPECT_EQ(record.format(), DiskFormat::Raw);
    EXPECT_EQ(record.sizeBytes(), 1024);
    EXPECT_EQ(record.status(), EvidenceStatus::Ingesting);
}

TEST(EvidenceRecordTest, StateTransitions) {
    EvidenceRecord record(EvidenceId("test-id"), "C:\\test.img", DiskFormat::Raw, 1024);
    
    record.setVerified("abcd1234hash");
    EXPECT_EQ(record.status(), EvidenceStatus::Verified);
    EXPECT_EQ(record.sha256(), "abcd1234hash");

    record.setMissing();
    EXPECT_EQ(record.status(), EvidenceStatus::Missing);

    record.setFailed();
    EXPECT_EQ(record.status(), EvidenceStatus::Failed);
}
