#pragma once
#include "core/application/domain/Case.hpp"
#include "core/application/domain/OperationRecord.hpp"
#include "core/application/domain/ApplicationError.hpp"
#include "vm/domain/VmConfig.hpp"
#include "vm/domain/VmId.hpp"
#include "vm/domain/EvidenceId.hpp"
#include <filesystem>
#include <expected>
#include <vector>

namespace fvm::core::application::contracts {

class IForensicApplication {
public:
    virtual ~IForensicApplication() = default;

    // Case operations
    virtual std::expected<void, domain::ApplicationError> createCase(const std::filesystem::path& caseRoot, const domain::CaseMetadata& meta) = 0;
    virtual std::expected<void, domain::ApplicationError> openCase(const std::filesystem::path& caseRoot) = 0;
    virtual void closeCase() = 0;
    virtual bool isCaseOpen() const noexcept = 0;
    virtual std::expected<domain::Case, domain::ApplicationError> getActiveCase() const = 0;

    // Evidence operations
    virtual std::expected<domain::OperationId, domain::ApplicationError> importEvidence(const std::filesystem::path& sourcePath) = 0;
    virtual std::expected<std::vector<fvm::domain::EvidenceId>, domain::ApplicationError> listEvidence() const = 0;

    // Execution operations
    virtual std::expected<domain::OperationId, domain::ApplicationError> launchSession(const fvm::domain::VmConfig& config) = 0;
    virtual std::expected<domain::OperationId, domain::ApplicationError> stopSession(const fvm::domain::VmId& vmId) = 0;

    // Acquisition operations
    virtual std::expected<domain::OperationId, domain::ApplicationError> acquireMemory(const fvm::domain::VmId& vmId) = 0;
    virtual std::expected<domain::OperationId, domain::ApplicationError> acquireDiskDelta(const fvm::domain::VmId& vmId) = 0;

    // Operation tracking
    virtual std::expected<domain::OperationRecord, domain::ApplicationError> getOperationStatus(const domain::OperationId& opId) const = 0;
    virtual std::expected<void, domain::ApplicationError> cancelOperation(const domain::OperationId& opId) = 0;
};

} // namespace fvm::core::application::contracts
