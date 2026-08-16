#pragma once
#include <cstdint>

namespace fvm::domain {

class CpuCount {
    uint32_t count_;
public:
    explicit CpuCount(uint32_t count) : count_(count) {}
    uint32_t value() const noexcept { return count_; }
    bool operator==(const CpuCount&) const = default;
};

struct CpuConfig {
    CpuCount vcpus;
    uint32_t sockets{1};
    uint32_t cores{1};
    uint32_t threads{1};

    bool isValid() const noexcept {
        return vcpus.value() > 0 && sockets > 0 && cores > 0 && threads > 0 && 
               (sockets * cores * threads == vcpus.value());
    }
};

} // namespace fvm::domain
