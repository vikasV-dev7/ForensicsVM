#pragma once
#include "vm/domain/VmId.hpp"
#include "vm/domain/VmConfig.hpp"
#include "vm/domain/VmState.hpp"
#include "vm/domain/VmError.hpp"

namespace fvm::contracts {

class IVirtualizationBackend {
public:
    virtual ~IVirtualizationBackend() = default;

    virtual domain::Result<void> createVm(const domain::VmConfig& config) = 0;
    virtual domain::Result<void> destroyVm(const domain::VmId& id) = 0;
    
    virtual domain::Result<void> startVm(const domain::VmId& id) = 0;
    virtual domain::Result<void> pauseVm(const domain::VmId& id) = 0;
    virtual domain::Result<void> resumeVm(const domain::VmId& id) = 0;
    virtual domain::Result<void> shutdownVm(const domain::VmId& id) = 0;
    virtual domain::Result<void> powerOffVm(const domain::VmId& id) = 0;
    virtual domain::Result<void> resetVm(const domain::VmId& id) = 0;
    
    virtual domain::Result<domain::VmState> queryState(const domain::VmId& id) = 0;
};

} // namespace fvm::contracts
