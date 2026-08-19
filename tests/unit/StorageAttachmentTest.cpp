#include "vm/domain/StorageAttachment.hpp"
#include <iostream>

using namespace fvm::domain;

int main() {
    int failed = 0;

    // ValidStorageAttachment
    {
        StorageAttachment attachment{
            "disk0",
            EvidenceId("valid-id"),
            AccessMode::Overlay,
            BusType::VirtIO,
            true
        };
        if (!attachment.isValid() || attachment.access != AccessMode::Overlay) {
            std::cerr << "Fail: ValidStorageAttachment\n";
            failed++;
        }
    }

    // InvalidStorageAttachment_EmptyDiskId
    {
        StorageAttachment attachment{
            "",
            EvidenceId("valid-id"),
            AccessMode::ReadOnly,
            BusType::SATA,
            false
        };
        if (attachment.isValid()) {
            std::cerr << "Fail: InvalidStorageAttachment_EmptyDiskId\n";
            failed++;
        }
    }

    // InvalidStorageAttachment_EmptyEvidenceId
    {
        StorageAttachment attachment{
            "disk1",
            EvidenceId(""),
            AccessMode::ReadOnly,
            BusType::SATA,
            false
        };
        if (attachment.isValid()) {
            std::cerr << "Fail: InvalidStorageAttachment_EmptyEvidenceId\n";
            failed++;
        }
    }

    if (failed == 0) {
        std::cout << "StorageAttachmentTest PASS\n";
        return 0;
    }
    return 1;
}
