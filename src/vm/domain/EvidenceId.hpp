#pragma once
#include <string>
#include <compare>
#include <functional>

namespace fvm::domain {

class EvidenceId {
    std::string value_;

public:
    EvidenceId() = default;
    explicit EvidenceId(std::string value) : value_(std::move(value)) {}

    const std::string& value() const noexcept { return value_; }
    bool empty() const noexcept { return value_.empty(); }

    auto operator<=>(const EvidenceId&) const = default;
};

} // namespace fvm::domain

namespace std {
    template<> struct hash<fvm::domain::EvidenceId> {
        std::size_t operator()(const fvm::domain::EvidenceId& id) const noexcept {
            return std::hash<std::string>{}(id.value());
        }
    };
}
