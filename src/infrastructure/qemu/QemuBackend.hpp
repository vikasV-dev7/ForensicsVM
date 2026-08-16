#pragma once
#include "vm/contracts/IVirtualizationBackend.hpp"
#include "QemuLocator.hpp"
#include "QemuCommandBuilder.hpp"
#include "QemuProcess.hpp"
#include "QmpClient.hpp"
#include <unordered_map>
#include <memory>

#include "image/IQemuImageTool.hpp"

namespace fvm::infrastructure::qemu {

class QemuBackend : public fvm::contracts::IVirtualizationBackend {
public:
    QemuBackend(std::unique_ptr<QemuLocator> locator,
                std::unique_ptr<QemuCommandBuilder> commandBuilder,
                std::unique_ptr<image::IQemuImageTool> imageTool);
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
    std::unique_ptr<image::IQemuImageTool> imageTool_;
    
    // Phase 2B implementation detail: runtime configuration cache
    std::unordered_map<domain::VmId, domain::VmConfig> configCache_;
    
    std::unordered_map<domain::VmId, std::unique_ptr<QemuProcess>> processes_;
    std::unordered_map<domain::VmId, std::unique_ptr<QmpClient>> qmpClients_;
    std::unordered_map<domain::VmId, std::vector<std::string>> overlayCleanupTracker_;

    void cleanupOverlays(const domain::VmId& id);
};

} // namespace fvm::infrastructure::qemu
