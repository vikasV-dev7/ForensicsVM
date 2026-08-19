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
        for (const auto& path : overlays) {
            std::error_code ec;
            std::filesystem::remove(path, ec);
        }
    }
}

std::mutex& QemuBackend::getVmLock(const domain::VmId& id) {
    if (!mutexes_.contains(id)) {
        mutexes_[id] = std::make_unique<std::mutex>();
    }
    return *mutexes_[id];
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
    std::lock_guard<std::mutex> lock(getVmLock(config.id));
    configCache_.insert_or_assign(config.id, config);
    states_[config.id] = domain::VmState::Created;
    termReasons_[config.id] = domain::TerminationReason::NotTerminated;
    return {};
}

domain::Result<void> QemuBackend::destroyVm(const domain::VmId& id) {
    std::lock_guard<std::mutex> lock(getVmLock(id));
    
    if (processes_.contains(id) && processes_[id]->isRunning()) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }
    
    qmpClients_.erase(id);
    processes_.erase(id);
    cleanupOverlays(id);
    configCache_.erase(id);
    states_.erase(id);
    termReasons_.erase(id);
    return {};
}

domain::Result<std::vector<domain::SessionEvidence>> QemuBackend::startVm(const domain::VmId& id, const std::vector<domain::EvidenceRecord>& resolvedEvidence) {
    std::lock_guard<std::mutex> lock(getVmLock(id));
    
    if (processes_.contains(id) && processes_[id]->isRunning()) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }
    if (!configCache_.contains(id)) {
        return std::unexpected(domain::VmError::VmNotFound);
    }
    
    states_[id] = domain::VmState::Starting;

    auto executableRes = locator_->discover();
    if (!executableRes) {
        states_[id] = domain::VmState::Failed;
        termReasons_[id] = domain::TerminationReason::StartupFailure;
        return std::unexpected(domain::VmError::BackendUnavailable);
    }

    std::map<std::string, std::string> overlayPaths;
    std::vector<std::string> createdOverlays;
    std::vector<domain::SessionEvidence> evidenceList;

    for (const auto& storage : configCache_.at(id).storage) {
        if (storage.access == domain::AccessMode::Overlay) {
            if (!isValidIdentifier(id.value()) || !isValidIdentifier(storage.diskId)) {
                states_[id] = domain::VmState::Failed;
                termReasons_[id] = domain::TerminationReason::StoragePreparationFailure;
                return std::unexpected(domain::VmError::OperationFailed);
            }

            // Find matching EvidenceRecord
            std::optional<domain::EvidenceRecord> matchingRecord;
            for (const auto& record : resolvedEvidence) {
                if (record.id() == storage.evidenceId) {
                    matchingRecord = record;
                    break;
                }
            }
            if (!matchingRecord) {
                states_[id] = domain::VmState::Failed;
                termReasons_[id] = domain::TerminationReason::StoragePreparationFailure;
                return std::unexpected(domain::VmError::OperationFailed);
            }

            std::filesystem::path tempDir = std::filesystem::canonical(std::filesystem::temp_directory_path());
            std::string safeId = std::to_string(std::hash<std::string>{}(id.value()));
            std::string safeDisk = std::to_string(std::hash<std::string>{}(storage.diskId));
            
            std::string overlayName = "fvm-overlay-" + safeId + "-" + safeDisk + ".qcow2";
            std::filesystem::path overlayPath = std::filesystem::weakly_canonical(tempDir / overlayName);
            
            if (overlayPath.parent_path() != tempDir) {
                states_[id] = domain::VmState::Failed;
                termReasons_[id] = domain::TerminationReason::StoragePreparationFailure;
                return std::unexpected(domain::VmError::OperationFailed);
            }
            
            auto imgRes = imageTool_->createOverlay(matchingRecord->path(), matchingRecord->format(), overlayPath);
            if (!imgRes) {
                std::error_code ec;
                std::filesystem::remove(overlayPath, ec);
                for (const auto& path : createdOverlays) {
                    std::filesystem::remove(path, ec);
                }
                states_[id] = domain::VmState::Failed;
                termReasons_[id] = domain::TerminationReason::StoragePreparationFailure;
                return std::unexpected(domain::VmError::OperationFailed);
            }
            overlayPaths[storage.diskId] = overlayPath.string();
            createdOverlays.push_back(overlayPath.string());
            
            evidenceList.push_back({
                storage.diskId,
                matchingRecord->sha256(),
                domain::AccessMode::Overlay,
                overlayPath.string()
            });
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
        states_[id] = domain::VmState::Failed;
        termReasons_[id] = domain::TerminationReason::StartupFailure;
        return std::unexpected(domain::VmError::BackendUnavailable);
    }

    auto qmpClient = std::make_unique<QmpClient>(spec.qmpPipeName);
    auto connectRes = qmpClient->connect(std::chrono::seconds(5));
    if (!connectRes) {
        process->terminate(true);
        cleanupOverlays(id);
        states_[id] = domain::VmState::Failed;
        termReasons_[id] = domain::TerminationReason::StartupFailure;
        return std::unexpected(domain::VmError::BackendUnavailable);
    }

    processes_[id] = std::move(process);
    qmpClients_[id] = std::move(qmpClient);
    
    states_[id] = domain::VmState::Running;
    termReasons_[id] = domain::TerminationReason::NotTerminated;
    
    return evidenceList;
}

