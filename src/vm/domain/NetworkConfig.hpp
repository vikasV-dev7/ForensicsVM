#pragma once
#include <string>

namespace fvm::domain {

enum class NetworkAttachmentMode {
    NAT,
    Bridged,
    HostOnly,
    Internal,
    Isolated,
    Custom
};

struct NetworkConfig {
    std::string nicId;
    std::string macAddress;
    NetworkAttachmentMode mode;
    std::string adapterModel;
    bool enabled;

    bool isValid() const noexcept {
        return !nicId.empty();
    }
};

} // namespace fvm::domain
