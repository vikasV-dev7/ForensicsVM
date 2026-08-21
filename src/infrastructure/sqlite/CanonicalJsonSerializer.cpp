#include "CanonicalJsonSerializer.hpp"
#include <sstream>

namespace fvm::infrastructure::sqlite {

std::string CanonicalJsonSerializer::serialize(const std::map<std::string, JsonValue>& fields) {
    std::stringstream ss;
    ss << "{";
    bool first = true;
    for (const auto& [key, val] : fields) {
        if (!first) ss << ",";
        first = false;
        ss << "\"" << key << "\":";
        if (std::holds_alternative<std::nullptr_t>(val)) {
            ss << "null";
        } else if (std::holds_alternative<int64_t>(val)) {
            ss << std::get<int64_t>(val);
        } else {
            // escape string
            const auto& str = std::get<std::string>(val);
            ss << "\"";
            for (char c : str) {
                if (c == '"') ss << "\\\"";
                else if (c == '\\') ss << "\\\\";
                else if (c == '\b') ss << "\\b";
                else if (c == '\f') ss << "\\f";
                else if (c == '\n') ss << "\\n";
                else if (c == '\r') ss << "\\r";
                else if (c == '\t') ss << "\\t";
                else ss << c;
            }
            ss << "\"";
        }
    }
    ss << "}";
    return ss.str();
}

} // namespace fvm::infrastructure::sqlite
