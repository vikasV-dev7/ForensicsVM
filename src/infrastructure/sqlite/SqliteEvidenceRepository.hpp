#pragma once
#include "vm/contracts/IEvidenceRepository.hpp"
#include "infrastructure/sqlite/DatabaseContext.hpp"
#include <memory>

namespace fvm::infrastructure::sqlite {

class SqliteEvidenceRepository : public fvm::contracts::IEvidenceRepository {
    std::shared_ptr<DatabaseContext> dbContext_;
    
public:
    explicit SqliteEvidenceRepository(std::shared_ptr<DatabaseContext> dbContext);
    
    std::expected<void, fvm::contracts::EvidenceError> save(const domain::EvidenceRecord& record) override;
    std::optional<domain::EvidenceRecord> find(const domain::EvidenceId& id) const override;
    std::expected<void, fvm::contracts::EvidenceError> remove(const domain::EvidenceId& id) override;
    std::vector<domain::EvidenceRecord> listAll() const override;
};

} // namespace fvm::infrastructure::sqlite
