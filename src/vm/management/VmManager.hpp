#pragma once
#include "vm/contracts/IVmRepository.hpp"
#include "vm/contracts/IVirtualizationBackend.hpp"
#include "vm/domain/VmId.hpp"
#include "vm/domain/VmConfig.hpp"
#include "vm/domain/VmState.hpp"
#include "vm/domain/VmError.hpp"
#include <memory>
#include <vector>

namespace fvm::management {

class VmManager {
    std::unique_ptr<contracts::IVmRepository> repository_;
    std::unique_ptr<contracts::IVirtualizationBackend> backend_;

public:
    VmManager(std::unique_ptr<contracts::IVmRepository> repository,
              std::unique_ptr<contracts::IVirtualizationBackend> backend);

    domain::Result<domain::VmId> createVm(const domain::VmConfig& config);
    domain::Result<void> removeVm(const domain::VmId& id);

    domain::Result<domain::VmConfig> findVm(const domain::VmId& id) const;
    domain::Result<std::vector<domain::VmId>> listVms() const;

    domain::Result<void> start(const domain::VmId& id);
    domain::Result<void> pause(const domain::VmId& id);
    domain::Result<void> resume(const domain::VmId& id);
    domain::Result<void> shutdown(const domain::VmId& id);
    domain::Result<void> powerOff(const domain::VmId& id);
    domain::Result<void> reset(const domain::VmId& id);
};

} // namespace fvm::management
