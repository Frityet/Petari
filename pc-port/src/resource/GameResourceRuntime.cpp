#include "GameResourceRuntime.hpp"
#include "compat/JutTextureAllocation.hpp"
#include <dolphin/os.h>
#include <stdexcept>

namespace smgpc::resource {
    GameResourceRuntime::GameResourceRuntime(GameResourceBudget budget) : _budget(budget) {
        compat::JkrHostAllocationScope host;
        if (budget.cohort_bytes >= budget.host_heap_bytes)
            throw std::invalid_argument("Resource cohort must fit inside its original JKR root heap");
        OSInit();
        _heaps = compat::JkrHeapRuntime::create(budget.host_heap_bytes);
        _mem1 = Mem1ResourceHeap::create(budget.mem1_bytes);
        _textures = std::make_unique<compat::JutTextureAllocationService>(_mem1);
    }
    GameResourceRuntime::~GameResourceRuntime() = default;
    std::shared_ptr<compat::JkrAllocationDomain> GameResourceRuntime::create_cohort() const {
        return compat::JkrAllocationDomain::create(_heaps, _budget.cohort_bytes);
    }
    const std::shared_ptr<Mem1ResourceHeap>& GameResourceRuntime::mem1_heap() const noexcept { return _mem1; }
    const GameResourceBudget& GameResourceRuntime::budget() const noexcept { return _budget; }
}
