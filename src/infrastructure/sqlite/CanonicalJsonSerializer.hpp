#pragma once
#include <string>
#include <map>
#include <cstdint>
#include <variant>

namespace fvm::infrastructure::sqlite {

using JsonValue = std::variant<std::string, int64_t, std::nullptr_t>;

class CanonicalJsonSerializer {
public:
    static std::string serialize(const std::map<std::string, JsonValue>& fields);
};

} // namespace fvm::infrastructure::sqlite
