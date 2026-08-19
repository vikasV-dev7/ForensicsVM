#pragma once
#include "vm/contracts/IEvidenceRepository.hpp"
#include "vm/contracts/IHashCalculator.hpp"
#include <memory>
#include <string>

namespace fvm::management {

class EvidenceRegistry {
public:
    EvidenceRegistry(std::unique_ptr<fvm::contracts::IEvidenceRepository> repository,
                     std::unique_ptr<fvm::contracts::IHashCalculator> hashCalculator);

    std::expected<domain::EvidenceId, std::string> ingest(const std::filesystem::path& path, domain::DiskFormat format);
    std::optional<domain::EvidenceRecord> getEvidence(const domain::EvidenceId& id) const;

private:
    std::unique_ptr<fvm::contracts::IEvidenceRepository> repository_;
    std::unique_ptr<fvm::contracts::IHashCalculator> hashCalculator_;

    domain::EvidenceId generateId(const std::filesystem::path& path) const;
};

} // namespace fvm::management
