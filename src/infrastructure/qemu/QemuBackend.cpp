#include "QemuBackend.hpp"
#include <iostream>
#include <system_error>

namespace fvm::infrastructure::qemu {

namespace {
bool isValidIdentifier(const std::string& id) {
    if (id.empty() || id.length() > 255) return false;
    if (id == "." || id == "..") return false;
    if (id.find('/') != std::string::npos || id.find('\\') != std::string::npos) return false;
    if (id.find_first_of("<>:\"|?*") != std::string::npos) return false;
    for (char c : id) {
        if (c >= 0 && c < 32) return false;
    }
    return true;
}
} // namespace

QemuBackend::QemuBackend(std::unique_ptr<QemuLocator> locator,
                         std::unique_ptr<QemuCommandBuilder> commandBuilder,
                         std::unique_ptr<image::IQemuImageTool> imageTool)
    : locator_(std::move(locator)), commandBuilder_(std::move(commandBuilder)), imageTool_(std::move(imageTool)) {}

QemuBackend::~QemuBackend() {
    for (auto& [id, client] : qmpClients_) {
        if (client) {
            client->execute("quit", nullptr, std::chrono::seconds(1));
            client->disconnect();
        }
    }
    for (auto& [id, process] : processes_) {
        if (process->isRunning()) {
            process->terminate(true);
        }
    }
    for (auto const& [id, overlays] : overlayCleanupTracker_) {
        // Since we are in the destructor, we just remove the files.
        for (const auto& path : overlays) {
            std::error_code ec;
            std::filesystem::remove(path, ec);
        }
    }
}

void QemuBackend::cleanupOverlays(const domain::VmId& id) {
    if (overlayCleanupTracker_.contains(id)) {
        for (const auto& path : overlayCleanupTracker_[id]) {
            std::error_code ec;
            std::filesystem::remove(path, ec);
        }
        overlayCleanupTracker_.erase(id);
    }
}

domain::Result<void> QemuBackend::createVm(const domain::VmConfig& config) {
    configCache_.insert_or_assign(config.id, config);
    return {};
}

domain::Result<void> QemuBackend::destroyVm(const domain::VmId& id) {
    if (qmpClients_.contains(id)) {
        if (processes_.contains(id) && processes_[id]->isRunning()) {
            return std::unexpected(domain::VmError::InvalidLifecycleTransition);
        }
        qmpClients_.erase(id);
    }
    if (processes_.contains(id)) {
        if (processes_[id]->isRunning()) {
            return std::unexpected(domain::VmError::InvalidLifecycleTransition);
        }
        processes_.erase(id);
    }
    cleanupOverlays(id);
    configCache_.erase(id);
    return {};
}

domain::Result<void> QemuBackend::startVm(const domain::VmId& id) {
    if (processes_.contains(id) && processes_[id]->isRunning()) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }
    
    if (!configCache_.contains(id)) {
        return std::unexpected(domain::VmError::VmNotFound);
    }

    auto executableRes = locator_->discover();
    if (!executableRes) {
        return std::unexpected(domain::VmError::BackendUnavailable);
    }

    std::map<std::string, std::string> overlayPaths;
    std::vector<std::string> createdOverlays;

    for (const auto& storage : configCache_.at(id).storage) {
        if (storage.access == domain::AccessMode::Overlay) {
            if (!isValidIdentifier(id.value()) || !isValidIdentifier(storage.diskId)) {
                return std::unexpected(domain::VmError::OperationFailed);
            }

            std::filesystem::path tempDir = std::filesystem::canonical(std::filesystem::temp_directory_path());
            
            // Generate deterministic safe representation:
            // Since we validated characters, we can concatenate safely. We also hash for extra safety against very long filenames.
            std::string safeId = std::to_string(std::hash<std::string>{}(id.value()));
            std::string safeDisk = std::to_string(std::hash<std::string>{}(storage.diskId));
            
            std::string overlayName = "fvm-overlay-" + safeId + "-" + safeDisk + ".qcow2";
            std::filesystem::path overlayPath = std::filesystem::weakly_canonical(tempDir / overlayName);
            
            // Verify strict containment
            if (overlayPath.parent_path() != tempDir) {
                return std::unexpected(domain::VmError::OperationFailed);
            }
            
            auto imgRes = imageTool_->createOverlay(storage.evidence.path(), storage.evidence.format(), overlayPath);
            if (!imgRes) {
                // Cleanup on failure including the partial file
                std::error_code ec;
                std::filesystem::remove(overlayPath, ec);
                
                for (const auto& path : createdOverlays) {
                    std::error_code ec2;
                    std::filesystem::remove(path, ec2);
                }
                return std::unexpected(domain::VmError::OperationFailed);
            }
            overlayPaths[storage.diskId] = overlayPath.string();
            createdOverlays.push_back(overlayPath.string());
        }
    }

