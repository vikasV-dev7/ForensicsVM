#pragma once
#include "EvidenceId.hpp"
#include <string>
#include <chrono>

namespace fvm::domain {

struct DerivedArtifact {
    std::string operationName;
    std::chrono::system_clock::time_point timestamp;
    EvidenceId evidenceId;
};

} // namespace fvm::domain
