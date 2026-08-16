#pragma once
#include <string>
#include <cstdint>

namespace fvm::domain {

class StorageCapacity {
    uint64_t gigabytes_;
public:
    explicit StorageCapacity(uint64_t gb) : gigabytes_(gb) {}
    uint64_t value() const noexcept { return gigabytes_; }
    bool operator==(const StorageCapacity&) const = default;
};

enum class BusType {
    VirtIO,
    SATA,
    NVMe,
    IDE
};

struct StorageConfig {
    std::string diskId;
    std::string path;
    StorageCapacity capacity;
    BusType bus;
    bool readOnly;
    bool bootable;

    bool isValid() const noexcept {
        return !diskId.empty() && capacity.value() > 0;
    }
};

} // namespace fvm::domain
