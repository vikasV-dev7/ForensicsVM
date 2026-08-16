#include "QemuBackend.hpp"

namespace fvm::infrastructure::qemu {

QemuBackend::QemuBackend(std::unique_ptr<QemuLocator> locator,
                         std::unique_ptr<QemuCommandBuilder> commandBuilder)
    : locator_(std::move(locator)), commandBuilder_(std::move(commandBuilder)) {}

QemuBackend::~QemuBackend() {
    for (auto& [id, process] : processes_) {
        if (process->isRunning()) {
            process->terminate(true);
        }
    }
}

domain::Result<void> QemuBackend::createVm(const domain::VmConfig& config) {
    configCache_.insert_or_assign(config.id, config);
    return {};
}

domain::Result<void> QemuBackend::destroyVm(const domain::VmId& id) {
    if (processes_.contains(id)) {
        if (processes_[id]->isRunning()) {
            return std::unexpected(domain::VmError::InvalidLifecycleTransition);
        }
        processes_.erase(id);
    }
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

    auto spec = commandBuilder_->build(configCache_.at(id), executableRes.value());
    
    auto process = std::make_unique<WindowsQemuProcess>();
    auto startRes = process->start(spec);
    if (!startRes) {
        return std::unexpected(domain::VmError::BackendUnavailable);
    }
    
    processes_[id] = std::move(process);
    return {};
}

domain::Result<void> QemuBackend::pauseVm(const domain::VmId&) {
    // QMP deferred to Phase 2C
    return std::unexpected(domain::VmError::OperationFailed);
}

domain::Result<void> QemuBackend::resumeVm(const domain::VmId&) {
    // QMP deferred to Phase 2C
    return std::unexpected(domain::VmError::OperationFailed);
}

domain::Result<void> QemuBackend::shutdownVm(const domain::VmId& id) {
    if (!processes_.contains(id) || !processes_.at(id)->isRunning()) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }
    // Phase 2B limitation: forced shutdown because QMP is deferred.
    auto termRes = processes_[id]->terminate(true);
    if (!termRes) {
        return std::unexpected(domain::VmError::OperationFailed);
    }
    return {};
}

domain::Result<void> QemuBackend::powerOffVm(const domain::VmId& id) {
    if (!processes_.contains(id) || !processes_.at(id)->isRunning()) {
        return std::unexpected(domain::VmError::InvalidLifecycleTransition);
    }
    auto termRes = processes_[id]->terminate(true);
    if (!termRes) {
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
    if (processes_.contains(id) && processes_.at(id)->isRunning()) {
        return domain::VmState::Running;
    }
    return domain::VmState::Stopped;
}

} // namespace fvm::infrastructure::qemu
