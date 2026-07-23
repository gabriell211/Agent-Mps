#pragma once

struct Counter {
    int       printer_id      = 0;
    long long total_pages     = 0;
    long long mono_pages      = 0;
    long long color_pages     = 0;
    long long simplex_pages   = 0;
    long long duplex_pages    = 0;
    long long copy_pages      = 0;
    long long print_pages     = 0;
    long long scan_pages      = 0;
    long long fax_pages       = 0;
    long long adf_pages       = 0;
    long long usb_pages       = 0;
    long long net_pages       = 0;
    long long secure_pages    = 0;
    long long collected_at    = 0;
};
