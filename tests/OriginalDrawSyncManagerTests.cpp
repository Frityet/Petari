#include "Game/System/DrawSyncManager.hpp"

#include <aurora/aurora.h>
#include <aurora/guest_thread.hpp>
#include <dolphin/gx/GXAurora.h>

#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Callback final : DrawSyncCallback {
    std::array<u16, 128> tokens{};
    std::size_t count = 0;

    void drawSyncCallback(u16 token) override {
        require(count < tokens.size(), "original callback count exceeded fixture capacity");
        require(GXReadDrawSync() == token, "original callback must observe its completed GPU token");
        tokens[count++] = token;
    }
};

// The original process heap owns these raw helper allocations. This isolated
// fixture uses native allocation and reclaims them only after the real manager
// destructor has joined its original SDK thread.
struct ManagerLifetime {
    DrawSyncManager* manager = DrawSyncManager::start(0x300, 15);
    ~ManagerLifetime() {
        auto* stack = manager->mStack;
        auto* messages = manager->mMessages;
        auto* fifo = manager->mFifo;
        DrawSyncManager::end();
        delete[] fifo->mArray;
        delete fifo;
        delete[] messages;
        delete[] stack;
    }
};
}

int main() {
    try {
        AuroraConfig config{};
        config.appName = "Original DrawSyncManager proof";
#if defined(__APPLE__)
        config.desiredBackend = BACKEND_METAL;
#else
        config.desiredBackend = BACKEND_VULKAN;
#endif
        config.allowCpuAdapter = true;
        config.windowWidth = 64;
        config.windowHeight = 64;
        config.vsync = false;
        config.pauseOnFocusLost = false;
        config.logLevel = LOG_WARNING;
        config.cachePath = std::getenv("AURORA_TEST_CACHE_PATH");
        const auto initialized = aurora_initialize(0, nullptr, &config);
        struct AuroraLifetime { ~AuroraLifetime() { aurora_shutdown(); } } aurora;
        require(initialized.backend == config.desiredBackend, "real GPU backend required");
        GXInit(nullptr, 0);
        require(aurora_begin_frame(), "recording frame required");
        {
            const aurora::os::GuestThreadExecutionScope execution;
            Callback first;
            Callback second;
            ManagerLifetime owner;
            const auto firstToken = owner.manager->setCallback(2, 1, &first);
            const auto secondToken = owner.manager->setCallback(4, 1, &second);
            require(firstToken == 1 && secondToken == 0xa000, "original low/high token ranges required");
            for (unsigned i = 0; i < 64; ++i) {
                owner.manager->pushBreakPoint();
                GXSetDrawSync(firstToken);
                owner.manager->pushBreakPoint();
                GXSetDrawSync(secondToken);
            }
            GXDrawDone();
            require(first.count == 64 && second.count == 64, "all original callback ranges must complete");
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{5};
            while (owner.manager->mQueue.usedCount != 0 || owner.manager->mFifo->getCount() != 0) {
                require(std::chrono::steady_clock::now() < deadline, "original manager queue must drain");
                OSYieldThread();
            }
            GXBool overflow, underflow, readIdle, commandIdle, breakpoint;
            GXGetGPStatus(&overflow, &underflow, &readIdle, &commandIdle, &breakpoint);
            require(!breakpoint, "original manager must disable its final FIFO breakpoint");
        }
        require(DrawSyncManager::sInstance == nullptr, "original shutdown must clear singleton identity");
        aurora_end_frame();
        std::cout << "Original DrawSyncManager completed 128 GPU callbacks, queue turnover and thread retirement\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
