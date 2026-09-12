#include "compat/DrawSyncManagerLifetime.hpp"

#include "compat/JkrAllocationDomain.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"

#include <aurora/exception.hpp>
#include <aurora/guest_thread.hpp>
#include <dolphin/gx/GXAurora.h>
#include <algorithm>
#include <exception>
#include <stdexcept>

namespace smgpc::compat {
    DrawSyncManagerLifetime::DrawSyncManagerLifetime(std::shared_ptr<JkrHeapRuntime> heaps) {
        const aurora::os::GuestThreadExecutionScope execution;
        const JkrHostAllocationScope host;
        if (DrawSyncManager::sInstance)
            aurora::throw_host_exception<std::logic_error>("The original DrawSyncManager already has a process owner");
        _domain = JkrAllocationDomain::create(std::move(heaps), 128U * 1024U);
        const JkrAllocationScope game(_domain);
        // GameSystem::init uses these original queue capacity/priority values.
        _manager = DrawSyncManager::start(0x300, 15);
    }

    DrawSyncManagerLifetime::~DrawSyncManagerLifetime() {
        const aurora::os::GuestThreadExecutionScope execution;
        if (DrawSyncManager::sInstance != _manager) std::terminate();
        quiesce_draw_sync();
        const JkrAllocationScope game(_domain);
        DrawSyncManager::end();
        // The original destructor joins its worker. Only then may the retained
        // heap reclaim its raw stack, message array and Fifo helper allocations.
    }

    void quiesce_draw_sync() {
        const aurora::os::GuestThreadExecutionScope execution;
        auto* manager = DrawSyncManager::sInstance;
        if (!manager) return;
        AuroraDrainGXCommands();
        // Token callbacks enqueue acknowledgements after dispatching the Game
        // callback. Empty GX alone therefore does not mean the manager is idle.
        // OSYieldThread releases the guest CPU so its original worker can run.
        while (manager->mQueue.usedCount != 0 || manager->mFifo->getCount() != 0)
            OSYieldThread();
    }

    void retire_draw_sync_callbacks(const JKRHeap& heap) {
        const aurora::os::GuestThreadExecutionScope execution;
        auto* manager = DrawSyncManager::sInstance;
        if (!manager) return;
        quiesce_draw_sync();
        for (auto& range : manager->mTokenRanges) {
            // find also includes original child heaps and callback base-class
            // subobjects. There is no parallel callback registry or actor list.
            if (range.mCallback && heap.find(range.mCallback))
                range = {};
        }
    }

    DrawSyncRegistrationTransaction::DrawSyncRegistrationTransaction() {
        const aurora::os::GuestThreadExecutionScope execution;
        _manager = DrawSyncManager::sInstance;
        if (!_manager) return;
        // Replacing a range while its previous token is pending would make
        // original dispatch drop that token's Fifo acknowledgement.
        quiesce_draw_sync();
        std::copy(std::begin(_manager->mTokenRanges), std::end(_manager->mTokenRanges), _ranges.begin());
        _low = _manager->_36E;
        _high = _manager->_370;
    }

    DrawSyncRegistrationTransaction::~DrawSyncRegistrationTransaction() { rollback(); }
    void DrawSyncRegistrationTransaction::commit() noexcept { _manager = nullptr; }

    void DrawSyncRegistrationTransaction::rollback() {
        const aurora::os::GuestThreadExecutionScope execution;
        if (!_manager) return;
        if (DrawSyncManager::sInstance != _manager) std::terminate();
        quiesce_draw_sync();
        std::copy(_ranges.begin(), _ranges.end(), std::begin(_manager->mTokenRanges));
        _manager->_36E = _low;
        _manager->_370 = _high;
        _manager = nullptr;
    }
} // namespace smgpc::compat
