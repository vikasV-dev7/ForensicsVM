#include "VmManager.hpp"
#include <utility>

namespace fvm::management {

VmManager::VmManager(std::unique_ptr<contracts::IVmRepository> repository,
                     std::unique_ptr<contracts::IVirtualizationBackend> backend)
    : repository_(std::move(repository)), backend_(std::move(backend)) {}

domain::Result<domain::VmId> VmManager::createVm(const domain::VmConfig& config) {
    if (!config.isValid()) {
        return std::unexpected(domain::VmError::InvalidConfiguration);
    }
    
    // Check if exists
    if (repository_->findConfig(config.id).has_value()) {
        return std::unexpected(domain::VmError::DuplicateVm);
    }

    auto res = backend_->createVm(config);
    if (!res) return std::unexpected(res.error());

    auto saveRes = repository_->save(config, domain::VmState::Created);
    if (!saveRes) return std::unexpected(saveRes.error());

    return config.id;
}

domain::Result<void> VmManager::removeVm(const domain::VmId& id) {
    auto stateRes = repository_->findState(id);
    if (!stateRes) return std::unexpected(domain::VmError::VmNotFound);

    if (stateRes.value() != domain::VmState::Stopped && stateRes.value() != domain::VmState::Created && stateRes.value() != domain::VmState::Failed) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }

    auto res = backend_->destroyVm(id);
    if (!res) return res;

    return repository_->remove(id);
}

domain::Result<domain::VmConfig> VmManager::findVm(const domain::VmId& id) const {
    return repository_->findConfig(id);
}

domain::Result<std::vector<domain::VmId>> VmManager::listVms() const {
    return repository_->listAll();
}

domain::Result<void> VmManager::start(const domain::VmId& id) {
    auto stateRes = repository_->findState(id);
    if (!stateRes) return std::unexpected(domain::VmError::VmNotFound);

    if (stateRes.value() != domain::VmState::Created && stateRes.value() != domain::VmState::Stopped) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }
    
    auto configRes = repository_->findConfig(id);
    if (!configRes) return std::unexpected(domain::VmError::VmNotFound);

    auto res = backend_->startVm(id);
    if (!res) return res;

    return repository_->save(configRes.value(), domain::VmState::Running);
}

domain::Result<void> VmManager::pause(const domain::VmId& id) {
    auto stateRes = repository_->findState(id);
    if (!stateRes) return std::unexpected(domain::VmError::VmNotFound);
    if (stateRes.value() != domain::VmState::Running) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }

    auto configRes = repository_->findConfig(id);
    auto res = backend_->pauseVm(id);
    if (!res) return res;
    return repository_->save(configRes.value(), domain::VmState::Paused);
}

domain::Result<void> VmManager::resume(const domain::VmId& id) {
    auto stateRes = repository_->findState(id);
    if (!stateRes) return std::unexpected(domain::VmError::VmNotFound);
    if (stateRes.value() != domain::VmState::Paused) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }

    auto configRes = repository_->findConfig(id);
    auto res = backend_->resumeVm(id);
    if (!res) return res;
    return repository_->save(configRes.value(), domain::VmState::Running);
}

domain::Result<void> VmManager::shutdown(const domain::VmId& id) {
    auto stateRes = repository_->findState(id);
    if (!stateRes) return std::unexpected(domain::VmError::VmNotFound);
    if (stateRes.value() != domain::VmState::Running) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }

    auto configRes = repository_->findConfig(id);
    auto res = backend_->shutdownVm(id);
    if (!res) return res;
    return repository_->save(configRes.value(), domain::VmState::Stopped);
}

domain::Result<void> VmManager::powerOff(const domain::VmId& id) {
    auto stateRes = repository_->findState(id);
    if (!stateRes) return std::unexpected(domain::VmError::VmNotFound);
    
    auto configRes = repository_->findConfig(id);
    auto res = backend_->powerOffVm(id);
    if (!res) return res;
    return repository_->save(configRes.value(), domain::VmState::Stopped);
}

domain::Result<void> VmManager::reset(const domain::VmId& id) {
    auto stateRes = repository_->findState(id);
    if (!stateRes) return std::unexpected(domain::VmError::VmNotFound);
    if (stateRes.value() != domain::VmState::Running) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }

    auto res = backend_->resetVm(id);
    return res;
}

} // namespace fvm::management
