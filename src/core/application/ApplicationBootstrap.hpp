#pragma once
#include "vm/management/VmManager.hpp"
#include <memory>

namespace fvm::core::application {

class ApplicationBootstrap {
public:
    // Assembles the concrete infrastructure and injects it into the VmManager
    static std::unique_ptr<fvm::management::VmManager> createVmManager();
};

} // namespace fvm::core::application
