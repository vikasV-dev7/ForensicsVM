#include "SqliteEvidenceRepository.hpp"
#include <sqlite3.h>
#include <iostream>

namespace fvm::infrastructure::sqlite {

SqliteEvidenceRepository::SqliteEvidenceRepository(std::shared_ptr<DatabaseContext> dbContext)
    : dbContext_(std::move(dbContext)) {
}

std::expected<void, fvm::contracts::EvidenceError> SqliteEvidenceRepository::save(const domain::EvidenceRecord& record) {
    sqlite3* db = dbContext_->getDb();
    if (!db) return std::unexpected(fvm::contracts::EvidenceError::StorageError);

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT OR REPLACE INTO evidence_records (id, hash, path, format, size_bytes, status) VALUES (?, ?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::unexpected(fvm::contracts::EvidenceError::StorageError);
    }

    sqlite3_bind_text(stmt, 1, record.id().value().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, record.sha256().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, record.path().string().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, static_cast<int>(record.format()));
    sqlite3_bind_int64(stmt, 5, record.sizeBytes());
    sqlite3_bind_int(stmt, 6, static_cast<int>(record.status()));

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return std::unexpected(fvm::contracts::EvidenceError::StorageError);
    }
    sqlite3_finalize(stmt);
    return {};
}

std::optional<domain::EvidenceRecord> SqliteEvidenceRepository::find(const domain::EvidenceId& id) const {
    sqlite3* db = dbContext_->getDb();
    if (!db) return std::nullopt;

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT path, format, size_bytes, status, hash FROM evidence_records WHERE id = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, id.value().c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::filesystem::path path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        auto format = static_cast<domain::DiskFormat>(sqlite3_column_int(stmt, 1));
        std::uintmax_t sizeBytes = sqlite3_column_int64(stmt, 2);
        auto status = static_cast<domain::EvidenceStatus>(sqlite3_column_int(stmt, 3));
        std::string hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));

        domain::EvidenceRecord rec(id, path, format, sizeBytes);
        if (status == domain::EvidenceStatus::Verified) rec.setVerified(hash);
        else if (status == domain::EvidenceStatus::Missing) rec.setMissing();
        else if (status == domain::EvidenceStatus::Failed) rec.setFailed();

        sqlite3_finalize(stmt);
        return rec;
    }
    
    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::expected<void, fvm::contracts::EvidenceError> SqliteEvidenceRepository::remove(const domain::EvidenceId& id) {
    sqlite3* db = dbContext_->getDb();
    if (!db) return std::unexpected(fvm::contracts::EvidenceError::StorageError);

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM evidence_records WHERE id = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::unexpected(fvm::contracts::EvidenceError::StorageError);
    }

    sqlite3_bind_text(stmt, 1, id.value().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return {};
}

std::vector<domain::EvidenceRecord> SqliteEvidenceRepository::listAll() const {
    sqlite3* db = dbContext_->getDb();
    if (!db) return {};

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id FROM evidence_records;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return {};
    }

    std::vector<domain::EvidenceRecord> records;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        domain::EvidenceId id(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        if (auto rec = find(id)) {
            records.push_back(std::move(*rec));
        }
    }
    sqlite3_finalize(stmt);
    return records;
}

} // namespace fvm::infrastructure::sqlite
