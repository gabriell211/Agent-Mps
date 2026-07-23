#pragma once
#include <sqlite3.h>
#include <string>
#include <functional>
#include <vector>
#include <stdexcept>

class Database {
public:
    Database() = default;
    ~Database();

    void open(const std::string& path);
    void close();

    void exec(const std::string& sql);

    void query(const std::string& sql, std::function<bool(sqlite3_stmt*)> fn);

    sqlite3_stmt* prepare(const std::string& sql);
    void          step(sqlite3_stmt* stmt);        // roda e verifica DONE
    void          finalize(sqlite3_stmt* stmt);
    void          reset(sqlite3_stmt* stmt);

    long long last_insert_id();

    void begin();
    void commit();
    void rollback();

    bool is_open() const { return db != nullptr; }

    sqlite3* handle() { return db; }

private:
    sqlite3* db = nullptr;

    void check(int rc, const std::string& ctx = "");
};
