#pragma once
#include "vm/contracts/IVirtualizationBackend.hpp"
#include "QemuLocator.hpp"
#include "QemuCommandBuilder.hpp"
#include "QemuProcess.hpp"
#include <unordered_map>
#include <memory>

namespace fvm::infrastructure::qemu {

class QemuBackend : public fvm::contracts::IVirtualizationBackend {
public:
    QemuBackend(std::unique_ptr<QemuLocator> locator,
                std::unique_ptr<QemuCommandBuilder> commandBuilder);
    ~QemuBackend() override;

    domain::Result<void> createVm(const domain::VmConfig& config) override;
    domain::Result<void> destroyVm(const domain::VmId& id) override;
    domain::Result<void> startVm(const domain::VmId& id) override;
    domain::Result<void> pauseVm(const domain::VmId& id) override;
    domain::Result<void> resumeVm(const domain::VmId& id) override;
    domain::Result<void> shutdownVm(const domain::VmId& id) override;
    domain::Result<void> powerOffVm(const domain::VmId& id) override;
    domain::Result<void> resetVm(const domain::VmId& id) override;
    domain::Result<domain::VmState> queryState(const domain::VmId& id) override;

private:
    std::unique_ptr<QemuLocator> locator_;
    std::unique_ptr<QemuCommandBuilder> commandBuilder_;
    
    // Phase 2B implementation detail: runtime configuration cache
    std::unordered_map<domain::VmId, domain::VmConfig> configCache_;
    
    std::unordered_map<domain::VmId, std::unique_ptr<QemuProcess>> processes_;
};

} // namespace fvm::infrastructure::qemu
