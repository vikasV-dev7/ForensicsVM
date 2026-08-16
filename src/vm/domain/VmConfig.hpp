#pragma once
#include "VmId.hpp"
#include "CpuConfig.hpp"
#include "MemoryConfig.hpp"
#include "StorageAttachment.hpp"
#include "NetworkConfig.hpp"
#include "FirmwareConfig.hpp"
#include "DisplayConfig.hpp"
#include <string>
#include <vector>

namespace fvm::domain {

struct VmConfig {
    VmId id;
    std::string name;
    std::string description;
    
    CpuConfig cpu;
    MemoryConfig memory;
    std::vector<StorageAttachment> storage;
    std::vector<NetworkConfig> network;
    FirmwareConfig firmware;
    DisplayConfig display;

    bool isValid() const noexcept {
        if (id.empty() || name.empty()) return false;
        if (!cpu.isValid() || !memory.isValid() || !firmware.isValid()) return false;
        
        for (const auto& s : storage) {
            if (!s.isValid()) return false;
        }
        for (const auto& n : network) {
            if (!n.isValid()) return false;
        }
        return true;
    }
};

} // namespace fvm::domain
