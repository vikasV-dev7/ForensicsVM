#pragma once

namespace fvm::domain {

enum class VmState {
    Created,
    Starting,
    Running,
    Paused,
    Stopping,
    Stopped,
    Failed
};

} // namespace fvm::domain
