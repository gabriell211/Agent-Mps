#pragma once

#include "../core/Config.hpp"
#include "../models/Alert.hpp"
#include "../models/Printer.hpp"

class EmailNotifier {
public:
    explicit EmailNotifier(const SmtpConfig& cfg) : cfg(cfg) {}
    void notify(const Alert& alert, const Printer& printer) const;

private:
    SmtpConfig cfg;
};
