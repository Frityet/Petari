#pragma once

#include <aurora/process.hpp>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

namespace smgpc::app {
// Called only after the application and its guest/host owners have unwound.
// A console shutdown/menu request exits this client. Reboot/restart replaces
// this process using its original invocation; it never resets the host OS.
inline int handle_process_request(const aurora::os::ProcessRequest& request, char* const* arguments) {
    using Destination = aurora::os::ProcessDestination;
    if (request.destination == Destination::Shutdown || request.destination == Destination::Menu) return 0;
    if (arguments == nullptr || arguments[0] == nullptr) {
        std::fputs("Cannot restart without the original application arguments.\n", stderr);
        return 1;
    }
    char code[16];
    std::snprintf(code, sizeof(code), "%u", request.resetCode);
#if defined(_WIN32)
    if (_putenv_s("AURORA_PROCESS_RESET_CODE", code) != 0) return 1;
    _execvp(arguments[0], arguments);
#else
    if (setenv("AURORA_PROCESS_RESET_CODE", code, 1) != 0) return 1;
    execvp(arguments[0], arguments);
#endif
    std::fprintf(stderr, "Could not restart the application: %s\n", std::strerror(errno));
    return 1;
}
}
