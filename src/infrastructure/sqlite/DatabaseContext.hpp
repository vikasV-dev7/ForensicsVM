#pragma once
#include "core/application/domain/AuditRecord.hpp"
#include "core/application/contracts/ICaseRepository.hpp"
#include "vm/contracts/IHashCalculator.hpp"
#include <sqlite3.h>
#include <filesystem>
#include <expected>
#include <memory>
#include <mutex>
#include <string>

namespace fvm::infrastructure::sqlite {

class DatabaseContext {
    sqlite3* db_{nullptr};
    std::filesystem::path dbPath_;
    std::mutex dbMutex_;
    std::shared_ptr<fvm::contracts::IHashCalculator> hasher_;

    std::expected<void, core::application::contracts::CaseError> executeStatement(const std::string& sql);
    std::expected<std::string, core::application::contracts::CaseError> getLastHash();
    std::expected<void, core::application::contracts::CaseError> verifyHashChain();
    std::string canonicalizePayload(const core::application::domain::AuditRecord& audit);

public:
    explicit DatabaseContext(std::shared_ptr<fvm::contracts::IHashCalculator> hasher);
    ~DatabaseContext();

    std::expected<void, core::application::contracts::CaseError> open(const std::filesystem::path& path, bool initializeSchema);
    void close();

    sqlite3* getDb() const { return db_; }
    std::mutex& getMutex() { return dbMutex_; }

    std::expected<void, core::application::contracts::CaseError> beginTransaction();
    std::expected<void, core::application::contracts::CaseError> commitTransaction(const core::application::domain::AuditRecord& audit);
    std::expected<void, core::application::contracts::CaseError> rollbackTransaction();
};

} // namespace fvm::infrastructure::sqlite
