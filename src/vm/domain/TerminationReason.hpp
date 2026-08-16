#pragma once

namespace fvm::domain {

enum class TerminationReason {
    NotTerminated,
    GracefulShutdown,
    UserPowerOff,
    ProcessCrashed,
    QmpDisconnected,
    StartupFailure,
    StoragePreparationFailure,
    ShutdownTimeout
};

} // namespace fvm::domain
