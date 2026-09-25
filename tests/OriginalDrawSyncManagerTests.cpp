#include "Game/System/DrawSyncManager.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "compat/JkrAllocationDomain.hpp"

#include <aurora/aurora.h>
#include <aurora/guest_thread.hpp>
#include <dolphin/gx/GXAurora.h>

#include <array>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
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

struct CallbackOwner {
    std::shared_ptr<smgpc::compat::JkrAllocationDomain> domain;
    std::unique_ptr<Callback> callback;

    explicit CallbackOwner(std::shared_ptr<smgpc::compat::JkrAllocationDomain> owner) : domain(std::move(owner)) {
        const smgpc::compat::JkrAllocationScope allocation(domain);
        callback.reset(new Callback);
        require(domain->heap().find(callback.get()), "callback must belong to its actual JKR heap");
    }
    ~CallbackOwner() {
        DrawSyncManager::retireNativeCallbacks(domain->heap());
        callback.reset();
    }
};

void submit(u16 token) {
    DrawSyncManager::sInstance->pushBreakPoint();
    GXSetDrawSync(token);
}

void verify_retirement(const std::shared_ptr<smgpc::compat::JkrHeapRuntime>& heaps) {
    using namespace smgpc::compat;
    const auto managerDomain = JkrAllocationDomain::create(heaps, 128U * 1024U);
    {
        const JkrAllocationScope allocation(managerDomain);
        DrawSyncManager::start(0x300, 15);
    }
    const std::unique_ptr<DrawSyncManager, void (*)(DrawSyncManager*)> owner(
        DrawSyncManager::sInstance, [](DrawSyncManager*) { DrawSyncManager::end(); });
    auto& manager = *DrawSyncManager::sInstance;
    CallbackOwner scene(JkrAllocationDomain::create(heaps, 64U * 1024U));
    CallbackOwner process(JkrAllocationDomain::create(heaps, 64U * 1024U));
    CallbackOwner transient(JkrAllocationDomain::create(heaps, 64U * 1024U));
    const auto firstToken = manager.setCallback(2, 1, scene.callback.get());
    const auto secondToken = manager.setCallback(4, 1, process.callback.get());
    require(firstToken == 1 && secondToken == 0xa000, "original low/high token ranges required");
    for (unsigned i = 0; i < 64; ++i) {
        submit(firstToken);
        submit(secondToken);
    }
    GXDrawDone();
    DrawSyncManager::quiesceNativeCallbacks();
    require(scene.callback->count == 64 && process.callback->count == 64,
            "all original callback ranges must complete");

    // Retirement must dispatch and acknowledge pending work before it removes
    // the callback range. Otherwise the original breakpoint Fifo cannot drain.
    submit(firstToken);
    DrawSyncManager::retireNativeCallbacks(scene.domain->heap());
    require(scene.callback->count == 65 && manager.mTokenRanges[2].mCallback == nullptr,
            "pending scene callback must finish before heap registration retirement");
    require(manager.mTokenRanges[4].mCallback == process.callback.get(),
            "retiring one heap must preserve callbacks in another heap");
    require(manager.mQueue.usedCount == 0 && manager.mFifo->getCount() == 0,
            "retirement must drain both original acknowledgement queues");

    const auto low = manager._36E;
    const auto high = manager._370;
    std::array<DrawSyncManager::TDrawSyncTokenRange, 5> ranges;
    std::copy(std::begin(manager.mTokenRanges), std::end(manager.mTokenRanges), ranges.begin());
    submit(secondToken);
    {
        DrawSyncManager::CallbackRegistration outer;
        require(process.callback->count == 65,
                "transaction entry must acknowledge an old pending token before its range can be replaced");
        const auto temporaryLow = manager.setCallback(1, 1, transient.callback.get());
        {
            DrawSyncManager::CallbackRegistration inner;
            const auto temporaryHigh = manager.setCallback(4, 1, transient.callback.get());
            submit(temporaryLow);
            submit(temporaryHigh);
            inner.commit();
        }
        // The outer failed construction rolls back a successfully initialized
        // child as well, after pending callbacks have used their still-live owner.
    }
    require(transient.callback->count == 2, "rollback must finish newly registered pending callbacks");
    require(manager._36E == low && manager._370 == high, "rollback must restore both original token counters");
    for (std::size_t i = 0; i < ranges.size(); ++i) {
        require(manager.mTokenRanges[i].mStart == ranges[i].mStart &&
                    manager.mTokenRanges[i].mEnd == ranges[i].mEnd &&
                    manager.mTokenRanges[i].mCallback == ranges[i].mCallback,
                "rollback must restore all original callback ranges exactly");
    }
    submit(secondToken);
    DrawSyncManager::quiesceNativeCallbacks();
    require(process.callback->count == 66, "restored process callback must still run");
    GXBool overflow, underflow, readIdle, commandIdle, breakpoint;
    GXGetGPStatus(&overflow, &underflow, &readIdle, &commandIdle, &breakpoint);
    require(!breakpoint, "original manager must disable its final FIFO breakpoint");
}
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
            const auto heaps = smgpc::compat::JkrHeapRuntime::create(2U * 1024U * 1024U);
            const auto freeBytes = heaps->root_heap().getTotalFreeSize();
            for (unsigned i = 0; i < 3; ++i) {
                verify_retirement(heaps);
                require(heaps->root_heap().getTotalFreeSize() == freeBytes,
                        "actual manager and callback heaps must return their full allocations");
                require(DrawSyncManager::sInstance == nullptr, "original shutdown must clear singleton identity");
            }
        }
        require(DrawSyncManager::sInstance == nullptr, "original shutdown must clear singleton identity");
        aurora_end_frame();
        std::cout << "Original DrawSyncManager completed 399 GPU callbacks, pending heap retirement, nested rollback and three full heap reclamations\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
