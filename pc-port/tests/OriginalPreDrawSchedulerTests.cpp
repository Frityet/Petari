#include "Game/NameObj/NameObj.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include <cstring>
#include "layout/LayoutRuntime.hpp"
#include "runtime/SceneScheduler.hpp"
#include <cassert>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using smgpc::runtime::SceneScheduler;
using smgpc::runtime::SceneSchedulerBinding;

struct DrawObject final : NameObj {
    DrawObject(const char* name, int value, std::vector<int>& log) : NameObj(name), value(value), log(log) {}
    void draw() const override { log.push_back(value); }
    int value;
    std::vector<int>& log;
};


struct AllocatingDrawObject final : NameObj {
    AllocatingDrawObject() : NameObj("allocation routing") {}
    void pre_draw() const { pre_allocation = new unsigned[8]; ++pre_calls; }
    void draw() const override { draw_allocation = new unsigned[8]; ++draw_calls; }
    void release_allocations() const {
        delete[] pre_allocation;
        delete[] draw_allocation;
        pre_allocation = draw_allocation = nullptr;
    }
    mutable unsigned* pre_allocation = nullptr;
    mutable unsigned* draw_allocation = nullptr;
    mutable unsigned pre_calls = 0;
    mutable unsigned draw_calls = 0;
};

static void verify_invocation_allocation_routing() {
    using namespace smgpc::compat;
    auto runtime = JkrHeapRuntime::create(2U << 20);
    auto registration_domain = JkrAllocationDomain::create(runtime, 64U << 10);
    auto invocation_domain = JkrAllocationDomain::create(runtime, 64U << 10);
    std::weak_ptr<JkrAllocationDomain> registration_weak = registration_domain;
    std::weak_ptr<JkrAllocationDomain> invocation_weak = invocation_domain;
    SceneScheduler scheduler;
    SceneSchedulerBinding binding(scheduler);
    AllocatingDrawObject object;
    {
        JkrAllocationScope heap(registration_domain);
        MR::registerPreDrawFunction(MR::Functor(static_cast<const AllocatingDrawObject*>(&object), &AllocatingDrawObject::pre_draw), 72);
    }
    registration_domain.reset();
    assert(!registration_weak.expired());
    {
        JkrAllocationScope heap(invocation_domain);
        scheduler.connect_name_obj(object, -1, -1, -1, 72);
        scheduler.execute_draw_type(72);
        assert(JKRHeap::findFromRoot(object.pre_allocation) == &invocation_domain->heap());
        assert(JKRHeap::findFromRoot(object.draw_allocation) == &invocation_domain->heap());
        // Metadata belongs to the host, even though game code allocates in the
        // invocation heap. The registration heap is deliberately different.
#ifndef NDEBUG
        assert(!scheduler.last_execution_trace().empty());
        assert(JKRHeap::findFromRoot(const_cast<smgpc::runtime::SceneSchedulerEntryState*>(scheduler.last_execution_trace().data())) == nullptr);
#endif
        object.release_allocations();
        auto* after = new unsigned[8];
        assert(JKRHeap::findFromRoot(after) == &invocation_domain->heap());
        delete[] after;
    }
    invocation_domain.reset();
    assert(invocation_weak.expired());
    // Reuse and overwrite a retired child allocation before reusing scheduler
    // metadata, rather than relying on freed arena bytes retaining old values.
    {
        auto churn = JkrAllocationDomain::create(runtime, 64U << 10);
        JkrAllocationScope heap(churn);
        auto* overwrite = new unsigned char[32U << 10];
        std::memset(overwrite, 0xa5, 32U << 10);
        delete[] overwrite;
    }
#ifndef NDEBUG
    assert(scheduler.last_execution_trace().back().name == "allocation routing");
#endif
    scheduler.execute_draw_type(72);
    assert(object.pre_calls == 2 && object.draw_calls == 2);
    assert(JKRHeap::findFromRoot(object.pre_allocation) == nullptr);
    assert(JKRHeap::findFromRoot(object.draw_allocation) == nullptr);
    object.release_allocations();
    scheduler.clear();
    assert(registration_weak.expired());
}

struct Callback final : MR::FunctorBase {
    Callback(void (*function)(void*), void* context, int& deleted) : function(function), context(context), deleted(deleted) {}
    ~Callback() override { if (cloned) ++deleted; }
    void operator()() const override { function(context); }
    MR::FunctorBase* clone(JKRHeap* heap) const override {
        auto* copy = new (heap, 0) Callback(*this);
        copy->cloned = true;
        return copy;
    }
    void (*function)(void*);
    void* context;
    int& deleted;
    bool cloned = false;
};

struct Push { std::vector<int>* log; int value; };
static void push(void* raw) { auto& p = *static_cast<Push*>(raw); p.log->push_back(p.value); }
static std::vector<int>* plain_log;
static void plain_callback() { plain_log->push_back(6); }

