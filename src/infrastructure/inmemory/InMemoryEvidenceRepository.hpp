#pragma once
#include "vm/contracts/IEvidenceRepository.hpp"
#include <unordered_map>
#include <mutex>

namespace fvm::infrastructure::inmemory {

class InMemoryEvidenceRepository : public fvm::contracts::IEvidenceRepository {
public:
    std::expected<void, fvm::contracts::EvidenceError> save(const domain::EvidenceRecord& record) override;
    std::optional<domain::EvidenceRecord> find(const domain::EvidenceId& id) const override;
    std::expected<void, fvm::contracts::EvidenceError> remove(const domain::EvidenceId& id) override;
    std::vector<domain::EvidenceRecord> listAll() const override;

private:
    mutable std::mutex mutex_;
    std::unordered_map<domain::EvidenceId, domain::EvidenceRecord> records_;
};

} // namespace fvm::infrastructure::inmemory
