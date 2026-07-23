#pragma once
#include <string>

struct Alert {
    int         id           = 0;
    int         printer_id   = 0;
    std::string severity;    
    std::string type;      
    std::string message;
    bool        acknowledged = false;
    long long   created_at   = 0;
    long long   resolved_at  = 0;
};
