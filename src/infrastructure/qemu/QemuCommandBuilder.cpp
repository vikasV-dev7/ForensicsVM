#include "QemuCommandBuilder.hpp"

namespace fvm::infrastructure::qemu {

QemuLaunchSpec DefaultQemuCommandBuilder::build(const domain::VmConfig& config, const std::string& executablePath) const {
    QemuLaunchSpec spec;
    spec.executablePath = executablePath;
    
    // Base configuration (safe defaults)
    spec.arguments = {
        "-accel", "whpx",
        "-nodefaults",
        "-name", config.name,
        "-m", std::to_string(config.memory.assignedMemory.value()),
        "-smp", std::to_string(config.cpu.vcpus.value())
    };
    
    // Display configuration
    if (config.display.displayCount == 0) {
        spec.arguments.push_back("-display");
        spec.arguments.push_back("none");
    } else {
        spec.arguments.push_back("-display");
        spec.arguments.push_back("sdl");
        spec.arguments.push_back("-vga");
        spec.arguments.push_back("std");
    }
    
    // Storage
    for (const auto& storage : config.storage) {
        spec.arguments.push_back("-drive");
        std::string driveArg = "file=" + storage.path + ",format=qcow2";
        if (storage.readOnly) {
            driveArg += ",readonly=on";
        }
        spec.arguments.push_back(driveArg);
    }
    
    return spec;
}

} // namespace fvm::infrastructure::qemu
