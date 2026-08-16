#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include "VmId.hpp"
#include "VmState.hpp"
#include "TerminationReason.hpp"
#include "SessionEvidence.hpp"

namespace fvm::domain {

using ExecutionSessionId = std::string;

struct ExecutionSession {
    ExecutionSessionId sessionId;
    VmId vmId;
    std::vector<SessionEvidence> evidence;
    
    std::chrono::system_clock::time_point startTime;
    std::optional<std::chrono::system_clock::time_point> stopTime;
    
    VmState finalState{VmState::Created};
    TerminationReason terminationReason{TerminationReason::NotTerminated};
};

} // namespace fvm::domain