    if (!createdOverlays.empty()) {
        overlayCleanupTracker_[id] = createdOverlays;
    }

    auto spec = commandBuilder_->build(id, configCache_.at(id), executableRes.value(), overlayPaths);
    
    auto process = std::make_unique<WindowsQemuProcess>();
    auto startRes = process->start(spec);
    if (!startRes) {
        cleanupOverlays(id);
        return std::unexpected(domain::VmError::BackendUnavailable);
    }

    auto qmpClient = std::make_unique<QmpClient>(spec.qmpPipeName);
    auto connectRes = qmpClient->connect(std::chrono::seconds(5));
    if (!connectRes) {
        process->terminate(true);
        cleanupOverlays(id);
        return std::unexpected(domain::VmError::BackendUnavailable);
    }

    processes_[id] = std::move(process);
    qmpClients_[id] = std::move(qmpClient);
    return {};
}

domain::Result<void> QemuBackend::pauseVm(const domain::VmId& id) {
    if (!qmpClients_.contains(id) || !processes_.contains(id) || !processes_.at(id)->isRunning()) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }
    auto res = qmpClients_.at(id)->execute("stop");
    if (!res) {
        return std::unexpected(domain::VmError::OperationFailed);
    }
    return {};
}

domain::Result<void> QemuBackend::resumeVm(const domain::VmId& id) {
    if (!qmpClients_.contains(id) || !processes_.contains(id) || !processes_.at(id)->isRunning()) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }
    auto res = qmpClients_.at(id)->execute("cont");
    if (!res) {
        std::cerr << "QMP cont failed with error: " << static_cast<int>(res.error()) << "\n";
        return std::unexpected(domain::VmError::OperationFailed);
    }
    return {};
}

domain::Result<void> QemuBackend::shutdownVm(const domain::VmId& id) {
    if (!qmpClients_.contains(id) || !processes_.contains(id) || !processes_.at(id)->isRunning()) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }
    auto res = qmpClients_.at(id)->execute("system_powerdown");
    if (!res) {
        std::cerr << "QMP system_powerdown failed with error: " << static_cast<int>(res.error()) << "\n";
        return std::unexpected(domain::VmError::OperationFailed);
    }
    return {};
}

domain::Result<void> QemuBackend::powerOffVm(const domain::VmId& id) {
    if (!processes_.contains(id) || !processes_.at(id)->isRunning()) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }
    
    if (qmpClients_.contains(id)) {
        auto res = qmpClients_.at(id)->execute("quit", nullptr, std::chrono::seconds(1));
        qmpClients_.at(id)->disconnect();
        qmpClients_.erase(id);
    }

    auto termRes = processes_[id]->terminate(true);
    cleanupOverlays(id);
    
    if (!termRes && termRes.error() != QemuProcess::Error::NotRunning) {
        return std::unexpected(domain::VmError::OperationFailed);
    }
    return {};
}

domain::Result<void> QemuBackend::resetVm(const domain::VmId&) {
    // QMP deferred to Phase 2C
    return std::unexpected(domain::VmError::OperationFailed);
}

domain::Result<domain::VmState> QemuBackend::queryState(const domain::VmId& id) {
    if (!configCache_.contains(id)) {
        return std::unexpected(domain::VmError::VmNotFound);
    }
    
    if (!processes_.contains(id) || !processes_.at(id)->isRunning()) {
        return domain::VmState::Stopped;
    }

    if (!qmpClients_.contains(id)) {
        return std::unexpected(domain::VmError::BackendUnavailable);
    }

    auto res = qmpClients_.at(id)->execute("query-status");
    if (!res) {
        return std::unexpected(domain::VmError::BackendUnavailable);
    }

    if (res->contains("return") && (*res)["return"].contains("status")) {
        std::string status = (*res)["return"]["status"];
        if (status == "running") {
            return domain::VmState::Running;
        } else if (status == "paused") {
            return domain::VmState::Paused;
        } else {
            return domain::VmState::Stopped; // E.g. prelaunch, internal-error
        }
    }

    return std::unexpected(domain::VmError::BackendUnavailable);
}

} // namespace fvm::infrastructure::qemu
