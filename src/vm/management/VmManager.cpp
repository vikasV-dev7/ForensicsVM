#include "VmManager.hpp"
#include <utility>
#include <chrono>
#include <random>

namespace fvm::management {

namespace {
std::string generateSessionId(const std::string& vmId) {
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::random_device rd;
    return "sess-" + vmId + "-" + std::to_string(now) + "-" + std::to_string(rd());
}
} // namespace

VmManager::VmManager(std::unique_ptr<contracts::IVmRepository> repository,
                     std::unique_ptr<contracts::IVirtualizationBackend> backend)
    : repository_(std::move(repository)), backend_(std::move(backend)) {}

domain::Result<domain::VmId> VmManager::createVm(const domain::VmConfig& config) {
    if (!config.isValid()) {
        return std::unexpected(domain::VmError::InvalidConfiguration);
    }
    
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
    auto stateRes = queryState(id);
    if (!stateRes) return std::unexpected(stateRes.error());

    auto state = stateRes->state;
    if (state != domain::VmState::Stopped && state != domain::VmState::Created && state != domain::VmState::Failed) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }

    auto res = backend_->destroyVm(id);
    if (!res) return res;

    sessions_.erase(id);
    return repository_->remove(id);
}

domain::Result<domain::VmConfig> VmManager::findVm(const domain::VmId& id) const {
    return repository_->findConfig(id);
}

domain::Result<std::vector<domain::VmId>> VmManager::listVms() const {
    return repository_->listAll();
}

domain::Result<contracts::RuntimeState> VmManager::queryState(const domain::VmId& id) {
    auto res = backend_->queryState(id);
    if (!res) return res;

    // Session finalization logic
    if (sessions_.contains(id)) {
        auto& session = sessions_.at(id);
        if (!session.stopTime.has_value()) {
            if (res->state == domain::VmState::Stopped || res->state == domain::VmState::Failed) {
                session.stopTime = std::chrono::system_clock::now();
                session.finalState = res->state;
                session.terminationReason = res->reason;
            }
        }
    }

    return res;
}

domain::Result<std::string> VmManager::start(const domain::VmId& id) {
    auto stateRes = queryState(id);
    if (!stateRes) return std::unexpected(domain::VmError::VmNotFound);

    if (stateRes->state != domain::VmState::Created && stateRes->state != domain::VmState::Stopped && stateRes->state != domain::VmState::Failed) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }
    
    auto configRes = repository_->findConfig(id);
    if (!configRes) return std::unexpected(domain::VmError::VmNotFound);

    auto startRes = backend_->startVm(id);
    if (!startRes) return std::unexpected(startRes.error());

    domain::ExecutionSession session{
        generateSessionId(id.value()),
        id,
        startRes.value(),
        std::chrono::system_clock::now(),
        std::nullopt,
        domain::VmState::Running,
        domain::TerminationReason::NotTerminated
    };
    
    sessions_.insert_or_assign(id, session);
    return session.sessionId;
}

domain::Result<void> VmManager::pause(const domain::VmId& id) {
    auto stateRes = queryState(id);
    if (!stateRes) return std::unexpected(stateRes.error());
    if (stateRes->state != domain::VmState::Running) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }

    return backend_->pauseVm(id);
}

domain::Result<void> VmManager::resume(const domain::VmId& id) {
    auto stateRes = queryState(id);
    if (!stateRes) return std::unexpected(stateRes.error());
    if (stateRes->state != domain::VmState::Paused) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }

    return backend_->resumeVm(id);
}

domain::Result<void> VmManager::shutdown(const domain::VmId& id) {
    auto stateRes = queryState(id);
    if (!stateRes) return std::unexpected(stateRes.error());
    if (stateRes->state != domain::VmState::Running) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }

    return backend_->shutdownVm(id);
}

domain::Result<void> VmManager::powerOff(const domain::VmId& id) {
    auto stateRes = queryState(id);
    if (!stateRes) return std::unexpected(stateRes.error());
    
    auto res = backend_->powerOffVm(id);
    if (res) {
        // Trigger reconciliation to finalize session immediately
        queryState(id);
    }
    return res;
}

domain::Result<void> VmManager::reset(const domain::VmId& id) {
    auto stateRes = queryState(id);
    if (!stateRes) return std::unexpected(stateRes.error());
    if (stateRes->state != domain::VmState::Running) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }

    return backend_->resetVm(id);
}

} // namespace fvm::management
