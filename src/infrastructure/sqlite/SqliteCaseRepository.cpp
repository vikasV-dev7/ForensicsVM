#include "SqliteCaseRepository.hpp"
#include <iostream>

namespace fvm::infrastructure::sqlite {

namespace {
    const int SCHEMA_VERSION = 2; // Incremented for Phase 8
}

SqliteCaseRepository::SqliteCaseRepository(std::shared_ptr<DatabaseContext> dbContext) : dbContext_(std::move(dbContext)) {}

SqliteCaseRepository::~SqliteCaseRepository() {}

std::expected<void, core::application::contracts::CaseError> SqliteCaseRepository::initSchema() {
    sqlite3* db = dbContext_->getDb();
    if (!db) return std::unexpected(core::application::contracts::CaseError::DatabaseCorruption);

    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS metadata (
            id TEXT PRIMARY KEY,
            name TEXT,
            description TEXT,
            investigator TEXT,
            created_at INTEGER,
            updated_at INTEGER
        );
        CREATE TABLE IF NOT EXISTS case_evidence (
            id TEXT PRIMARY KEY
        );
        CREATE TABLE IF NOT EXISTS case_sessions (
            id TEXT PRIMARY KEY
        );
        CREATE TABLE IF NOT EXISTS evidence_records (
            id TEXT PRIMARY KEY,
            hash TEXT,
            path TEXT,
            format INTEGER,
            size_bytes INTEGER,
            status INTEGER
        );
        CREATE TABLE IF NOT EXISTS vm_configs (
            id TEXT PRIMARY KEY,
            name TEXT,
            state INTEGER
        );
    )";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        if (errMsg) sqlite3_free(errMsg);
        return std::unexpected(core::application::contracts::CaseError::DatabaseCorruption);
    }

    std::string versionSql = "PRAGMA user_version = " + std::to_string(SCHEMA_VERSION) + ";";
    if (sqlite3_exec(db, versionSql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        if (errMsg) sqlite3_free(errMsg);
        return std::unexpected(core::application::contracts::CaseError::DatabaseCorruption);
    }

    return {};
}

std::expected<void, core::application::contracts::CaseError> SqliteCaseRepository::verifySchemaVersion() {
    sqlite3* db = dbContext_->getDb();
    if (!db) return std::unexpected(core::application::contracts::CaseError::DatabaseCorruption);

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "PRAGMA user_version;", -1, &stmt, nullptr) != SQLITE_OK) {
        return std::unexpected(core::application::contracts::CaseError::DatabaseCorruption);
    }
    
    int version = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        version = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (version != 0 && version != SCHEMA_VERSION && version != 1) { // Accept v1 for backward compatibility in mock testing if needed
        return std::unexpected(core::application::contracts::CaseError::SchemaMismatch);
    }
    return {};
}

std::expected<void, core::application::contracts::CaseError> SqliteCaseRepository::createCase(const core::application::domain::Case& caseObj, const std::filesystem::path& caseRoot) {
    std::filesystem::path dbPath = caseRoot / "case.fvmcase";
    if (std::filesystem::exists(dbPath)) {
        return std::unexpected(core::application::contracts::CaseError::Duplicate);
    }

    auto openRes = dbContext_->open(dbPath, true);
    if (!openRes) return openRes;

    auto schemaRes = initSchema();
    if (!schemaRes) return schemaRes;

    return saveCase(caseObj, caseRoot);
}

