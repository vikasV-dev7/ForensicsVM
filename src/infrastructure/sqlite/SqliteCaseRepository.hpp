#pragma once
#include "core/application/contracts/ICaseRepository.hpp"
#include <sqlite3.h>
#include <string>
#include <memory>
#include <filesystem>

namespace fvm::infrastructure::sqlite {

class SqliteCaseRepository : public core::application::contracts::ICaseRepository {
public:
    SqliteCaseRepository();
    ~SqliteCaseRepository() override;

    std::expected<void, core::application::contracts::CaseError> createCase(const core::application::domain::Case& caseObj, const std::filesystem::path& caseRoot) override;
    std::expected<core::application::domain::Case, core::application::contracts::CaseError> loadCase(const std::filesystem::path& caseRoot) override;
    std::expected<void, core::application::contracts::CaseError> saveCase(const core::application::domain::Case& caseObj, const std::filesystem::path& caseRoot) override;

private:
    struct DatabaseConnection {
        sqlite3* db{nullptr};
        ~DatabaseConnection() {
            if (db) sqlite3_close(db);
        }
    };

    std::expected<void, core::application::contracts::CaseError> initSchema(sqlite3* db);
    std::expected<void, core::application::contracts::CaseError> verifySchemaVersion(sqlite3* db);
    std::expected<void, core::application::contracts::CaseError> executeStatement(sqlite3* db, const std::string& sql);
    std::expected<void, core::application::contracts::CaseError> beginTransaction(sqlite3* db);
    std::expected<void, core::application::contracts::CaseError> commitTransaction(sqlite3* db);
    std::expected<void, core::application::contracts::CaseError> rollbackTransaction(sqlite3* db);
};

} // namespace fvm::infrastructure::sqlite