int main() {
    verify_invocation_allocation_routing();
    std::vector<int> log;
    int deleted = 0;
    Push old_push{&log, 9}, new_push{&log, 8};
    Callback old_callback(push, &old_push, deleted), new_callback(push, &new_push, deleted);
    SceneScheduler scheduler;
    SceneSchedulerBinding binding(scheduler);
    DrawObject a("first", 1, log), b("second", 2, log), c("third", 3, log);
    scheduler.connect_name_obj(a, -1, -1, -1, 71);
    scheduler.connect_name_obj(b, -1, -1, -1, 71);
    scheduler.connect_name_obj(c, -1, -1, -1, 71);
    MR::registerPreDrawFunction(old_callback, 71);
    scheduler.execute_draw_type(71);
    assert((log == std::vector<int>{9, 1, 2, 3}));
    log.clear();
    scheduler.disconnect_name_obj(a);
    scheduler.execute_draw_type(71);
    assert((log == std::vector<int>{9, 3, 2}));
    log.clear();
    const auto marker = scheduler.registration_marker();
    MR::registerPreDrawFunction(new_callback, 71);
    auto scoped = std::make_unique<DrawObject>("scoped", 4, log);
    scheduler.connect_name_obj(*scoped, -1, -1, -1, 71);
    scheduler.execute_draw_type(71);
    assert((log == std::vector<int>{8, 3, 2, 4}));
    log.clear();
    const auto removed = scheduler.remove_registrations_since(marker);
    assert(removed.size() == 1 && removed[0].name_obj == scoped.get());
    scoped.reset();
    scheduler.execute_draw_type(71);
    assert((log == std::vector<int>{9, 3, 2}));
    log.clear();
    const auto real_functor_marker = scheduler.registration_marker();
    plain_log = &log;
    MR::registerPreDrawFunction(MR::Functor(plain_callback), 71);
    scheduler.execute_draw_type(71);
    assert((log == std::vector<int>{6, 3, 2}));
    log.clear();
    MR::registerPreDrawFunction(MR::Functor(static_cast<const DrawObject*>(&a), &DrawObject::draw), 71);
    scheduler.execute_draw_type(71);
    assert((log == std::vector<int>{1, 3, 2}));
    log.clear();
    scheduler.remove_registrations_since(real_functor_marker);
    auto automatic = std::make_unique<DrawObject>("destructor unregister", 5, log);
    scheduler.connect_name_obj(*automatic, -1, -1, -1, 71);
    scheduler.execute_draw_type(71);
    automatic.reset();
    log.clear();
    scheduler.execute_draw_type(71);
    assert((log == std::vector<int>{9, 3, 2}));
    log.clear();

    struct Replace { Callback* next; std::vector<int>* log; } replacement{&new_callback, &log};
    Callback replacing([](void* raw) {
        auto& r = *static_cast<Replace*>(raw);
        r.log->push_back(7);
        MR::registerPreDrawFunction(*r.next, 71);
    }, &replacement, deleted);
    MR::registerPreDrawFunction(replacing, 71);
    scheduler.execute_draw_type(71);
    assert((log == std::vector<int>{7, 3, 2}));
    log.clear();
    scheduler.execute_draw_type(71);
    assert((log == std::vector<int>{8, 3, 2}));
    log.clear();

    // Clones retain the originating scene heap, independently from the caller's
    // local domain handle and from the draw-buffer owner's current domain.
    auto runtime = smgpc::compat::JkrHeapRuntime::create(1U << 20);
    auto domain = smgpc::compat::JkrAllocationDomain::create(runtime, 64U << 10);
    std::weak_ptr<smgpc::compat::JkrAllocationDomain> weak = domain;
    const auto heap_marker = scheduler.registration_marker();
    {
        smgpc::compat::JkrAllocationScope heap(domain);
        MR::registerPreDrawFunction(old_callback, 71);
    }
    domain.reset();
    assert(!weak.expired());
    scheduler.execute_draw_type(71);
    assert((log == std::vector<int>{9, 3, 2}));
    log.clear();
    scheduler.remove_registrations_since(heap_marker);
    assert(weak.expired());
    runtime.reset();

    // A live native layout enters the same original category batch. Its actual
    // draw reaches the renderer boundary; this CPU fixture opens no renderer.
    smgpc::layout::LayoutRuntime layout("native layout", "unused", 1, 74);
    layout.appear();
    scheduler.connect_name_obj(a, -1, -1, -1, 74);
    scheduler.register_layout(layout, -1, -1, 74);
    MR::registerPreDrawFunction(old_callback, 74);
    bool reached_renderer = false;
    try { scheduler.execute_draw_type(74); }
    catch (const std::runtime_error& e) { reached_renderer = std::string(e.what()) == "Aurora renderer context is not active"; }
    assert(reached_renderer && (log == std::vector<int>{9, 1}));
    log.clear();
    scheduler.unregister_layout(layout);
    scheduler.disconnect_name_obj(a);
    scheduler.execute_draw_type(74);
    assert(log.empty());

    int clearing_deleted = 0;
    struct Clear { SceneScheduler* scheduler; int* deleted; } clear{&scheduler, &clearing_deleted};
    Callback clearing([](void* raw) {
        auto& c = *static_cast<Clear*>(raw);
        assert(*c.deleted == 0);
        c.scheduler->clear();
        assert(*c.deleted == 0);
        // The currently executing clone remains alive after registration clear.
    }, &clear, clearing_deleted);
    MR::registerPreDrawFunction(clearing, 71);
    scheduler.execute_draw_type(71);
    assert(log.empty() && clearing_deleted == 1);
    const int after_clear = deleted;
    scheduler.execute_draw_type(71);
    assert(log.empty() && deleted == after_clear);

    bool invalid_category = false;
    try { MR::registerPreDrawFunction(old_callback, 83); }
    catch (const std::out_of_range&) { invalid_category = true; }
    assert(invalid_category);
    std::cout << "invocation_heap_routing=pass metadata_after_heap_retirement=pass original_order_and_swap_remove=pass callback_once_and_empty_skip=pass scoped_rollback=pass nameobj_destruction=pass self_replacement=pass callback_heap_retention=pass mixed_layout_renderer_boundary=pass clear_during_predraw=pass category_bounds=pass actual_function_and_const_member_functors=pass clones_destroyed=" << deleted << '\n';
}
