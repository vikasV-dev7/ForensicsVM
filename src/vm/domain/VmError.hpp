#pragma once
#include <expected>

namespace fvm::domain {

enum class VmError {
    InvalidConfiguration,
    VmNotFound,
    DuplicateVm,
    InvalidLifecycleTransition,
    BackendUnavailable,
    OperationFailed,
    EvidenceIntegrityFailure
};

template <typename T>
using Result = std::expected<T, VmError>;

} // namespace fvm::domain
