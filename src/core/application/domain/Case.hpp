#pragma once
#include "CaseId.hpp"
#include "CaseMetadata.hpp"
#include "vm/domain/EvidenceId.hpp"
#include "vm/domain/VmId.hpp"
#include <vector>

namespace fvm::core::application::domain {

class Case {
public:
    Case(CaseId id, CaseMetadata metadata)
        : id_(std::move(id)), metadata_(std::move(metadata)) {}

    const CaseId& getId() const noexcept { return id_; }
    
    const CaseMetadata& getMetadata() const noexcept { return metadata_; }
    void setMetadata(CaseMetadata meta) { metadata_ = std::move(meta); }

    const std::vector<fvm::domain::EvidenceId>& getEvidenceIds() const noexcept { return evidenceIds_; }
    void addEvidenceId(fvm::domain::EvidenceId evidenceId) { evidenceIds_.push_back(std::move(evidenceId)); }

    const std::vector<fvm::domain::VmId>& getVmIds() const noexcept { return vmIds_; }
    void addVmId(fvm::domain::VmId vmId) { vmIds_.push_back(std::move(vmId)); }

private:
    CaseId id_;
    CaseMetadata metadata_;
    std::vector<fvm::domain::EvidenceId> evidenceIds_;
    std::vector<fvm::domain::VmId> vmIds_;
};

} // namespace fvm::core::application::domain
