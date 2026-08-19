#pragma once
#include "vm/contracts/IVmRepository.hpp"
#include "vm/contracts/IVirtualizationBackend.hpp"
#include "vm/domain/VmId.hpp"
#include "vm/domain/VmConfig.hpp"
#include "vm/domain/VmState.hpp"
#include "vm/domain/VmError.hpp"
#include "vm/domain/ExecutionSession.hpp"
#include "vm/management/EvidenceRegistry.hpp"
#include <memory>
#include <vector>
#include <unordered_map>

namespace fvm::management {

class VmManager {
    std::unique_ptr<contracts::IVmRepository> repository_;
    std::unique_ptr<contracts::IVirtualizationBackend> backend_;
    std::shared_ptr<EvidenceRegistry> registry_;
    
    // In-memory session tracking for Phase 2E
    std::unordered_map<domain::VmId, domain::ExecutionSession> sessions_;

public:
    VmManager(std::unique_ptr<contracts::IVmRepository> repository,
              std::unique_ptr<contracts::IVirtualizationBackend> backend,
              std::shared_ptr<EvidenceRegistry> registry);

    domain::Result<domain::VmId> createVm(const domain::VmConfig& config);
    domain::Result<void> removeVm(const domain::VmId& id);

    domain::Result<domain::VmConfig> findVm(const domain::VmId& id) const;
    domain::Result<std::vector<domain::VmId>> listVms() const;

    // Returns the ExecutionSessionId or an error.
    domain::Result<std::string> start(const domain::VmId& id);
    
    domain::Result<void> pause(const domain::VmId& id);
    domain::Result<void> resume(const domain::VmId& id);
    domain::Result<void> shutdown(const domain::VmId& id);
    domain::Result<void> powerOff(const domain::VmId& id);
    domain::Result<void> reset(const domain::VmId& id);
    
    // Explicitly query reconciled state
    domain::Result<contracts::RuntimeState> queryState(const domain::VmId& id);
};

} // namespace fvm::management
