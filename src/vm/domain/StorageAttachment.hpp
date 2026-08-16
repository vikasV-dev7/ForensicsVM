#pragma once
#include "EvidenceSource.hpp"
#include <string>

namespace fvm::domain {

enum class AccessMode {
    ReadOnly,
    Overlay
};

enum class BusType {
    VirtIO,
    SATA,
    NVMe,
    IDE
};

struct StorageAttachment {
    std::string diskId;
    EvidenceSource evidence;
    AccessMode access;
    BusType bus;
    bool bootable;

    bool isValid() const noexcept {
        return !diskId.empty() && evidence.isValid();
    }
};

} // namespace fvm::domain
