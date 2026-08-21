#include "DatabaseContext.hpp"
#include "CanonicalJsonSerializer.hpp"
#include <iostream>

namespace fvm::infrastructure::sqlite {

DatabaseContext::DatabaseContext(std::shared_ptr<fvm::contracts::IHashCalculator> hasher)
    : hasher_(std::move(hasher)) {}

DatabaseContext::~DatabaseContext() {
    close();
}

std::expected<void, core::application::contracts::CaseError> DatabaseContext::executeStatement(const std::string& sql) {
    char* errMsg = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string errStr(errMsg ? errMsg : "Unknown SQLite error");
        if (errMsg) sqlite3_free(errMsg);
        std::cerr << "SQLite Exec Error: " << errStr << "\n";
        return std::unexpected(core::application::contracts::CaseError::DatabaseCorruption);
    }
    return {};
}

std::expected<void, core::application::contracts::CaseError> DatabaseContext::open(const std::filesystem::path& path, bool initializeSchema) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (db_) {
        close();
    }
    dbPath_ = path;
    if (sqlite3_open(dbPath_.string().c_str(), &db_) != SQLITE_OK) {
        return std::unexpected(core::application::contracts::CaseError::PersistenceFailure);
    }

    if (initializeSchema) {
        // Init minimal schema for audit log here. The rest is done by Repositories.
        auto res = executeStatement(R"(
            CREATE TABLE IF NOT EXISTS audit_log (
                seq_num INTEGER PRIMARY KEY AUTOINCREMENT,
                event_id TEXT,
                timestamp INTEGER,
                event_type TEXT,
                payload TEXT,
                previous_hash TEXT,
                current_hash TEXT
            );
        )");
        if (!res) return res;
    }

    // Verify hash chain if not initializing
    if (!initializeSchema) {
        auto verRes = verifyHashChain();
        if (!verRes) return verRes;
    }

    return {};
}

void DatabaseContext::close() {
    if (db_) {
        sqlite3_close_v2(db_);
        db_ = nullptr;
    }
}

std::expected<void, core::application::contracts::CaseError> DatabaseContext::verifyHashChain() {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT seq_num, payload, previous_hash, current_hash FROM audit_log ORDER BY seq_num ASC;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::unexpected(core::application::contracts::CaseError::DatabaseCorruption);
    }

    std::string expectedPrevHash = "GENESIS";
    bool hasRows = false;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        hasRows = true;
        std::string payload = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string prevHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string currHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

        if (prevHash != expectedPrevHash) {
            sqlite3_finalize(stmt);
            return std::unexpected(core::application::contracts::CaseError::DatabaseCorruption);
        }

        std::string dataToHash = payload + prevHash;
        auto hashRes = hasher_->calculateSha256(dataToHash);
        if (!hashRes || *hashRes != currHash) {
            sqlite3_finalize(stmt);
            return std::unexpected(core::application::contracts::CaseError::DatabaseCorruption);
        }
        
        expectedPrevHash = currHash;
    }
    sqlite3_finalize(stmt);
    
    if (!hasRows) {
        return std::unexpected(core::application::contracts::CaseError::DatabaseCorruption);
    }
    return {};
}

std::expected<std::string, core::application::contracts::CaseError> DatabaseContext::getLastHash() {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT current_hash FROM audit_log ORDER BY seq_num DESC LIMIT 1;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::unexpected(core::application::contracts::CaseError::DatabaseCorruption);
    }

    std::string lastHash = "GENESIS";
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        lastHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    }
    sqlite3_finalize(stmt);
    return lastHash;
}

std::string DatabaseContext::canonicalizePayload(const core::application::domain::AuditRecord& audit) {
    if (!audit.payloadFields.empty()) {
        std::map<std::string, JsonValue> jsonFields;
        for (const auto& [k, v] : audit.payloadFields) {
            if (std::holds_alternative<std::string>(v)) {
                jsonFields[k] = std::get<std::string>(v);
            } else if (std::holds_alternative<int64_t>(v)) {
                jsonFields[k] = std::get<int64_t>(v);
            } else {
                jsonFields[k] = nullptr;
            }
        }
        return CanonicalJsonSerializer::serialize(jsonFields);
    }
    return audit.payload;
}

std::expected<void, core::application::contracts::CaseError> DatabaseContext::beginTransaction() {
    return executeStatement("BEGIN EXCLUSIVE TRANSACTION;");
}

std::expected<void, core::application::contracts::CaseError> DatabaseContext::commitTransaction(const core::application::domain::AuditRecord& audit) {
    auto lastHashRes = getLastHash();
    if (!lastHashRes) {
        rollbackTransaction();
        return std::unexpected(lastHashRes.error());
    }

    std::string prevHash = *lastHashRes;
    std::string canonicalPayload = canonicalizePayload(audit);
    std::string dataToHash = canonicalPayload + prevHash;
    
    auto hashRes = hasher_->calculateSha256(dataToHash);
    if (!hashRes) {
        rollbackTransaction();
        return std::unexpected(core::application::contracts::CaseError::PersistenceFailure);
    }
    std::string currentHash = *hashRes;

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO audit_log (event_id, timestamp, event_type, payload, previous_hash, current_hash) VALUES (?, ?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        rollbackTransaction();
        return std::unexpected(core::application::contracts::CaseError::DatabaseCorruption);
    }

    sqlite3_bind_text(stmt, 1, audit.eventId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, audit.timestampUnixMs);
    sqlite3_bind_text(stmt, 3, audit.eventType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, canonicalPayload.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, prevHash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, currentHash.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        rollbackTransaction();
        return std::unexpected(core::application::contracts::CaseError::PersistenceFailure);
    }
    sqlite3_finalize(stmt);

    return executeStatement("COMMIT;");
}

std::expected<void, core::application::contracts::CaseError> DatabaseContext::rollbackTransaction() {
    return executeStatement("ROLLBACK;");
}

} // namespace fvm::infrastructure::sqlite
