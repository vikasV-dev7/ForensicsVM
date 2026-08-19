#pragma once
#include "EvidenceId.hpp"
#include "EvidenceSource.hpp" // For DiskFormat
#include <filesystem>
#include <string>
#include <optional>
#include <cstdint>

namespace fvm::domain {

enum class EvidenceStatus {
    Ingesting,
    Verified,
    Missing,
    Failed
};

class EvidenceRecord {
    EvidenceId id_;
    std::filesystem::path path_;
    DiskFormat format_;
    std::string sha256_;
    std::uintmax_t sizeBytes_;
    EvidenceStatus status_;

public:
    EvidenceRecord(EvidenceId id, std::filesystem::path path, DiskFormat format, std::uintmax_t sizeBytes)
        : id_(std::move(id)), path_(std::move(path)), format_(format), 
          sizeBytes_(sizeBytes), status_(EvidenceStatus::Ingesting) {}

    const EvidenceId& id() const noexcept { return id_; }
    const std::filesystem::path& path() const noexcept { return path_; }
    DiskFormat format() const noexcept { return format_; }
    const std::string& sha256() const noexcept { return sha256_; }
    std::uintmax_t sizeBytes() const noexcept { return sizeBytes_; }
    EvidenceStatus status() const noexcept { return status_; }

    void setVerified(std::string hash) {
        sha256_ = std::move(hash);
        status_ = EvidenceStatus::Verified;
    }

    void setMissing() {
        status_ = EvidenceStatus::Missing;
    }

    void setFailed() {
        status_ = EvidenceStatus::Failed;
    }
};

} // namespace fvm::domain
