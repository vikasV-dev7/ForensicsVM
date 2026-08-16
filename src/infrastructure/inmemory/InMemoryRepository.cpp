#include "InMemoryRepository.hpp"

namespace fvm::infrastructure::inmemory {

domain::Result<void> InMemoryRepository::save(const domain::VmConfig& config, domain::VmState state) {
    store_.insert_or_assign(config.id, Entry{config, state});
    return {};
}

domain::Result<void> InMemoryRepository::remove(const domain::VmId& id) {
    if (store_.erase(id) == 0) return std::unexpected(domain::VmError::VmNotFound);
    return {};
}

domain::Result<domain::VmConfig> InMemoryRepository::findConfig(const domain::VmId& id) const {
    auto it = store_.find(id);
    if (it == store_.end()) return std::unexpected(domain::VmError::VmNotFound);
    return it->second.config;
}

domain::Result<domain::VmState> InMemoryRepository::findState(const domain::VmId& id) const {
    auto it = store_.find(id);
    if (it == store_.end()) return std::unexpected(domain::VmError::VmNotFound);
    return it->second.state;
}

domain::Result<std::vector<domain::VmId>> InMemoryRepository::listAll() const {
    std::vector<domain::VmId> list;
    for (const auto& [id, entry] : store_) {
        list.push_back(id);
    }
    return list;
}

} // namespace fvm::infrastructure::inmemory
