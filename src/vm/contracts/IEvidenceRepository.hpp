#pragma once
#include "vm/domain/EvidenceRecord.hpp"
#include <expected>
#include <vector>
#include <optional>

namespace fvm::contracts {

enum class EvidenceError {
    NotFound,
    Duplicate,
    StorageError
};

class IEvidenceRepository {
public:
    virtual ~IEvidenceRepository() = default;

    virtual std::expected<void, EvidenceError> save(const domain::EvidenceRecord& record) = 0;
    virtual std::optional<domain::EvidenceRecord> find(const domain::EvidenceId& id) const = 0;
    virtual std::expected<void, EvidenceError> remove(const domain::EvidenceId& id) = 0;
    virtual std::vector<domain::EvidenceRecord> listAll() const = 0;
};

} // namespace fvm::contracts
