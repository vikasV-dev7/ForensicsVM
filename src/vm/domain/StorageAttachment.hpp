#pragma once
#include "EvidenceId.hpp"
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
    EvidenceId evidenceId;
    AccessMode access;
    BusType bus;
    bool bootable;

    bool isValid() const noexcept {
        return !diskId.empty() && !evidenceId.empty();
    }
};

} // namespace fvm::domain
