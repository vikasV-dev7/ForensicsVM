#pragma once
#include <filesystem>
#include <string>

namespace fvm::domain {

enum class DiskFormat {
    Raw,
    Qcow2,
    Vhdx,
    Vmdk,
    Elf
};

class EvidenceSource {
    std::filesystem::path path_;
    DiskFormat format_;

public:
    EvidenceSource(std::filesystem::path path, DiskFormat format)
        : path_(std::move(path)), format_(format) {}

    const std::filesystem::path& path() const noexcept { return path_; }
    DiskFormat format() const noexcept { return format_; }

    bool isValid() const noexcept {
        return !path_.empty();
    }
};

} // namespace fvm::domain
