#pragma once

namespace fvm::core::application::domain {

enum class OperationState {
    Queued,
    Starting,
    Running,
    Validating,
    Hashing,
    Finalizing,
    Completed,
    Failed,
    Cancelling,
    Cancelled
};

} // namespace fvm::core::application::domain
