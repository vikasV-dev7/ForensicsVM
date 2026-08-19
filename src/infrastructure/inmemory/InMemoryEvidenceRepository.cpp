#include "InMemoryEvidenceRepository.hpp"

namespace fvm::infrastructure::inmemory {

std::expected<void, fvm::contracts::EvidenceError> InMemoryEvidenceRepository::save(const domain::EvidenceRecord& record) {
    std::lock_guard<std::mutex> lock(mutex_);
    records_.insert_or_assign(record.id(), record);
    return {};
}

std::optional<domain::EvidenceRecord> InMemoryEvidenceRepository::find(const domain::EvidenceId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = records_.find(id);
    if (it != records_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::expected<void, fvm::contracts::EvidenceError> InMemoryEvidenceRepository::remove(const domain::EvidenceId& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (records_.erase(id) > 0) {
        return {};
    }
    return std::unexpected(fvm::contracts::EvidenceError::NotFound);
}

std::vector<domain::EvidenceRecord> InMemoryEvidenceRepository::listAll() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<domain::EvidenceRecord> result;
    result.reserve(records_.size());
    for (const auto& [id, record] : records_) {
        result.push_back(record);
    }
    return result;
}

} // namespace fvm::infrastructure::inmemory
