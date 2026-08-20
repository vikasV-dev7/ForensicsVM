#include "ForensicApplicationImpl.hpp"
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>

namespace fvm::core::application::services {

ForensicApplicationImpl::ForensicApplicationImpl(
    std::shared_ptr<contracts::ICaseRepository> caseRepo,
    std::shared_ptr<fvm::management::VmManager> vmManager,
    std::shared_ptr<fvm::management::EvidenceRegistry> evidenceRegistry
) : caseRepo_(std::move(caseRepo)), vmManager_(std::move(vmManager)), evidenceRegistry_(std::move(evidenceRegistry)) {
}

ForensicApplicationImpl::~ForensicApplicationImpl() {
    std::lock_guard<std::mutex> lock(opsMutex_);
    for (auto& [id, worker] : workers_) {
        worker.request_stop();
    }
}

int64_t ForensicApplicationImpl::getCurrentTimeUnix() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

domain::OperationId ForensicApplicationImpl::generateOperationId() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    std::stringstream ss;
    ss << "op-" << std::hex << std::setw(16) << std::setfill('0') << dis(gen);
    return domain::OperationId(ss.str());
}

void ForensicApplicationImpl::validateCaseOpen() const {
    if (!activeCase_) {
        throw domain::CaseNotOpenError();
    }
}

void ForensicApplicationImpl::validatePathWithinCase(const std::filesystem::path& path) const {
    // Resolve and canonicalize paths to prevent traversal
    std::filesystem::path absPath;
    try {
        absPath = std::filesystem::weakly_canonical(path);
    } catch (const std::exception& e) {
        throw domain::PathSecurityError("Invalid path format: " + std::string(e.what()));
    }
    
    std::string rootStr = activeCaseRoot_.string();
    std::string targetStr = absPath.string();
    
    // Ensure the target string starts with the root string
    if (targetStr.find(rootStr) != 0) {
        throw domain::PathSecurityError("Path escape attempted: target is outside case root");
    }
}

void ForensicApplicationImpl::updateOperationState(const domain::OperationId& opId, domain::OperationState state, const std::string& message, const std::string& error) {
    std::lock_guard<std::mutex> lock(opsMutex_);
    auto it = operations_.find(opId);
    if (it != operations_.end()) {
        it->second.state = state;
        if (!message.empty()) it->second.message = message;
        if (!error.empty()) it->second.error = error;
        
        if (state == domain::OperationState::Running) {
            it->second.startedAtUnixSeconds = getCurrentTimeUnix();
        } else if (state == domain::OperationState::Completed || state == domain::OperationState::Failed || state == domain::OperationState::Cancelled) {
            it->second.completedAtUnixSeconds = getCurrentTimeUnix();
        }
    }
}

std::expected<void, domain::ApplicationError> ForensicApplicationImpl::createCase(const std::filesystem::path& caseRoot, const domain::CaseMetadata& meta) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    if (activeCase_) {
        return std::unexpected(domain::ApplicationError("A case is already open. Close it first."));
    }
    
    std::filesystem::path canonicalRoot;
    try {
        std::filesystem::create_directories(caseRoot);
        canonicalRoot = std::filesystem::canonical(caseRoot);
    } catch (const std::exception& e) {
        return std::unexpected(domain::ApplicationError("Failed to create/resolve case directory: " + std::string(e.what())));
    }

    domain::Case newCase(domain::CaseId("case-" + std::to_string(getCurrentTimeUnix())), meta);
    
    auto saveRes = caseRepo_->createCase(newCase, canonicalRoot);
    if (!saveRes) {
        return std::unexpected(domain::ApplicationError("Failed to create case in repository"));
    }

    activeCase_ = std::move(newCase);
    activeCaseRoot_ = canonicalRoot;
    
    // Create required subdirectories
    std::filesystem::create_directories(canonicalRoot / "evidence");
    std::filesystem::create_directories(canonicalRoot / "artifacts" / "memory");
    std::filesystem::create_directories(canonicalRoot / "artifacts" / "disk-delta");
    std::filesystem::create_directories(canonicalRoot / "workspace");

    return {};
}

