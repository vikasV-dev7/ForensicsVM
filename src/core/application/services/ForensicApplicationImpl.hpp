#pragma once
#include "core/application/contracts/IForensicApplication.hpp"
#include "core/application/contracts/ICaseRepository.hpp"
#include "vm/management/VmManager.hpp"
#include <mutex>
#include <thread>
#include <unordered_map>
#include <memory>
#include <optional>
#include <filesystem>

#include "vm/management/EvidenceRegistry.hpp"

namespace fvm::core::application::services {

class ForensicApplicationImpl : public contracts::IForensicApplication {
public:
    ForensicApplicationImpl(
        std::shared_ptr<contracts::ICaseRepository> caseRepo,
        std::shared_ptr<fvm::management::VmManager> vmManager,
        std::shared_ptr<fvm::management::EvidenceRegistry> evidenceRegistry
    );
    ~ForensicApplicationImpl() override;

    std::expected<void, domain::ApplicationError> createCase(const std::filesystem::path& caseRoot, const domain::CaseMetadata& meta) override;
    std::expected<void, domain::ApplicationError> openCase(const std::filesystem::path& caseRoot) override;
    void closeCase() override;
    bool isCaseOpen() const noexcept override;
    std::expected<domain::Case, domain::ApplicationError> getActiveCase() const override;

    std::expected<domain::OperationId, domain::ApplicationError> importEvidence(const std::filesystem::path& sourcePath) override;
    std::expected<std::vector<fvm::domain::EvidenceId>, domain::ApplicationError> listEvidence() const override;

    std::expected<domain::OperationId, domain::ApplicationError> launchSession(const fvm::domain::VmConfig& config) override;
    std::expected<domain::OperationId, domain::ApplicationError> stopSession(const fvm::domain::VmId& vmId) override;

    std::expected<domain::OperationId, domain::ApplicationError> acquireMemory(const fvm::domain::VmId& vmId) override;
    std::expected<domain::OperationId, domain::ApplicationError> acquireDiskDelta(const fvm::domain::VmId& vmId) override;

    std::expected<domain::OperationRecord, domain::ApplicationError> getOperationStatus(const domain::OperationId& opId) const override;
    std::expected<void, domain::ApplicationError> cancelOperation(const domain::OperationId& opId) override;

private:
    std::shared_ptr<contracts::ICaseRepository> caseRepo_;
    std::shared_ptr<fvm::management::VmManager> vmManager_;
    std::shared_ptr<fvm::management::EvidenceRegistry> evidenceRegistry_;
    
    mutable std::mutex stateMutex_;
    std::optional<domain::Case> activeCase_;
    std::filesystem::path activeCaseRoot_;
    
    mutable std::mutex opsMutex_;
    std::unordered_map<domain::OperationId, domain::OperationRecord> operations_;
    std::unordered_map<domain::OperationId, std::jthread> workers_;

    domain::OperationId generateOperationId();
    int64_t getCurrentTimeUnix();
    
    void updateOperationState(const domain::OperationId& opId, domain::OperationState state, const std::string& message = "", const std::string& error = "");
    void validateCaseOpen() const;
    void validatePathWithinCase(const std::filesystem::path& path) const;
};

} // namespace fvm::core::application::services
