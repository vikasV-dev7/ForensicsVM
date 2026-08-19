#pragma once

namespace fvm::core::application::domain {

enum class OperationType {
    AcquireMemory,
    AcquireDiskDelta,
    ImportEvidence,
    LaunchSession,
    StopSession,
    Unknown
};

} // namespace fvm::core::application::domain
