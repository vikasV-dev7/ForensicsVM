#pragma once

namespace fvm::domain {

enum class VmState {
    Created,
    Starting,
    Running,
    Paused,
    ShuttingDown,
    Stopped,
    Failed
};

} // namespace fvm::domain
