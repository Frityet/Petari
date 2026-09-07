#pragma once

// Native output and fatal-error boundary for the original heap algorithms.
// The on-screen JUT console and exception monitor are separate SDK systems.
namespace smgpc::compat {
    void jkr_warning(const char*);
    void jkr_warning_f(const char*, ...);
    void jkr_report(const char*);
    void jkr_report_f(const char*, ...);
    [[noreturn]] void jkr_panic(const char* file, int line, const char* format, ...);
}
