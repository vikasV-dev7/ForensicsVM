#include "vm/domain/StorageAttachment.hpp"
#include <iostream>

using namespace fvm::domain;

int main() {
    int failed = 0;

    // ValidEvidenceSource
    {
        EvidenceSource evidence("C:\\some\\evidence.raw", DiskFormat::Raw);
        if (!evidence.isValid() || evidence.format() != DiskFormat::Raw || evidence.path().string() != "C:\\some\\evidence.raw") {
            std::cerr << "Fail: ValidEvidenceSource\n";
            failed++;
        }
    }

    // InvalidEvidenceSource
    {
        EvidenceSource evidence("", DiskFormat::Raw);
        if (evidence.isValid()) {
            std::cerr << "Fail: InvalidEvidenceSource\n";
            failed++;
        }
    }

    // ValidStorageAttachment
    {
        EvidenceSource evidence("C:\\some\\evidence.qcow2", DiskFormat::Qcow2);
        StorageAttachment attachment{
            "disk0",
            evidence,
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
        EvidenceSource evidence("C:\\some\\evidence.vhdx", DiskFormat::Vhdx);
        StorageAttachment attachment{
            "",
            evidence,
            AccessMode::ReadOnly,
            BusType::SATA,
            false
        };
        if (attachment.isValid()) {
            std::cerr << "Fail: InvalidStorageAttachment_EmptyDiskId\n";
            failed++;
        }
    }

    if (failed == 0) {
        std::cout << "StorageAttachmentTest PASS\n";
        return 0;
    }
    return 1;
}
