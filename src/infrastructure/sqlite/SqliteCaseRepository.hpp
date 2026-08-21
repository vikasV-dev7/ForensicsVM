#pragma once
#include "core/application/contracts/ICaseRepository.hpp"
#include <sqlite3.h>
#include <string>
#include <memory>
#include <filesystem>
#include "DatabaseContext.hpp"

namespace fvm::infrastructure::sqlite {

class SqliteCaseRepository : public core::application::contracts::ICaseRepository {
    std::shared_ptr<DatabaseContext> dbContext_;
public:
    explicit SqliteCaseRepository(std::shared_ptr<DatabaseContext> dbContext);
    ~SqliteCaseRepository() override;

    std::expected<void, core::application::contracts::CaseError> createCase(const core::application::domain::Case& caseObj, const std::filesystem::path& caseRoot) override;
    std::expected<core::application::domain::Case, core::application::contracts::CaseError> loadCase(const std::filesystem::path& caseRoot) override;
    std::expected<void, core::application::contracts::CaseError> saveCase(const core::application::domain::Case& caseObj, const std::filesystem::path& caseRoot) override;

    std::expected<void, core::application::contracts::CaseError> beginTransaction() override;
    std::expected<void, core::application::contracts::CaseError> commitTransaction(const core::application::domain::AuditRecord& audit) override;
    std::expected<void, core::application::contracts::CaseError> rollbackTransaction() override;

private:
    std::expected<void, core::application::contracts::CaseError> initSchema();
    std::expected<void, core::application::contracts::CaseError> verifySchemaVersion();
};

} // namespace fvm::infrastructure::sqlite
