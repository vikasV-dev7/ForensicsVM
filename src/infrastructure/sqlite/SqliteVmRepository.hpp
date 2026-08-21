#pragma once
#include "vm/contracts/IVmRepository.hpp"
#include "infrastructure/sqlite/DatabaseContext.hpp"
#include <memory>
#include <unordered_map>
#include <mutex>

namespace fvm::infrastructure::sqlite {

class SqliteVmRepository : public fvm::contracts::IVmRepository {
    std::shared_ptr<DatabaseContext> dbContext_;
    
    // For Phase 8 we will keep a minimal schema for VmConfig, 
    // and persist them to SQLite.
public:
    explicit SqliteVmRepository(std::shared_ptr<DatabaseContext> dbContext);
    
    domain::Result<void> save(const domain::VmConfig& config, domain::VmState state) override;
    domain::Result<void> remove(const domain::VmId& id) override;
    
    domain::Result<domain::VmConfig> findConfig(const domain::VmId& id) const override;
    domain::Result<domain::VmState> findState(const domain::VmId& id) const override;
    domain::Result<std::vector<domain::VmId>> listAll() const override;
};

} // namespace fvm::infrastructure::sqlite
