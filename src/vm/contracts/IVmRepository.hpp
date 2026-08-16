#pragma once
#include "vm/domain/VmId.hpp"
#include "vm/domain/VmConfig.hpp"
#include "vm/domain/VmState.hpp"
#include "vm/domain/VmError.hpp"
#include <vector>

namespace fvm::contracts {

class IVmRepository {
public:
    virtual ~IVmRepository() = default;

    virtual domain::Result<void> save(const domain::VmConfig& config, domain::VmState state) = 0;
    virtual domain::Result<void> remove(const domain::VmId& id) = 0;
    
    virtual domain::Result<domain::VmConfig> findConfig(const domain::VmId& id) const = 0;
    virtual domain::Result<domain::VmState> findState(const domain::VmId& id) const = 0;
    virtual domain::Result<std::vector<domain::VmId>> listAll() const = 0;
};

} // namespace fvm::contracts
