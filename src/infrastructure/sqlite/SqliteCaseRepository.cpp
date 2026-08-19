#include "SqliteCaseRepository.hpp"
#include <iostream>

namespace fvm::infrastructure::sqlite {

namespace {
    const int SCHEMA_VERSION = 1;
}

SqliteCaseRepository::SqliteCaseRepository() {}

SqliteCaseRepository::~SqliteCaseRepository() {}

std::expected<void, core::application::contracts::CaseError> SqliteCaseRepository::executeStatement(sqlite3* db, const std::string& sql) {
    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string errStr(errMsg ? errMsg : "Unknown SQLite error");
        if (errMsg) sqlite3_free(errMsg);
        std::cerr << "SQLite Exec Error: " << errStr << "\n";
        return std::unexpected(core::application::contracts::CaseError::DatabaseCorruption);
    }
    return {};
}

std::expected<void, core::application::contracts::CaseError> SqliteCaseRepository::beginTransaction(sqlite3* db) {
    return executeStatement(db, "BEGIN TRANSACTION;");
}

std::expected<void, core::application::contracts::CaseError> SqliteCaseRepository::commitTransaction(sqlite3* db) {
    return executeStatement(db, "COMMIT;");
}

std::expected<void, core::application::contracts::CaseError> SqliteCaseRepository::rollbackTransaction(sqlite3* db) {
    return executeStatement(db, "ROLLBACK;");
}

std::expected<void, core::application::contracts::CaseError> SqliteCaseRepository::initSchema(sqlite3* db) {
    auto res = executeStatement(db, R"(
        CREATE TABLE IF NOT EXISTS metadata (
            id TEXT PRIMARY KEY,
            name TEXT,
            description TEXT,
            investigator TEXT,
            created_at INTEGER,
            updated_at INTEGER
        );
        CREATE TABLE IF NOT EXISTS evidence (
            id TEXT PRIMARY KEY
        );
        CREATE TABLE IF NOT EXISTS sessions (
            id TEXT PRIMARY KEY
        );
    )");
    if (!res) return res;

    std::string versionSql = "PRAGMA user_version = " + std::to_string(SCHEMA_VERSION) + ";";
    return executeStatement(db, versionSql);
}

std::expected<void, core::application::contracts::CaseError> SqliteCaseRepository::verifySchemaVersion(sqlite3* db) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "PRAGMA user_version;", -1, &stmt, nullptr) != SQLITE_OK) {
        return std::unexpected(core::application::contracts::CaseError::DatabaseCorruption);
    }
    
    int version = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        version = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (version != 0 && version != SCHEMA_VERSION) {
        return std::unexpected(core::application::contracts::CaseError::SchemaMismatch);
    }
    return {};
}

std::expected<void, core::application::contracts::CaseError> SqliteCaseRepository::createCase(const core::application::domain::Case& caseObj, const std::filesystem::path& caseRoot) {
    std::filesystem::path dbPath = caseRoot / "case.fvmcase";
    if (std::filesystem::exists(dbPath)) {
        return std::unexpected(core::application::contracts::CaseError::Duplicate);
    }

    DatabaseConnection conn;
    if (sqlite3_open(dbPath.string().c_str(), &conn.db) != SQLITE_OK) {
        return std::unexpected(core::application::contracts::CaseError::PersistenceFailure);
    }

    auto schemaRes = initSchema(conn.db);
    if (!schemaRes) return schemaRes;

    return saveCase(caseObj, caseRoot);
}

std::expected<core::application::domain::Case, core::application::contracts::CaseError> SqliteCaseRepository::loadCase(const std::filesystem::path& caseRoot) {
    std::filesystem::path dbPath = caseRoot / "case.fvmcase";
    if (!std::filesystem::exists(dbPath)) {
        return std::unexpected(core::application::contracts::CaseError::NotFound);
    }

    DatabaseConnection conn;
    if (sqlite3_open(dbPath.string().c_str(), &conn.db) != SQLITE_OK) {
        return std::unexpected(core::application::contracts::CaseError::PersistenceFailure);
    }

    auto verRes = verifySchemaVersion(conn.db);
    if (!verRes) return std::unexpected(verRes.error());

    core::application::domain::CaseMetadata meta;
    std::string caseIdStr;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(conn.db, "SELECT id, name, description, investigator, created_at, updated_at FROM metadata LIMIT 1;", -1, &stmt, nullptr) == SQLITE_OK) {
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
    if (sqlite3_prepare_v2(conn.db, "SELECT id FROM evidence;", -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string evIdStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            loadedCase.addEvidenceId(fvm::domain::EvidenceId(evIdStr));
        }
        sqlite3_finalize(stmt);
    }

    // Load sessions
    if (sqlite3_prepare_v2(conn.db, "SELECT id FROM sessions;", -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string vmIdStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            loadedCase.addVmId(fvm::domain::VmId(vmIdStr));
        }
        sqlite3_finalize(stmt);
    }

    return loadedCase;
}

std::expected<void, core::application::contracts::CaseError> SqliteCaseRepository::saveCase(const core::application::domain::Case& caseObj, const std::filesystem::path& caseRoot) {
    std::filesystem::path dbPath = caseRoot / "case.fvmcase";
    DatabaseConnection conn;
    if (sqlite3_open(dbPath.string().c_str(), &conn.db) != SQLITE_OK) {
        return std::unexpected(core::application::contracts::CaseError::PersistenceFailure);
    }

    auto transRes = beginTransaction(conn.db);
    if (!transRes) return transRes;

    sqlite3_stmt* stmt = nullptr;
    const char* metaSql = "INSERT OR REPLACE INTO metadata (id, name, description, investigator, created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(conn.db, metaSql, -1, &stmt, nullptr) != SQLITE_OK) {
        rollbackTransaction(conn.db);
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
        rollbackTransaction(conn.db);
        return std::unexpected(core::application::contracts::CaseError::PersistenceFailure);
    }
    sqlite3_finalize(stmt);

    // Save evidence
    executeStatement(conn.db, "DELETE FROM evidence;");
    const char* evSql = "INSERT INTO evidence (id) VALUES (?);";
    if (sqlite3_prepare_v2(conn.db, evSql, -1, &stmt, nullptr) == SQLITE_OK) {
        for (const auto& evId : caseObj.getEvidenceIds()) {
            sqlite3_bind_text(stmt, 1, evId.value().c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                sqlite3_finalize(stmt);
                rollbackTransaction(conn.db);
                return std::unexpected(core::application::contracts::CaseError::PersistenceFailure);
            }
            sqlite3_reset(stmt);
        }
        sqlite3_finalize(stmt);
    }

    // Save sessions
    executeStatement(conn.db, "DELETE FROM sessions;");
    const char* sessSql = "INSERT INTO sessions (id) VALUES (?);";
    if (sqlite3_prepare_v2(conn.db, sessSql, -1, &stmt, nullptr) == SQLITE_OK) {
        for (const auto& vmId : caseObj.getVmIds()) {
            sqlite3_bind_text(stmt, 1, vmId.value().c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                sqlite3_finalize(stmt);
                rollbackTransaction(conn.db);
                return std::unexpected(core::application::contracts::CaseError::PersistenceFailure);
            }
            sqlite3_reset(stmt);
        }
        sqlite3_finalize(stmt);
    }

    return commitTransaction(conn.db);
}

} // namespace fvm::infrastructure::sqlite
