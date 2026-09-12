#include "app/ProcessRequest.hpp"

#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace {
bool unwound = false;
struct Owner {
    ~Owner() {
        unwound = true;
#if defined(_WIN32)
        _putenv_s("AURORA_PROCESS_TEST_UNWOUND", "1");
#else
        setenv("AURORA_PROCESS_TEST_UNWOUND", "1", 1);
#endif
    }
};
}

int main(int argc, char** argv) {
    using namespace aurora::os;
    if (argc != 2) return 2;
    if (std::strcmp(argv[1], "invalid-invocation") == 0) {
        return smgpc::app::handle_process_request(ProcessRequest(ProcessDestination::Restart), nullptr) == 1 ? 0 : 3;
    }
    const bool restart = std::strcmp(argv[1], "restart") == 0;
    if (restart) {
        if (const auto* code = std::getenv("AURORA_PROCESS_RESET_CODE")) {
            const auto* retired = std::getenv("AURORA_PROCESS_TEST_UNWOUND");
            if (std::strcmp(code, "137") != 0 || !retired || std::strcmp(retired, "1") != 0) return 4;
            std::puts("[pass] real process replacement preserved invocation and reset code after owner teardown");
            return 0;
        }
    }
    try {
        Owner owner;
        throw ProcessRequest(restart ? ProcessDestination::Restart : ProcessDestination::Shutdown, 137);
    } catch (const ProcessRequest& request) {
        if (!unwound) return 5;
        const int result = smgpc::app::handle_process_request(request, argv);
        if (!restart && result == 0) std::puts("[pass] shutdown exits the client after owner teardown");
        return result;
    }
}
