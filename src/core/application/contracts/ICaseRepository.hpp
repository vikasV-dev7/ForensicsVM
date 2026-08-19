#pragma once
#include "core/application/domain/Case.hpp"
#include <expected>
#include <filesystem>

namespace fvm::core::application::contracts {

enum class CaseError {
    NotFound,
    Duplicate,
    Malformed,
    SchemaMismatch,
    DatabaseCorruption,
    PersistenceFailure,
    InvalidPath
};

class ICaseRepository {
public:
    virtual ~ICaseRepository() = default;

    virtual std::expected<void, CaseError> createCase(const domain::Case& caseObj, const std::filesystem::path& caseRoot) = 0;
    virtual std::expected<domain::Case, CaseError> loadCase(const std::filesystem::path& caseRoot) = 0;
    virtual std::expected<void, CaseError> saveCase(const domain::Case& caseObj, const std::filesystem::path& caseRoot) = 0;
};

} // namespace fvm::core::application::contracts
