#include "Game/NameObj/NameObjAdaptor.hpp"
#include "Game/Util/Functor.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include <cassert>
#include <iostream>
#include <memory>

static int free_calls;
static void free_callback() { ++free_calls; }

struct Receiver {
    int* calls;
    void on_draw() const { ++*calls; }
};

int main() {
    using namespace smgpc::compat;
    auto runtime = JkrHeapRuntime::create(1U << 20);
    auto active = JkrAllocationDomain::create(runtime, 64U << 10);
    auto requested = JkrAllocationDomain::create(runtime, 64U << 10);
    int calls = 0;
    const Receiver receiver{&calls};
    auto method = MR::Functor(&receiver, &Receiver::on_draw);
    auto function = MR::Functor_Inline(free_callback);
    {
        JkrAllocationScope scope(active);
        // Explicit clone ownership must win over the unrelated current heap.
        std::unique_ptr<MR::FunctorBase> cloned_method(method.clone(&requested->heap()));
        std::unique_ptr<MR::FunctorBase> cloned_function(function.clone(&requested->heap()));
        assert(JKRHeap::findFromRoot(cloned_method.get()) == &requested->heap());
        assert(JKRHeap::findFromRoot(cloned_function.get()) == &requested->heap());
        (*cloned_method)();
        (*cloned_function)();
        NameObjAdaptor adaptor("particle callback ownership probe");
        adaptor.connectToDraw(method);
        adaptor.connectToCalcAnim(function);
        adaptor.connectToMovement(method);
        assert(JKRHeap::findFromRoot(adaptor.mDrawAnimFunc) == &active->heap());
        assert(JKRHeap::findFromRoot(adaptor.mCalcAnimFunc) == &active->heap());
        assert(JKRHeap::findFromRoot(adaptor.mMovementFunc) == &active->heap());
        const NameObj& base = adaptor;
        base.draw();
        adaptor.calcAnim();
        adaptor.movement();
    }
    requested.reset();
    active.reset();
    assert(calls == 3 && free_calls == 2);
    std::cout << "actual_adaptor_dispatches=3 explicit_clone_dispatches=2 heap_ownership_checks=5 native_virtual_destruction=pass\n";
}