std::expected<void, domain::ApplicationError> ForensicApplicationImpl::openCase(const std::filesystem::path& caseRoot) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    if (activeCase_) {
        return std::unexpected(domain::ApplicationError("A case is already open. Close it first."));
    }

    std::filesystem::path canonicalRoot;
    try {
        canonicalRoot = std::filesystem::canonical(caseRoot);
    } catch (const std::exception& e) {
        return std::unexpected(domain::ApplicationError("Failed to resolve case directory: " + std::string(e.what())));
    }

    auto loadRes = caseRepo_->loadCase(canonicalRoot);
    if (!loadRes) {
        return std::unexpected(domain::ApplicationError("Failed to load case from repository"));
    }

    activeCase_ = std::move(*loadRes);
    activeCaseRoot_ = canonicalRoot;
    return {};
}

void ForensicApplicationImpl::closeCase() {
    std::lock_guard<std::mutex> lock(stateMutex_);
    if (activeCase_) {
        // Save before closing
        (void)caseRepo_->saveCase(*activeCase_, activeCaseRoot_);
        activeCase_.reset();
        activeCaseRoot_.clear();
    }
}

bool ForensicApplicationImpl::isCaseOpen() const noexcept {
    std::lock_guard<std::mutex> lock(stateMutex_);
    return activeCase_.has_value();
}

std::expected<domain::Case, domain::ApplicationError> ForensicApplicationImpl::getActiveCase() const {
    std::lock_guard<std::mutex> lock(stateMutex_);
    if (activeCase_) {
        return *activeCase_;
    }
    return std::unexpected(domain::CaseNotOpenError());
}

std::expected<domain::OperationId, domain::ApplicationError> ForensicApplicationImpl::importEvidence(const std::filesystem::path& sourcePath) {
    std::lock_guard<std::mutex> sLock(stateMutex_);
    validateCaseOpen();

    auto opId = generateOperationId();
    domain::OperationRecord opRec{opId, domain::OperationType::ImportEvidence, domain::OperationState::Queued, getCurrentTimeUnix(), 0, 0, "Queued", "", ""};
    
    {
        std::lock_guard<std::mutex> lock(opsMutex_);
        operations_[opId] = opRec;
        
        workers_[opId] = std::jthread([this, opId, sourcePath, root = activeCaseRoot_](std::stop_token stoken) {
            (void)stoken;
            updateOperationState(opId, domain::OperationState::Starting, "Starting import evidence");
            
            if (!std::filesystem::exists(sourcePath) || !std::filesystem::is_regular_file(sourcePath)) {
                updateOperationState(opId, domain::OperationState::Failed, "", "Source evidence file not found or is not a regular file");
                return;
            }
            
            // Note: Since VmManager/EvidenceRegistry doesn't expose a long-running import yet, we simulate the copy step.
            updateOperationState(opId, domain::OperationState::Running, "Copying evidence to case directory");
            
            std::filesystem::path destPath = root / "evidence" / sourcePath.filename();
            try {
                if (std::filesystem::exists(destPath)) {
                    updateOperationState(opId, domain::OperationState::Failed, "", "Destination evidence file already exists in case");
                    return;
                }
                std::filesystem::copy_file(sourcePath, destPath);
            } catch (const std::exception& e) {
                updateOperationState(opId, domain::OperationState::Failed, "", std::string("Copy failed: ") + e.what());
                return;
            }
            
            updateOperationState(opId, domain::OperationState::Hashing, "Registering evidence with core");
            
            fvm::domain::EvidenceSource source(destPath, fvm::domain::DiskFormat::Raw);
            auto regRes = evidenceRegistry_->ingest(destPath, fvm::domain::DiskFormat::Raw);
            if (!regRes) {
                updateOperationState(opId, domain::OperationState::Failed, "", "Failed to register evidence with backend");
                return;
            }
            
            {
                std::lock_guard<std::mutex> l(stateMutex_);
                if (activeCase_) {
                    activeCase_->addEvidenceId(*regRes);
                    (void)caseRepo_->saveCase(*activeCase_, activeCaseRoot_);
                }
            }
            
            {
                std::lock_guard<std::mutex> l(opsMutex_);
                operations_[opId].resultReference = regRes->value();
            }
            
            updateOperationState(opId, domain::OperationState::Completed, "Evidence imported successfully");
        });
    }
    return opId;
}

