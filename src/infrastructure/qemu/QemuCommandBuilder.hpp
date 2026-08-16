#pragma once
#include "QemuLaunchSpec.hpp"
#include "vm/domain/VmConfig.hpp"
#include "vm/domain/VmId.hpp"
#include <string>
#include <map>

namespace fvm::infrastructure::qemu {

class QemuCommandBuilder {
public:
    virtual ~QemuCommandBuilder() = default;
    virtual QemuLaunchSpec build(const domain::VmId& id, const domain::VmConfig& config, const std::string& executablePath, const std::map<std::string, std::string>& overlayPaths = {}) const = 0;
};

class DefaultQemuCommandBuilder : public QemuCommandBuilder {
public:
    QemuLaunchSpec build(const domain::VmId& id, const domain::VmConfig& config, const std::string& executablePath, const std::map<std::string, std::string>& overlayPaths = {}) const override;
};

} // namespace fvm::infrastructure::qemu
