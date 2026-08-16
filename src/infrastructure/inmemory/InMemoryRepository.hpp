#pragma once
#include "vm/contracts/IVmRepository.hpp"
#include <unordered_map>
#include <mutex>

namespace fvm::infrastructure::inmemory {

class InMemoryRepository : public fvm::contracts::IVmRepository {
    struct Entry {
        domain::VmConfig config;
        domain::VmState state;
    };
    std::unordered_map<domain::VmId, Entry> store_;

public:
    domain::Result<void> save(const domain::VmConfig& config, domain::VmState state) override;
    domain::Result<void> remove(const domain::VmId& id) override;
    domain::Result<domain::VmConfig> findConfig(const domain::VmId& id) const override;
    domain::Result<domain::VmState> findState(const domain::VmId& id) const override;
    domain::Result<std::vector<domain::VmId>> listAll() const override;
};

} // namespace fvm::infrastructure::inmemory
