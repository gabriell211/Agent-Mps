#pragma once
#include "Database.hpp"

class Migrations {
public:
    static void run(Database& db);

private:
    static int  current_version(Database& db);
    static void set_version(Database& db, int v);

    static void v1_create_tables(Database& db);
    static void v2_add_security(Database& db);
    static void v3_add_jobs(Database& db);
    static void v4_add_transport_fields(Database& db);
    static void v5_add_events_and_logs(Database& db);
};
