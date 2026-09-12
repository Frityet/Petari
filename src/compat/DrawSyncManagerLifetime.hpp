#pragma once

#include "Game/System/DrawSyncManager.hpp"

#include <array>
#include <memory>

class JKRHeap;

namespace smgpc::compat {
    class JkrAllocationDomain;
    class JkrHeapRuntime;

    // The original manager, message storage and SDK worker belong to one
    // retained process heap. Callback objects continue to belong to their
    // original process/scene heaps; the manager only borrows their addresses.
    class DrawSyncManagerLifetime final {
    public:
        explicit DrawSyncManagerLifetime(std::shared_ptr<JkrHeapRuntime>);
        ~DrawSyncManagerLifetime();
        DrawSyncManagerLifetime(const DrawSyncManagerLifetime&) = delete;
        DrawSyncManagerLifetime& operator=(const DrawSyncManagerLifetime&) = delete;

    private:
        std::shared_ptr<JkrAllocationDomain> _domain;
        DrawSyncManager* _manager = nullptr;
    };

    // Finish already submitted callbacks and their original SDK queue/Fifo
    // acknowledgements before changing borrowed callback identities.
    void quiesce_draw_sync();
    void retire_draw_sync_callbacks(const JKRHeap&);

    // Save the actual five ranges and both allocation counters around an
    // original factory/init operation. Roll back before destroying its failed
    // construction graph; successful nested transactions remain part of the
    // enclosing transaction until that owner commits.
    class DrawSyncRegistrationTransaction final {
    public:
        DrawSyncRegistrationTransaction();
        ~DrawSyncRegistrationTransaction();
        DrawSyncRegistrationTransaction(const DrawSyncRegistrationTransaction&) = delete;
        DrawSyncRegistrationTransaction& operator=(const DrawSyncRegistrationTransaction&) = delete;
        void commit() noexcept;
        void rollback();

    private:
        DrawSyncManager* _manager = nullptr;
        std::array<DrawSyncManager::TDrawSyncTokenRange, 5> _ranges;
        u16 _low = 0;
        u16 _high = 0;
    };
} // namespace smgpc::compat
