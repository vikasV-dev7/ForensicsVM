#pragma once

namespace fvm::domain {

enum class FirmwareType {
    BIOS,
    UEFI
};

struct FirmwareConfig {
    FirmwareType type;
    bool secureBootEnabled{false};
    bool tpmEnabled{false};

    bool isValid() const noexcept {
        if (secureBootEnabled && type == FirmwareType::BIOS) return false;
        return true;
    }
};

} // namespace fvm::domain
