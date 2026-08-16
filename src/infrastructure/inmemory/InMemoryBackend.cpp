#include "InMemoryBackend.hpp"

namespace fvm::infrastructure::inmemory {

domain::Result<void> InMemoryBackend::createVm(const domain::VmConfig& config) {
    if (activeVms_.contains(config.id)) return std::unexpected(domain::VmError::DuplicateVm);
    return {}; // Assume creation successful
}

domain::Result<void> InMemoryBackend::destroyVm(const domain::VmId& id) {
    activeVms_.erase(id); // Doesn't fail if not running
    return {};
}

domain::Result<void> InMemoryBackend::startVm(const domain::VmId& id) {
    activeVms_.insert(id);
    return {};
}

domain::Result<void> InMemoryBackend::pauseVm(const domain::VmId&) {
    return {}; // Simplification
}

domain::Result<void> InMemoryBackend::resumeVm(const domain::VmId&) {
    return {}; // Simplification
}

domain::Result<void> InMemoryBackend::shutdownVm(const domain::VmId& id) {
    activeVms_.erase(id);
    return {};
}

domain::Result<void> InMemoryBackend::powerOffVm(const domain::VmId& id) {
    activeVms_.erase(id);
    return {};
}

domain::Result<void> InMemoryBackend::resetVm(const domain::VmId&) {
    return {}; // Simplification
}

domain::Result<domain::VmState> InMemoryBackend::queryState(const domain::VmId& id) {
    if (activeVms_.contains(id)) return domain::VmState::Running;
    return domain::VmState::Stopped; // Simplification
}

} // namespace fvm::infrastructure::inmemory
