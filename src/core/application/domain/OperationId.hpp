#pragma once
#include <string>
#include <utility>
#include <functional>

namespace fvm::core::application::domain {

class OperationId {
    std::string id_;
public:
    OperationId() : id_("") {}
    explicit OperationId(std::string id) : id_(std::move(id)) {}

    const std::string& value() const noexcept { return id_; }

    bool operator==(const OperationId& other) const = default;
    
    bool empty() const noexcept { return id_.empty(); }
};

} // namespace fvm::core::application::domain

namespace std {
    template<> struct hash<fvm::core::application::domain::OperationId> {
        std::size_t operator()(const fvm::core::application::domain::OperationId& k) const {
            return hash<string>()(k.value());
        }
    };
}
