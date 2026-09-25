#include <aurora/exception.hpp>
#include "GameResourceRuntime.hpp"
#include "resource/EmbeddedGameTables.hpp"
#include <dolphin/os.h>
#include <stdexcept>

namespace smgpc::resource {
    namespace { GameResourceRuntime* active_runtime = nullptr; }
    GameResourceRuntime::GameResourceRuntime(GameResourceBudget budget) : _budget(budget) {
        compat::JkrHostAllocationScope host;
        if (budget.cohort_bytes >= budget.host_heap_bytes)
            aurora::throw_host_exception<std::invalid_argument>("Resource cohort must fit inside its original JKR root heap");
        if (active_runtime)
            aurora::throw_host_exception<std::logic_error>("A game resource runtime is already installed");
        OSInit();
        _heaps = compat::JkrHeapRuntime::create(budget.host_heap_bytes);
        _mem1 = Mem1ResourceHeap::create(budget.mem1_bytes);
        _embedded_tables = std::make_unique<EmbeddedGameTables>();
        active_runtime = this;
    }
    GameResourceRuntime::~GameResourceRuntime() { active_runtime = nullptr; }
    GameResourceRuntime* GameResourceRuntime::active() noexcept { return active_runtime; }
    std::shared_ptr<compat::JkrAllocationDomain> GameResourceRuntime::create_cohort() const {
        return compat::JkrAllocationDomain::create(_heaps, _budget.cohort_bytes);
    }
    const std::shared_ptr<Mem1ResourceHeap>& GameResourceRuntime::mem1_heap() const noexcept { return _mem1; }
    const std::shared_ptr<compat::JkrHeapRuntime>& GameResourceRuntime::host_heaps() const noexcept { return _heaps; }
    const GameResourceBudget& GameResourceRuntime::budget() const noexcept { return _budget; }
}
