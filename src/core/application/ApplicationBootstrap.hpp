#pragma once
#include "vm/management/VmManager.hpp"
#include "core/application/contracts/IForensicApplication.hpp"
#include <memory>

namespace fvm::core::application {

class ApplicationBootstrap {
public:
    static std::unique_ptr<fvm::management::VmManager> createVmManager();
    static std::unique_ptr<fvm::core::application::contracts::IForensicApplication> createApplication();
};

} // namespace fvm::core::application
