#pragma once
#include <string>
#include "EvidenceSource.hpp"

namespace fvm::domain {

struct AcquisitionResult {
    std::string temporaryFilePath;
    DiskFormat format;
};

} // namespace fvm::domain
