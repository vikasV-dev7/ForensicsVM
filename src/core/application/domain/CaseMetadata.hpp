#pragma once
#include <string>
#include <cstdint>

namespace fvm::core::application::domain {

struct CaseMetadata {
    std::string name;
    std::string description;
    std::string investigator;
    int64_t createdAtUnixSeconds{0};
    int64_t updatedAtUnixSeconds{0};
};

} // namespace fvm::core::application::domain
