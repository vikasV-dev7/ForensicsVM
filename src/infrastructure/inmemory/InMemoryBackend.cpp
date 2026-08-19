#include "InMemoryBackend.hpp"
#include "vm/domain/VmError.hpp"
#include <vector>

namespace fvm::infrastructure::inmemory {

domain::Result<void> InMemoryBackend::createVm(const domain::VmConfig& config) {
    if (states_.contains(config.id)) return std::unexpected(domain::VmError::DuplicateVm);
    states_[config.id] = domain::VmState::Stopped;
    return {};
}

domain::Result<void> InMemoryBackend::destroyVm(const domain::VmId& id) {
    states_.erase(id);
    return {};
}

domain::Result<domain::AcquisitionResult> InMemoryBackend::acquireMemory(const domain::VmId& id, std::chrono::milliseconds timeout) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!states_.contains(id)) return std::unexpected(domain::VmError::VmNotFound);
    auto st = states_[id];
    if (st == domain::VmState::Created || st == domain::VmState::Failed || st == domain::VmState::Stopped) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }
    
    // Simulate successful acquisition for testing
    return domain::AcquisitionResult{ "C:\\temp\\fvm-inmemory-memdump.elf", domain::DiskFormat::Elf };
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
