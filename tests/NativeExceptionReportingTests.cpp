#include "Game/System/GameSystemException.hpp"

#include <aurora/allocation.hpp>
#include <aurora/diagnostics.hpp>
#include <dolphin/gx/GXStruct.h>

#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string_view>
#include <thread>

namespace {
    void throw_exception() {
        throw std::runtime_error("native diagnostic exception marker");
    }

    void violate_noexcept() noexcept {
        throw_exception();
    }
}

// Each fatal mode is executed in a fresh subprocess by the receipt script.
// This exercises the installed handler without intercepting abort or replacing
// the SDK reporting functions. No renderer, game heap or device is initialized.
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const auto previous = std::get_terminate();
    GameSystemException::init();
    const auto installed = std::get_terminate();
    if (installed == previous) return 3;
    {
        const aurora::allocation::ClientAllocationScope client({true, true});
        GameSystemException::init();
        aurora::diagnostics::install_native_exception_reporting();
        if (!aurora::allocation::routing_state.guest || !aurora::allocation::routing_state.callbackGuest) return 4;
    }
    if (std::get_terminate() != installed || aurora::allocation::routing_state.guest) return 5;

    const std::string_view mode(argv[1]);
    if (mode == "install") {
        std::fputs("native diagnostic installation passed\n", stdout);
        return 0;
    }
    if (mode == "throw") throw_exception();
    if (mode == "unknown") throw 42;
    if (mode == "terminate") std::terminate();
    if (mode == "noexcept") violate_noexcept();
    if (mode == "worker") {
        std::thread worker(throw_exception);
        worker.join();
    }
    if (mode == "panic") OSPanic("diagnostic-fixture.cpp", 123, "panic marker %d", 456);
    if (mode == "fatal") OSFatal(GXColor{}, GXColor{}, "fatal marker");
    if (mode == "ppc") GameSystemException::handleException(static_cast<OSError>(2), nullptr, 0x1234, 0x5678);
    return 6;
}
