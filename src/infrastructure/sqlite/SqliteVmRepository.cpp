#include "SqliteVmRepository.hpp"
#include <sqlite3.h>
#include <iostream>

namespace fvm::infrastructure::sqlite {

SqliteVmRepository::SqliteVmRepository(std::shared_ptr<DatabaseContext> dbContext)
    : dbContext_(std::move(dbContext)) {
}

domain::Result<void> SqliteVmRepository::save(const domain::VmConfig& config, domain::VmState state) {
    sqlite3* db = dbContext_->getDb();
    if (!db) return std::unexpected(domain::VmError::OperationFailed);

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT OR REPLACE INTO vm_configs (id, name, state) VALUES (?, ?, ?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::unexpected(domain::VmError::OperationFailed);
    }

    sqlite3_bind_text(stmt, 1, config.id.value().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, config.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, static_cast<int>(state));

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return std::unexpected(domain::VmError::OperationFailed);
    }
    sqlite3_finalize(stmt);
    return {};
}

domain::Result<void> SqliteVmRepository::remove(const domain::VmId& id) {
    sqlite3* db = dbContext_->getDb();
    if (!db) return std::unexpected(domain::VmError::OperationFailed);

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM vm_configs WHERE id = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::unexpected(domain::VmError::OperationFailed);
    }

    sqlite3_bind_text(stmt, 1, id.value().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return {};
}

domain::Result<domain::VmConfig> SqliteVmRepository::findConfig(const domain::VmId& id) const {
    sqlite3* db = dbContext_->getDb();
    if (!db) return std::unexpected(domain::VmError::OperationFailed);

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT name FROM vm_configs WHERE id = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::unexpected(domain::VmError::OperationFailed);
    }

    sqlite3_bind_text(stmt, 1, id.value().c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        domain::VmConfig config{
            id,
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)),
            "",
            domain::CpuConfig{domain::CpuCount(1)},
            domain::MemoryConfig{domain::Megabytes(1024)},
            {}, {},
            domain::FirmwareConfig{domain::FirmwareType::BIOS},
            domain::DisplayConfig{}
        };
        sqlite3_finalize(stmt);
        return config;
    }
    
    sqlite3_finalize(stmt);
    return std::unexpected(domain::VmError::VmNotFound);
}

domain::Result<domain::VmState> SqliteVmRepository::findState(const domain::VmId& id) const {
    sqlite3* db = dbContext_->getDb();
    if (!db) return std::unexpected(domain::VmError::OperationFailed);

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT state FROM vm_configs WHERE id = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::unexpected(domain::VmError::OperationFailed);
    }

    sqlite3_bind_text(stmt, 1, id.value().c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        domain::VmState state = static_cast<domain::VmState>(sqlite3_column_int(stmt, 0));
        sqlite3_finalize(stmt);
        return state;
    }
    
    sqlite3_finalize(stmt);
    return std::unexpected(domain::VmError::VmNotFound);
}

domain::Result<std::vector<domain::VmId>> SqliteVmRepository::listAll() const {
    sqlite3* db = dbContext_->getDb();
    if (!db) return std::unexpected(domain::VmError::OperationFailed);

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id FROM vm_configs;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::unexpected(domain::VmError::OperationFailed);
    }

    std::vector<domain::VmId> ids;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ids.emplace_back(domain::VmId(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));
    }
    sqlite3_finalize(stmt);
    return ids;
}

} // namespace fvm::infrastructure::sqlite
