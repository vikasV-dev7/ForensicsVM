#pragma once
#include "QemuLaunchSpec.hpp"
#include "vm/domain/VmConfig.hpp"
#include <string>

namespace fvm::infrastructure::qemu {

class QemuCommandBuilder {
public:
    virtual ~QemuCommandBuilder() = default;
    virtual QemuLaunchSpec build(const domain::VmConfig& config, const std::string& executablePath) const = 0;
};

class DefaultQemuCommandBuilder : public QemuCommandBuilder {
public:
    QemuLaunchSpec build(const domain::VmConfig& config, const std::string& executablePath) const override;
};

} // namespace fvm::infrastructure::qemu