std::expected<std::vector<fvm::domain::EvidenceId>, domain::ApplicationError> ForensicApplicationImpl::listEvidence() const {
    std::lock_guard<std::mutex> lock(stateMutex_);
    validateCaseOpen();
    return activeCase_->getEvidenceIds();
}

std::expected<domain::OperationId, domain::ApplicationError> ForensicApplicationImpl::launchSession(const fvm::domain::VmConfig& config) {
    std::lock_guard<std::mutex> sLock(stateMutex_);
    validateCaseOpen();

    auto opId = generateOperationId();
    domain::OperationRecord opRec{opId, domain::OperationType::LaunchSession, domain::OperationState::Queued, getCurrentTimeUnix(), 0, 0, "Queued", "", ""};
    
    {
        std::lock_guard<std::mutex> lock(opsMutex_);
        operations_[opId] = opRec;
        
        workers_[opId] = std::jthread([this, opId, config](std::stop_token stoken) {
            (void)stoken;
            updateOperationState(opId, domain::OperationState::Starting, "Preparing session");
            updateOperationState(opId, domain::OperationState::Running, "Launching session");
            
            auto createRes = vmManager_->createVm(config);
            if (!createRes) {
                updateOperationState(opId, domain::OperationState::Failed, "", "Failed to create VM session");
                return;
            }
            
            auto res = vmManager_->start(*createRes);
            if (!res) {
                updateOperationState(opId, domain::OperationState::Failed, "", "Failed to start VM session");
                return;
            }
            
            {
                std::lock_guard<std::mutex> l(stateMutex_);
                if (activeCase_) {
                    activeCase_->addVmId(*createRes);
                    (void)caseRepo_->saveCase(*activeCase_, activeCaseRoot_);
                }
            }
            
            {
                std::lock_guard<std::mutex> l(opsMutex_);
                operations_[opId].resultReference = createRes->value();
            }
            
            updateOperationState(opId, domain::OperationState::Completed, "Session launched successfully");
        });
    }
    return opId;
}

std::expected<domain::OperationId, domain::ApplicationError> ForensicApplicationImpl::stopSession(const fvm::domain::VmId& vmId) {
    std::lock_guard<std::mutex> sLock(stateMutex_);
    validateCaseOpen();

    auto opId = generateOperationId();
    domain::OperationRecord opRec{opId, domain::OperationType::StopSession, domain::OperationState::Queued, getCurrentTimeUnix(), 0, 0, "Queued", "", ""};
    
    {
        std::lock_guard<std::mutex> lock(opsMutex_);
        operations_[opId] = opRec;
        
        workers_[opId] = std::jthread([this, opId, vmId](std::stop_token stoken) {
            (void)stoken;
            updateOperationState(opId, domain::OperationState::Starting, "Preparing to stop session");
            updateOperationState(opId, domain::OperationState::Running, "Stopping session");
            
            auto res = vmManager_->powerOff(vmId);
            if (!res) {
                updateOperationState(opId, domain::OperationState::Failed, "", "Failed to stop session");
                return;
            }
            
            updateOperationState(opId, domain::OperationState::Completed, "Session stopped successfully");
        });
    }
    return opId;
}

