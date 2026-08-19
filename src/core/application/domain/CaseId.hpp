#pragma once
#include <string>
#include <utility>
#include <functional>

namespace fvm::core::application::domain {

class CaseId {
    std::string id_;
public:
    explicit CaseId(std::string id) : id_(std::move(id)) {}

    const std::string& value() const noexcept { return id_; }

    bool operator==(const CaseId& other) const = default;
    
    bool empty() const noexcept { return id_.empty(); }
};

} // namespace fvm::core::application::domain

namespace std {
    template<> struct hash<fvm::core::application::domain::CaseId> {
        std::size_t operator()(const fvm::core::application::domain::CaseId& k) const {
            return hash<string>()(k.value());
        }
    };
}
