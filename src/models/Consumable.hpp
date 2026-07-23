#pragma once
#include <string>

struct Consumable {
    int         id            = 0;
    int         printer_id    = 0;
    std::string name;           
    std::string type;            
    std::string color;           
    int         capacity_pages  = 0;
    int         remaining_pages = 0;
    int         level_pct       = -1;   
    std::string state;           
    long long   collected_at    = 0;
};
