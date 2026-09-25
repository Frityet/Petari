#include "GameResourceRuntime.hpp"
#include "resource/EmbeddedGameTables.hpp"
#include <JSystem/JKernel/JKRExpHeap.hpp>
#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>
#include <aurora/guest_thread.hpp>
#include <aurora/mem2_arena.hpp>
#include <dolphin/ar.h>
#include <dolphin/os.h>
#include <cstdlib>
#include <limits>
#include <new>
#include <stdexcept>

namespace smgpc::resource {
    namespace {
        GameResourceRuntime* active_runtime = nullptr;
        std::size_t checked_budget(std::size_t size, std::size_t minimum) {
            if (size < minimum || size > std::numeric_limits<s32>::max())
                aurora::throw_host_exception<std::invalid_argument>("JKR host heap budget is outside the original signed-size range");
            return size & ~std::size_t(31);
        }
        constexpr std::size_t root_header_size = (sizeof(JKRExpHeap) + 31) & ~std::size_t(31);
    }

    GameResourceRuntime::GameResourceRuntime(GameResourceBudget budget) : _budget(budget) {
        const aurora::allocation::HostAllocationScope host;
        if (active_runtime)
            aurora::throw_host_exception<std::logic_error>("A game resource runtime is already installed");
        OSInit();
        if (JKRHeap::sRootHeap || JKRHeap::getUserRamStart())
            aurora::throw_host_exception<std::logic_error>("An original JKR root heap already exists");
        const auto bytes = checked_budget(budget.host_heap_bytes, root_header_size + sizeof(JKRExpHeap::CMemBlock) + 32);
        void* memory = nullptr;
        if (posix_memalign(&memory, 32, bytes) != 0) throw std::bad_alloc();
        std::shared_ptr<void> storage(memory, std::free);
        auto* heap = JKRExpHeap::createRoot(memory, static_cast<u32>(bytes), false);
        if (!heap) throw std::bad_alloc();
        _rootHeap = heap->adoptNativeOwnership(std::move(storage));
        _mem1 = Mem1ResourceHeap::create(budget.mem1_bytes);
        _embedded_tables = std::make_unique<EmbeddedGameTables>();
        active_runtime = this;
    }

    GameResourceRuntime::~GameResourceRuntime() { active_runtime = nullptr; }
    GameResourceRuntime* GameResourceRuntime::active() noexcept { return active_runtime; }
    const std::shared_ptr<Mem1ResourceHeap>& GameResourceRuntime::mem1_heap() const noexcept { return _mem1; }
    const JKRHeap::Handle& GameResourceRuntime::root_heap() const noexcept { return _rootHeap; }
    const std::shared_ptr<void>& GameResourceRuntime::mem2_storage() const noexcept { return _mem2Storage; }
    const GameResourceBudget& GameResourceRuntime::budget() const noexcept { return _budget; }

    void GameResourceRuntime::prepare_mem2_arena(std::size_t bytes) {
        const aurora::allocation::HostAllocationScope host;
        bytes = checked_budget(bytes, 0xE00000 + root_header_size + 32);
        if (_mem2Storage)
            aurora::throw_host_exception<std::logic_error>("The process MEM2 arena is already initialized");
        void* memory = nullptr;
        if (posix_memalign(&memory, 32, bytes) != 0) throw std::bad_alloc();
        try { aurora::bind_mem2_arena(memory, bytes); }
        catch (...) { std::free(memory); throw; }
        _mem2Storage = std::shared_ptr<void>(memory, [](void* base) {
            const aurora::os::GuestThreadExecutionScope execution;
            const aurora::allocation::HostAllocationScope host;
            ARReset();
            aurora::unbind_mem2_arena(base);
            std::free(base);
        });
    }
}
