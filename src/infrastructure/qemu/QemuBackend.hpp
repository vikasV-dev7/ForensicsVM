#pragma once
#include "vm/contracts/IVirtualizationBackend.hpp"
#include "QemuLocator.hpp"
#include "QemuCommandBuilder.hpp"
#include "QemuProcess.hpp"
#include "QmpClient.hpp"
#include <unordered_map>
#include <memory>
#include <mutex>

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
    
    domain::Result<std::vector<domain::SessionEvidence>> startVm(const domain::VmId& id, const std::vector<domain::EvidenceRecord>& resolvedEvidence) override;
    domain::Result<void> pauseVm(const domain::VmId& id) override;
    domain::Result<void> resumeVm(const domain::VmId& id) override;
    domain::Result<void> shutdownVm(const domain::VmId& id) override;
    domain::Result<void> powerOffVm(const domain::VmId& id) override;
    domain::Result<void> resetVm(const domain::VmId& id) override;

    domain::Result<domain::AcquisitionResult> acquireMemory(const domain::VmId& id, std::chrono::milliseconds timeout) override;
    domain::Result<domain::AcquisitionResult> acquireDiskDelta(const domain::VmId& id, const std::string& diskId, std::chrono::milliseconds timeout) override;
    
    domain::Result<contracts::RuntimeState> queryState(const domain::VmId& id) override;

private:
    std::unique_ptr<QemuLocator> locator_;
    std::unique_ptr<QemuCommandBuilder> commandBuilder_;
    std::unique_ptr<image::IQemuImageTool> imageTool_;
    
    // Per-VM synchronization
    std::unordered_map<domain::VmId, std::unique_ptr<std::mutex>> mutexes_;
    
    // Infrastructure reality tracking
    std::unordered_map<domain::VmId, domain::VmState> states_;
    std::unordered_map<domain::VmId, domain::TerminationReason> termReasons_;
    
    std::unordered_map<domain::VmId, domain::VmConfig> configCache_;
    std::unordered_map<domain::VmId, std::unique_ptr<QemuProcess>> processes_;
    std::unordered_map<domain::VmId, std::unique_ptr<QmpClient>> qmpClients_;
    std::unordered_map<domain::VmId, std::vector<std::string>> overlayCleanupTracker_;
    std::unordered_map<domain::VmId, std::map<std::string, std::pair<std::filesystem::path, domain::DiskFormat>>> activeStorageEvidence_;

    void cleanupOverlays(const domain::VmId& id);
    std::mutex& getVmLock(const domain::VmId& id);
};

} // namespace fvm::infrastructure::qemu
