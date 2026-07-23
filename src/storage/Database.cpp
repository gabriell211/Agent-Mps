#include "Database.hpp"
#include <stdexcept>
#include <string>

Database::~Database() {
    close();
}

void Database::open(const std::string& path) {
    int rc = sqlite3_open(path.c_str(), &db);
    check(rc, "open " + path);
    exec("PRAGMA journal_mode=WAL");
    exec("PRAGMA synchronous=NORMAL");
    exec("PRAGMA foreign_keys=ON");
    exec("PRAGMA cache_size=-8000");  
}

void Database::close() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

void Database::check(int rc, const std::string& ctx) {
    if (rc != SQLITE_OK && rc != SQLITE_ROW && rc != SQLITE_DONE) {
        std::string msg = ctx + ": " + (db ? sqlite3_errmsg(db) : "sem db");
        throw std::runtime_error(msg);
    }
}

void Database::exec(const std::string& sql) {
    char* err = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::string msg = std::string(err ? err : "erro desconhecido");
        sqlite3_free(err);
        throw std::runtime_error("exec: " + msg + " | sql: " + sql.substr(0, 80));
    }
}

void Database::query(const std::string& sql, std::function<bool(sqlite3_stmt*)> fn) {
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    check(rc, "prepare");
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (!fn(stmt)) break;
    }
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) check(rc, "step");
}

sqlite3_stmt* Database::prepare(const std::string& sql) {
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    check(rc, "prepare");
    return stmt;
}

void Database::step(sqlite3_stmt* stmt) {
    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW)
        check(rc, "step");
}

void Database::finalize(sqlite3_stmt* stmt) {
    if (stmt) sqlite3_finalize(stmt);
}

void Database::reset(sqlite3_stmt* stmt) {
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
}

long long Database::last_insert_id() {
    return sqlite3_last_insert_rowid(db);
}

void Database::begin()    { exec("BEGIN"); }
void Database::commit()   { exec("COMMIT"); }
void Database::rollback() { exec("ROLLBACK"); }
