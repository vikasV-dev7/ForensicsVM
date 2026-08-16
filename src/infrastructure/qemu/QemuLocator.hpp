#pragma once
#include <string>
#include <optional>
#include <expected>

namespace fvm::infrastructure::qemu {

class QemuLocator {
public:
    enum class Error {
        NotFound,
        ExecutionFailed,
        InvalidVersion
    };

    virtual ~QemuLocator() = default;

    // Discovers and validates the QEMU executable.
    virtual std::expected<std::string, Error> discover(const std::optional<std::string>& explicitPath = std::nullopt) const = 0;
};

class DefaultQemuLocator : public QemuLocator {
public:
    std::expected<std::string, Error> discover(const std::optional<std::string>& explicitPath = std::nullopt) const override;
private:
    std::expected<void, Error> validate(const std::string& path) const;
    std::optional<std::string> findInPath() const;
};

} // namespace fvm::infrastructure::qemu
