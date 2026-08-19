#include "EvidenceRegistry.hpp"
#include <chrono>
#include <random>

namespace fvm::management {

EvidenceRegistry::EvidenceRegistry(std::unique_ptr<fvm::contracts::IEvidenceRepository> repository,
                                   std::unique_ptr<fvm::contracts::IHashCalculator> hashCalculator)
    : repository_(std::move(repository)), hashCalculator_(std::move(hashCalculator)) {}

domain::EvidenceId EvidenceRegistry::generateId(const std::filesystem::path& path) const {
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::random_device rd;
    return domain::EvidenceId("evd-" + std::to_string(std::hash<std::string>{}(path.string())) + "-" + std::to_string(now) + "-" + std::to_string(rd()));
}

std::expected<domain::EvidenceId, std::string> EvidenceRegistry::ingest(const std::filesystem::path& path, domain::DiskFormat format) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec) {
        return std::unexpected("File not found");
    }

    std::uintmax_t sizeBytes = std::filesystem::file_size(path, ec);
    if (ec) {
        return std::unexpected("Failed to read file size");
    }

    domain::EvidenceId id = generateId(path);
    domain::EvidenceRecord record(id, path, format, sizeBytes);
    
    auto saveRes = repository_->save(record);
    if (!saveRes) {
        return std::unexpected("Failed to save initial evidence record");
    }

    // Synchronous hashing for Phase 2F
    auto hashRes = hashCalculator_->calculateSha256(path);
    if (!hashRes) {
        record.setFailed();
        auto _ = repository_->save(record); // Ignore secondary failure
        return std::unexpected("Failed to compute SHA-256 hash");
    }

    record.setVerified(hashRes.value());
    saveRes = repository_->save(record);
    if (!saveRes) {
        return std::unexpected("Failed to save verified evidence record");
    }

    return id;
}

std::optional<domain::EvidenceRecord> EvidenceRegistry::getEvidence(const domain::EvidenceId& id) const {
    auto recordOpt = repository_->find(id);
    if (!recordOpt) {
        return std::nullopt;
    }

    auto record = recordOpt.value();
    
    // Verify file still exists on disk
    if (record.status() == domain::EvidenceStatus::Verified) {
        std::error_code ec;
        if (!std::filesystem::exists(record.path(), ec) || ec) {
            record.setMissing();
            auto _ = repository_->save(record);
            return record;
        }
    }

    return record;
}

} // namespace fvm::management
