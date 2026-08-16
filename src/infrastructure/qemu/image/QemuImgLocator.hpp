#pragma once
#include <string>
#include <optional>
#include <expected>

namespace fvm::infrastructure::qemu::image {

class QemuImgLocator {
public:
    enum class Error {
        NotFound,
        ExecutionFailed,
        InvalidVersion
    };

    virtual ~QemuImgLocator() = default;

    virtual std::expected<std::string, Error> discover(const std::optional<std::string>& explicitPath = std::nullopt) const = 0;
};

class DefaultQemuImgLocator : public QemuImgLocator {
public:
    std::expected<std::string, Error> discover(const std::optional<std::string>& explicitPath = std::nullopt) const override;
private:
    std::expected<void, Error> validate(const std::string& path) const;
    std::optional<std::string> findInPath() const;
};

} // namespace fvm::infrastructure::qemu::image
