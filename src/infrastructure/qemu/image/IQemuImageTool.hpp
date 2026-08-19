#pragma once
#include <string>
#include <filesystem>
#include <expected>
#include "vm/domain/StorageAttachment.hpp"
#include "vm/domain/EvidenceSource.hpp"

namespace fvm::infrastructure::qemu::image {

class IQemuImageTool {
public:
    enum class Error {
        ToolNotFound,
        ExecutionFailed,
        InvalidPath,
        UnsupportedFormat
    };

    virtual ~IQemuImageTool() = default;

    virtual std::expected<void, Error> createOverlay(
        const std::filesystem::path& evidencePath, 
        domain::DiskFormat evidenceFormat, 
        const std::filesystem::path& overlayPath) const = 0;
};

} // namespace fvm::infrastructure::qemu::image
