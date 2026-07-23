#include "UsbCollector.hpp"
#include "../core/Logger.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <winspool.h>
#endif

namespace {
long long now_epoch() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string narrow(const wchar_t* value) {
#ifdef _WIN32
    if (!value) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) return {};
    std::string text(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value, -1, text.data(), size, nullptr, nullptr);
    text.pop_back();
    return text;
#else
    (void)value;
    return {};
#endif
}

bool is_usb_port(std::string port) {
    std::transform(port.begin(), port.end(), port.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return port.rfind("USB", 0) == 0 || port.rfind("DOT4", 0) == 0;
}

std::string endpoint_key(const std::string& port, const std::string& name) {
    std::string key = port + "/" + name;
    for (char& c : key) {
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '/')) c = '_';
    }
    return "usb://" + key;
}
}

std::vector<Printer> UsbCollector::discover() const {
    std::vector<Printer> printers;
#ifdef _WIN32
    DWORD needed = 0;
    DWORD returned = 0;
    const DWORD flags = PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS;
    EnumPrintersW(flags, nullptr, 2, nullptr, 0, &needed, &returned);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || needed == 0) return printers;

    std::vector<BYTE> buffer(needed);
    if (!EnumPrintersW(flags, nullptr, 2, buffer.data(), needed, &needed, &returned)) {
        Log::warn("usb: EnumPrinters falhou: {}", static_cast<unsigned long>(GetLastError()));
        return printers;
    }

    auto* entries = reinterpret_cast<PRINTER_INFO_2W*>(buffer.data());
    for (DWORD i = 0; i < returned; ++i) {
        const std::string port = narrow(entries[i].pPortName);
        if (!is_usb_port(port)) continue;

        Printer p;
        p.connection_type = "usb";
        p.port_name = port;
        p.hostname = narrow(entries[i].pPrinterName);
        p.friendly_name = p.hostname;
        p.model = narrow(entries[i].pDriverName);
        p.manufacturer = "Local USB";
        p.ip = endpoint_key(port, p.hostname);
        p.last_seen = now_epoch();
        printers.push_back(std::move(p));
    }
#else
    FILE* pipe = popen("lpstat -v 2>/dev/null", "r");
    if (!pipe) return printers;
    char line[2048]{};
    while (std::fgets(line, sizeof(line), pipe)) {
        const std::string value(line);
        const auto prefix = value.find("device for ");
        const auto separator = value.find(": ");
        if (prefix != 0 || separator == std::string::npos) continue;
        const std::string queue = value.substr(11, separator - 11);
        std::string uri = value.substr(separator + 2);
        while (!uri.empty() && std::isspace(static_cast<unsigned char>(uri.back()))) uri.pop_back();
        std::string lower = uri;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (lower.rfind("usb://", 0) != 0 && lower.rfind("parallel:", 0) != 0) continue;
        Printer p;
        p.connection_type = "usb";
        p.port_name = uri;
        p.hostname = queue;
        p.friendly_name = queue;
        p.model = queue;
        p.manufacturer = "CUPS USB";
        p.ip = endpoint_key(uri, queue);
        p.last_seen = now_epoch();
        printers.push_back(std::move(p));
    }
    pclose(pipe);
#endif
    return printers;
}

PrinterStatus UsbCollector::collect_status(const Printer& printer) const {
    PrinterStatus status;
    status.printer_id = printer.id;
    status.status = "unknown";
    status.collected_at = now_epoch();

#ifdef _WIN32
    std::wstring queue;
    if (!printer.hostname.empty()) {
        int size = MultiByteToWideChar(CP_UTF8, 0, printer.hostname.c_str(), -1, nullptr, 0);
        if (size > 1) {
            queue.resize(static_cast<size_t>(size));
            MultiByteToWideChar(CP_UTF8, 0, printer.hostname.c_str(), -1, queue.data(), size);
        }
    }
    HANDLE handle = nullptr;
    if (queue.empty() || !OpenPrinterW(queue.data(), &handle, nullptr)) {
        status.status = "offline";
        status.status_message = "fila USB local indisponivel";
        return status;
    }

    DWORD needed = 0;
    GetPrinterW(handle, 2, nullptr, 0, &needed);
    std::vector<BYTE> buffer(needed);
    if (needed == 0 || !GetPrinterW(handle, 2, buffer.data(), needed, &needed)) {
        ClosePrinter(handle);
        status.status = "unknown";
        status.status_message = "nao foi possivel ler a fila USB";
        return status;
    }
    ClosePrinter(handle);
    auto* info = reinterpret_cast<PRINTER_INFO_2W*>(buffer.data());
    if (info->Status & PRINTER_STATUS_OFFLINE) status.status = "offline";
    else if (info->Status & PRINTER_STATUS_PRINTING) status.status = "printing";
    else if (info->Status & PRINTER_STATUS_WARMING_UP) status.status = "warming";
    else if (info->Status & PRINTER_STATUS_ERROR) status.status = "error";
    else status.status = "idle";
    status.status_message = narrow(info->pStatus);
#else
    FILE* pipe = popen("lpstat -p 2>/dev/null", "r");
    if (!pipe) {
        status.status = "offline";
        status.status_message = "CUPS indisponivel";
        return status;
    }
    char line[2048]{};
    const std::string prefix = "printer " + printer.hostname + " ";
    while (std::fgets(line, sizeof(line), pipe)) {
        const std::string value(line);
        if (value.rfind(prefix, 0) != 0) continue;
        std::string lower = value;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (lower.find("printing") != std::string::npos) status.status = "printing";
        else if (lower.find("disabled") != std::string::npos || lower.find("not accepting") != std::string::npos) status.status = "error";
        else status.status = "idle";
        status.status_message = value;
        break;
    }
    pclose(pipe);
#endif

    return status;
}
