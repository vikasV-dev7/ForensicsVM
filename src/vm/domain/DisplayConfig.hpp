#pragma once
#include <cstdint>

namespace fvm::domain {

struct DisplayConfig {
    uint32_t vramMegabytes{16};
    uint32_t displayCount{1};
    bool hardwareAcceleration{false};
};

} // namespace fvm::domain