domain::Result<void> QemuBackend::pauseVm(const domain::VmId& id) {
    std::lock_guard<std::mutex> lock(getVmLock(id));
    if (!qmpClients_.contains(id) || !processes_.contains(id) || !processes_.at(id)->isRunning()) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }
    auto res = qmpClients_.at(id)->execute("stop");
    if (!res) return std::unexpected(domain::VmError::OperationFailed);
    states_[id] = domain::VmState::Paused;
    return {};
}

domain::Result<void> QemuBackend::resumeVm(const domain::VmId& id) {
    std::lock_guard<std::mutex> lock(getVmLock(id));
    if (!qmpClients_.contains(id) || !processes_.contains(id) || !processes_.at(id)->isRunning()) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }
    auto res = qmpClients_.at(id)->execute("cont");
    if (!res) return std::unexpected(domain::VmError::OperationFailed);
    states_[id] = domain::VmState::Running;
    return {};
}

domain::Result<void> QemuBackend::shutdownVm(const domain::VmId& id) {
    std::lock_guard<std::mutex> lock(getVmLock(id));
    if (!qmpClients_.contains(id) || !processes_.contains(id) || !processes_.at(id)->isRunning()) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }
    auto res = qmpClients_.at(id)->execute("system_powerdown");
    if (!res) return std::unexpected(domain::VmError::OperationFailed);
    states_[id] = domain::VmState::ShuttingDown;
    return {};
}

domain::Result<void> QemuBackend::powerOffVm(const domain::VmId& id) {
    std::lock_guard<std::mutex> lock(getVmLock(id));
    if (!processes_.contains(id) || !processes_.at(id)->isRunning()) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }
    
    if (qmpClients_.contains(id)) {
        qmpClients_.at(id)->execute("quit", nullptr, std::chrono::seconds(1));
        qmpClients_.at(id)->disconnect();
    }

    processes_[id]->terminate(true);
    cleanupOverlays(id);
    
    states_[id] = domain::VmState::Failed; // Or Stopped, but Failed(UserPowerOff) is safer. Let's use Stopped.
    states_[id] = domain::VmState::Stopped;
    termReasons_[id] = domain::TerminationReason::UserPowerOff;
    
    return {};
}

domain::Result<void> QemuBackend::resetVm(const domain::VmId&) {
    return std::unexpected(domain::VmError::OperationFailed);
}

domain::Result<contracts::RuntimeState> QemuBackend::queryState(const domain::VmId& id) {
    std::lock_guard<std::mutex> lock(getVmLock(id));
    
    if (!configCache_.contains(id)) {
        return std::unexpected(domain::VmError::VmNotFound);
    }
    
    if (!states_.contains(id)) {
        return contracts::RuntimeState{domain::VmState::Created, domain::TerminationReason::NotTerminated};
    }

    auto currentState = states_[id];
    auto currentReason = termReasons_[id];

    // If already terminated, just return
    if (currentState == domain::VmState::Stopped || currentState == domain::VmState::Failed) {
        return contracts::RuntimeState{currentState, currentReason};
    }

    // Still active state. We must reconcile.
    if (!processes_.contains(id)) {
        // Should never happen, but handle it.
        states_[id] = domain::VmState::Failed;
        termReasons_[id] = domain::TerminationReason::ProcessCrashed;
        return contracts::RuntimeState{states_[id], termReasons_[id]};
    }

    auto* process = processes_[id].get();
    auto* qmp = qmpClients_.contains(id) ? qmpClients_[id].get() : nullptr;

    bool processAlive = process->isRunning();
    
    bool shutdownEvent = false;

    if (qmp) {
        auto events = qmp->pollEvents();
        for (const auto& ev : events) {
            if (ev.contains("event")) {
                if (ev["event"] == "SHUTDOWN") shutdownEvent = true;
            }
        }
    }

    if (!processAlive) {
        // OS process confirmed dead! Overlay cleanup permitted.
        cleanupOverlays(id);

        if (shutdownEvent || currentState == domain::VmState::ShuttingDown) {
            states_[id] = domain::VmState::Stopped;
            termReasons_[id] = domain::TerminationReason::GracefulShutdown;
        } else {
            states_[id] = domain::VmState::Failed;
            termReasons_[id] = domain::TerminationReason::ProcessCrashed;
        }
        return contracts::RuntimeState{states_[id], termReasons_[id]};
    }

    // Process is alive. Verify QMP health.
    if (qmp) {
        auto res = qmp->execute("query-status");
        if (!res) {
            // Infrastructure failure: QMP disconnected while QEMU alive.
            // Force termination to regain determinism.
            process->terminate(true);
            cleanupOverlays(id);
            states_[id] = domain::VmState::Failed;
            termReasons_[id] = domain::TerminationReason::QmpDisconnected;
            return contracts::RuntimeState{states_[id], termReasons_[id]};
        } else {
            // QMP healthy. Update state from query-status if not shutting down.
            if (currentState != domain::VmState::ShuttingDown) {
                if (res->contains("return") && (*res)["return"].contains("status")) {
                    std::string status = (*res)["return"]["status"];
                    if (status == "running") states_[id] = domain::VmState::Running;
                    else if (status == "paused") states_[id] = domain::VmState::Paused;
                }
            }
        }
    }

    return contracts::RuntimeState{states_[id], termReasons_[id]};
}

} // namespace fvm::infrastructure::qemu
