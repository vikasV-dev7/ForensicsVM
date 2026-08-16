#pragma once
#include "vm/contracts/IVirtualizationBackend.hpp"
#include <unordered_set>

namespace fvm::infrastructure::inmemory {

class InMemoryBackend : public fvm::contracts::IVirtualizationBackend {
    std::unordered_set<domain::VmId> activeVms_;

public:
    domain::Result<void> createVm(const domain::VmConfig& config) override;
    domain::Result<void> destroyVm(const domain::VmId& id) override;
    domain::Result<void> startVm(const domain::VmId& id) override;
    domain::Result<void> pauseVm(const domain::VmId& id) override;
    domain::Result<void> resumeVm(const domain::VmId& id) override;
    domain::Result<void> shutdownVm(const domain::VmId& id) override;
    domain::Result<void> powerOffVm(const domain::VmId& id) override;
    domain::Result<void> resetVm(const domain::VmId& id) override;
    domain::Result<domain::VmState> queryState(const domain::VmId& id) override;
};

} // namespace fvm::infrastructure::inmemory
