#pragma once
#include <cstdint>

namespace fvm::domain {

class Megabytes {
    uint64_t value_;
public:
    explicit Megabytes(uint64_t v) : value_(v) {}
    uint64_t value() const noexcept { return value_; }
    bool operator==(const Megabytes&) const = default;
    bool operator>=(const Megabytes& o) const noexcept { return value_ >= o.value_; }
    bool operator<=(const Megabytes& o) const noexcept { return value_ <= o.value_; }
};

struct MemoryConfig {
    Megabytes assignedMemory;
    Megabytes minMemory{0};
    Megabytes maxMemory{0};

    bool isValid() const noexcept {
        if (assignedMemory.value() == 0) return false;
        if (minMemory.value() > 0 && assignedMemory.value() < minMemory.value()) return false;
        if (maxMemory.value() > 0 && assignedMemory.value() > maxMemory.value()) return false;
        return true;
    }
};

} // namespace fvm::domain
