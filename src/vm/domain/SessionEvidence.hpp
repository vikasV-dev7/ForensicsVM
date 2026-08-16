#pragma once
#include <string>
#include "StorageAttachment.hpp" // For AccessMode

namespace fvm::domain {

struct SessionEvidence {
    std::string diskId;
    std::string evidenceSha256;
    AccessMode access;
    std::string overlayPath;
};

} // namespace fvm::domain
