#pragma once
#include <string>
#include <utility>

namespace fvm::domain {

class VmId {
    std::string id_;
public:
    explicit VmId(std::string id) : id_(std::move(id)) {}

    const std::string& value() const noexcept { return id_; }

    bool operator==(const VmId& other) const = default;
    
    bool empty() const noexcept { return id_.empty(); }
};

} // namespace fvm::domain

namespace std {
    template<> struct hash<fvm::domain::VmId> {
        std::size_t operator()(const fvm::domain::VmId& k) const {
            return hash<string>()(k.value());
        }
    };
}