std::expected<core::application::domain::Case, core::application::contracts::CaseError> SqliteCaseRepository::loadCase(const std::filesystem::path& caseRoot) {
    std::filesystem::path dbPath = caseRoot / "case.fvmcase";
    if (!std::filesystem::exists(dbPath)) {
        return std::unexpected(core::application::contracts::CaseError::NotFound);
    }

    auto openRes = dbContext_->open(dbPath, false);
    if (!openRes) return std::unexpected(openRes.error());

    auto verRes = verifySchemaVersion();
    if (!verRes) return std::unexpected(verRes.error());

    sqlite3* db = dbContext_->getDb();
    core::application::domain::CaseMetadata meta;
    std::string caseIdStr;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT id, name, description, investigator, created_at, updated_at FROM metadata LIMIT 1;", -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            caseIdStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            meta.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            meta.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            meta.investigator = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            meta.createdAtUnixSeconds = sqlite3_column_int64(stmt, 4);
            meta.updatedAtUnixSeconds = sqlite3_column_int64(stmt, 5);
        } else {
            sqlite3_finalize(stmt);
            return std::unexpected(core::application::contracts::CaseError::Malformed);
        }
        sqlite3_finalize(stmt);
    } else {
        return std::unexpected(core::application::contracts::CaseError::DatabaseCorruption);
    }

    core::application::domain::Case loadedCase(core::application::domain::CaseId(caseIdStr), meta);

    // Load evidence
    if (sqlite3_prepare_v2(db, "SELECT id FROM case_evidence;", -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string evIdStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            loadedCase.addEvidenceId(fvm::domain::EvidenceId(evIdStr));
        }
        sqlite3_finalize(stmt);
    }

    // Load sessions
    if (sqlite3_prepare_v2(db, "SELECT id FROM case_sessions;", -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string vmIdStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            loadedCase.addVmId(fvm::domain::VmId(vmIdStr));
        }
        sqlite3_finalize(stmt);
    }

    return loadedCase;
}

std::expected<void, core::application::contracts::CaseError> SqliteCaseRepository::saveCase(const core::application::domain::Case& caseObj, const std::filesystem::path& caseRoot) {
    (void)caseRoot;
    sqlite3* db = dbContext_->getDb();
    if (!db) return std::unexpected(core::application::contracts::CaseError::PersistenceFailure);

    sqlite3_stmt* stmt = nullptr;
    const char* metaSql = "INSERT OR REPLACE INTO metadata (id, name, description, investigator, created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db, metaSql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::unexpected(core::application::contracts::CaseError::DatabaseCorruption);
    }
    
    sqlite3_bind_text(stmt, 1, caseObj.getId().value().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, caseObj.getMetadata().name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, caseObj.getMetadata().description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, caseObj.getMetadata().investigator.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 5, caseObj.getMetadata().createdAtUnixSeconds);
    sqlite3_bind_int64(stmt, 6, caseObj.getMetadata().updatedAtUnixSeconds);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return std::unexpected(core::application::contracts::CaseError::PersistenceFailure);
    }
    sqlite3_finalize(stmt);

    // Save evidence
    sqlite3_exec(db, "DELETE FROM case_evidence;", nullptr, nullptr, nullptr);
    const char* evSql = "INSERT INTO case_evidence (id) VALUES (?);";
    if (sqlite3_prepare_v2(db, evSql, -1, &stmt, nullptr) == SQLITE_OK) {
        for (const auto& evId : caseObj.getEvidenceIds()) {
            sqlite3_bind_text(stmt, 1, evId.value().c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                sqlite3_finalize(stmt);
                return std::unexpected(core::application::contracts::CaseError::PersistenceFailure);
            }
            sqlite3_reset(stmt);
        }
        sqlite3_finalize(stmt);
    }

    // Save sessions
    sqlite3_exec(db, "DELETE FROM case_sessions;", nullptr, nullptr, nullptr);
    const char* sessSql = "INSERT INTO case_sessions (id) VALUES (?);";
    if (sqlite3_prepare_v2(db, sessSql, -1, &stmt, nullptr) == SQLITE_OK) {
        for (const auto& vmId : caseObj.getVmIds()) {
            sqlite3_bind_text(stmt, 1, vmId.value().c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                sqlite3_finalize(stmt);
                return std::unexpected(core::application::contracts::CaseError::PersistenceFailure);
            }
            sqlite3_reset(stmt);
        }
        sqlite3_finalize(stmt);
    }

    return {};
}

std::expected<void, core::application::contracts::CaseError> SqliteCaseRepository::beginTransaction() {
    return dbContext_->beginTransaction();
}

std::expected<void, core::application::contracts::CaseError> SqliteCaseRepository::commitTransaction(const core::application::domain::AuditRecord& audit) {
    return dbContext_->commitTransaction(audit);
}

std::expected<void, core::application::contracts::CaseError> SqliteCaseRepository::rollbackTransaction() {
    return dbContext_->rollbackTransaction();
}

} // namespace fvm::infrastructure::sqlite
