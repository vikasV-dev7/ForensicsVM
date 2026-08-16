#pragma once
#include <string>
#include <vector>

namespace fvm::infrastructure::qemu {

struct QemuLaunchSpec {
    std::string executablePath;
    std::vector<std::string> arguments;
};

} // namespace fvm::infrastructure::qemu
