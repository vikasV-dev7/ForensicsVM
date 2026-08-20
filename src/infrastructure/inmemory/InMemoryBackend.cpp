#include "InMemoryBackend.hpp"
#include "vm/domain/VmError.hpp"
#include <vector>
#include <mutex>
#include <fstream>
#include <filesystem>

namespace fvm::infrastructure::inmemory {

domain::Result<void> InMemoryBackend::createVm(const domain::VmConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (states_.contains(config.id)) return std::unexpected(domain::VmError::DuplicateVm);
    states_[config.id] = domain::VmState::Stopped;
    return {};
}

domain::Result<void> InMemoryBackend::destroyVm(const domain::VmId& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    states_.erase(id);
    return {};
}


domain::Result<domain::AcquisitionResult> InMemoryBackend::acquireMemory(const domain::VmId& id, std::chrono::milliseconds timeout, std::stop_token stoken) {
    (void)timeout;
    std::lock_guard<std::mutex> lock(mutex_);
    if (!states_.contains(id)) return std::unexpected(domain::VmError::VmNotFound);
    auto st = states_[id];
    if (st != domain::VmState::Running) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }
    if (stoken.stop_requested()) return std::unexpected(domain::VmError::OperationFailed);
    
    // Simulate successful acquisition for testing
    std::filesystem::path tempPath = std::filesystem::current_path() / "fvm-inmemory-memdump.elf";
    {
        std::ofstream dummy(tempPath);
        dummy << "memdump";
    }
    return domain::AcquisitionResult{ tempPath.string(), domain::DiskFormat::Elf };
}

domain::Result<domain::AcquisitionResult> InMemoryBackend::acquireDiskDelta(const domain::VmId& id, const std::string& diskId, std::chrono::milliseconds timeout, std::stop_token stoken) {
    (void)timeout;
    std::lock_guard<std::mutex> lock(mutex_);
    if (!states_.contains(id)) return std::unexpected(domain::VmError::VmNotFound);
    auto st = states_[id];
    if (st == domain::VmState::Created || st == domain::VmState::Failed || st == domain::VmState::Stopped) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }
    if (stoken.stop_requested()) return std::unexpected(domain::VmError::OperationFailed);
    
    // Simulate successful acquisition for testing
    std::filesystem::path tempPath = std::filesystem::current_path() / ("fvm-inmemory-diskdelta-" + diskId + ".qcow2");
    {
        std::ofstream dummy(tempPath);
        dummy << "diskdelta";
    }
    return domain::AcquisitionResult{ tempPath.string(), domain::DiskFormat::Qcow2 };
}

domain::Result<std::vector<domain::SessionEvidence>> InMemoryBackend::startVm(const domain::VmId& id, const std::vector<domain::EvidenceRecord>& /*resolvedEvidence*/) {
    if (!states_.contains(id)) return std::unexpected(domain::VmError::VmNotFound);
    states_[id] = domain::VmState::Running;
    return std::vector<domain::SessionEvidence>{};
}

domain::Result<void> InMemoryBackend::pauseVm(const domain::VmId& id) {
    if (states_.contains(id)) states_[id] = domain::VmState::Paused;
    return {};
}

domain::Result<void> InMemoryBackend::resumeVm(const domain::VmId& id) {
    if (states_.contains(id)) states_[id] = domain::VmState::Running;
    return {};
}

domain::Result<void> InMemoryBackend::shutdownVm(const domain::VmId& id) {
    if (states_.contains(id)) states_[id] = domain::VmState::Stopped;
    return {};
}

domain::Result<void> InMemoryBackend::powerOffVm(const domain::VmId& id) {
    if (states_.contains(id)) states_[id] = domain::VmState::Stopped;
    return {};
}

domain::Result<void> InMemoryBackend::resetVm(const domain::VmId& id) {
    (void)id;
    return {}; // Simplification
}

domain::Result<contracts::RuntimeState> InMemoryBackend::queryState(const domain::VmId& id) {
    if (!states_.contains(id)) return std::unexpected(domain::VmError::VmNotFound);
    return contracts::RuntimeState{states_[id], domain::TerminationReason::NotTerminated};
}

} // namespace fvm::infrastructure::inmemory
