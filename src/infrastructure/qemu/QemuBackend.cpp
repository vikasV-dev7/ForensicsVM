#include "QemuBackend.hpp"
#include <iostream>

namespace fvm::infrastructure::qemu {

QemuBackend::QemuBackend(std::unique_ptr<QemuLocator> locator,
                         std::unique_ptr<QemuCommandBuilder> commandBuilder)
    : locator_(std::move(locator)), commandBuilder_(std::move(commandBuilder)) {}

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

    auto spec = commandBuilder_->build(id, configCache_.at(id), executableRes.value());
    
    auto process = std::make_unique<WindowsQemuProcess>();
    auto startRes = process->start(spec);
    if (!startRes) {
        return std::unexpected(domain::VmError::BackendUnavailable);
    }

    auto qmpClient = std::make_unique<QmpClient>(spec.qmpPipeName);
    auto connectRes = qmpClient->connect(std::chrono::seconds(5));
    if (!connectRes) {
        process->terminate(true);
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
