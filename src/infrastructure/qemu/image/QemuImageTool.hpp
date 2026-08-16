#pragma once
#include "IQemuImageTool.hpp"
#include "QemuImgLocator.hpp"
#include <memory>
#include <mutex>

namespace fvm::infrastructure::qemu::image {

class QemuImageTool : public IQemuImageTool {
public:
    explicit QemuImageTool(std::unique_ptr<QemuImgLocator> locator);
    ~QemuImageTool() override = default;

    std::expected<void, Error> createOverlay(
        const std::filesystem::path& evidencePath, 
        domain::DiskFormat evidenceFormat, 
        const std::filesystem::path& overlayPath) const override;

private:
    std::unique_ptr<QemuImgLocator> locator_;
    
    // We cache the discovered executable path to avoid running 'discover()' every time
    mutable std::string executablePath_;
    mutable std::once_flag initFlag_;
    mutable std::expected<void, Error> initStatus_;
    
    std::expected<void, Error> initialize() const;
    std::string formatToString(domain::DiskFormat format) const;
};

} // namespace fvm::infrastructure::qemu::image
