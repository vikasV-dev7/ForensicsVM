#pragma once
#include <string>
#include <cstdint>

#include <variant>
#include <map>

namespace fvm::core::application::domain {

using AuditField = std::variant<std::string, int64_t, std::nullptr_t>;

struct AuditRecord {
    int64_t seqNum{0};
    std::string eventId;
    int64_t timestampUnixMs{0};
    std::string eventType;
    std::string payload; // Final JSON format, rendered by repository
    std::map<std::string, AuditField> payloadFields; // Used to construct canonical payload
    std::string previousHash;
    std::string currentHash;
};

} // namespace fvm::core::application::domain