std::expected<domain::OperationId, domain::ApplicationError> ForensicApplicationImpl::acquireMemory(const fvm::domain::VmId& vmId) {
    std::lock_guard<std::mutex> sLock(stateMutex_);
    validateCaseOpen();

    auto opId = generateOperationId();
    domain::OperationRecord opRec{opId, domain::OperationType::AcquireMemory, domain::OperationState::Queued, getCurrentTimeUnix(), 0, 0, "Queued", "", ""};
    
    {
        std::lock_guard<std::mutex> lock(opsMutex_);
        operations_[opId] = opRec;
        
        workers_[opId] = std::jthread([this, opId, vmId](std::stop_token stoken) {
            updateOperationState(opId, domain::OperationState::Running, "Memory acquisition started");
            
            auto res = vmManager_->acquireMemory(vmId, std::chrono::minutes(5), stoken);
            if (!res) {
                if (stoken.stop_requested()) {
                    updateOperationState(opId, domain::OperationState::Cancelled, "Memory acquisition cancelled");
                } else {
                    updateOperationState(opId, domain::OperationState::Failed, "", "Failed to acquire memory");
                }
                return;
            }
            
            {
                std::lock_guard<std::mutex> l(opsMutex_);
                operations_[opId].resultReference = res->value();
            }
            
            updateOperationState(opId, domain::OperationState::Completed, "Memory acquisition completed");
        });
    }
    return opId;
}

std::expected<domain::OperationId, domain::ApplicationError> ForensicApplicationImpl::acquireDiskDelta(const fvm::domain::VmId& vmId) {
    std::lock_guard<std::mutex> sLock(stateMutex_);
    validateCaseOpen();
    
    auto opId = generateOperationId();
    {
        std::lock_guard<std::mutex> lock(opsMutex_);
        operations_[opId] = domain::OperationRecord(opId, domain::OperationType::AcquireDiskDelta, domain::OperationState::Queued,
                                                    getCurrentTimeUnix(), 0, 0, "Queued disk delta acquisition", "", "");
        
        workers_[opId] = std::jthread([this, opId, vmId](std::stop_token stoken) {
            updateOperationState(opId, domain::OperationState::Running, "Disk delta acquisition started");
            
            auto res = vmManager_->acquireDiskDelta(vmId, "drive0", std::chrono::minutes(15), stoken);
            if (!res) {
                if (stoken.stop_requested()) {
                    updateOperationState(opId, domain::OperationState::Cancelled, "Disk delta acquisition cancelled");
                } else {
                    updateOperationState(opId, domain::OperationState::Failed, "", "Failed to acquire disk delta");
                }
                return;
            }
            
            {
                std::lock_guard<std::mutex> l(opsMutex_);
                operations_[opId].resultReference = res->value();
            }
            
            updateOperationState(opId, domain::OperationState::Completed, "Disk delta acquisition completed");
        });
    }
    return opId;
}

std::expected<domain::OperationRecord, domain::ApplicationError> ForensicApplicationImpl::getOperationStatus(const domain::OperationId& opId) const {
    std::lock_guard<std::mutex> lock(opsMutex_);
    auto it = operations_.find(opId);
    if (it != operations_.end()) {
        return it->second;
    }
    return std::unexpected(domain::InvalidOperationError("Operation not found"));
}

std::expected<void, domain::ApplicationError> ForensicApplicationImpl::cancelOperation(const domain::OperationId& opId) {
    std::lock_guard<std::mutex> lock(opsMutex_);
    auto it = operations_.find(opId);
    if (it == operations_.end()) {
        return std::unexpected(domain::InvalidOperationError("Operation not found"));
    }

    if (it->second.state == domain::OperationState::Completed || 
        it->second.state == domain::OperationState::Failed || 
        it->second.state == domain::OperationState::Cancelled) {
        return std::unexpected(domain::InvalidOperationError("Operation already finished"));
    }

    // Mark as cancelling requested
    it->second.state = domain::OperationState::Cancelling;
    it->second.message = "Cancellation requested";
    
    // Request jthread stop. If the underlying VmManager call respects stop tokens, it will abort.
    // In Phase 4, VmManager might be fully synchronous and un-interruptible, so it may still complete or fail on its own.
    auto workerIt = workers_.find(opId);
    if (workerIt != workers_.end()) {
        workerIt->second.request_stop();
    }
    
    return {};
}

} // namespace fvm::core::application::services
