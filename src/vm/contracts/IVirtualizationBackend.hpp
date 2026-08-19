#pragma once
#include "vm/domain/VmId.hpp"
#include "vm/domain/VmConfig.hpp"
#include "vm/domain/VmState.hpp"
#include "vm/domain/VmError.hpp"
#include "vm/domain/EvidenceRecord.hpp"
#include "vm/domain/AcquisitionResult.hpp"
#include "vm/domain/TerminationReason.hpp"
#include "vm/domain/SessionEvidence.hpp"
#include <vector>

namespace fvm::contracts {

struct RuntimeState {
    domain::VmState state;
    domain::TerminationReason reason;
};

class IVirtualizationBackend {
public:
    virtual ~IVirtualizationBackend() = default;

    virtual domain::Result<void> createVm(const domain::VmConfig& config) = 0;
    virtual domain::Result<void> destroyVm(const domain::VmId& id) = 0;
    
    // startVm now returns the generated SessionEvidence records so the manager can track provenance.
    virtual domain::Result<std::vector<domain::SessionEvidence>> startVm(const domain::VmId& id, const std::vector<domain::EvidenceRecord>& resolvedEvidence) = 0;
    
    virtual domain::Result<void> pauseVm(const domain::VmId& id) = 0;
    virtual domain::Result<void> resumeVm(const domain::VmId& id) = 0;
    virtual domain::Result<void> shutdownVm(const domain::VmId& id) = 0;
    virtual domain::Result<void> powerOffVm(const domain::VmId& id) = 0;
    virtual domain::Result<void> resetVm(const domain::VmId& id) = 0;
    
    virtual domain::Result<domain::AcquisitionResult> acquireMemory(const domain::VmId& id, std::chrono::milliseconds timeout) = 0;
    virtual domain::Result<domain::AcquisitionResult> acquireDiskDelta(const domain::VmId& id, const std::string& diskId, std::chrono::milliseconds timeout) = 0;

    // queryState evaluates the deterministic lifecycle and returns the reconciled state and termination reason.
    virtual domain::Result<RuntimeState> queryState(const domain::VmId& id) = 0;
};

} // namespace fvm::contracts
