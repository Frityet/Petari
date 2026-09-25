#include "compat/WPadOwnership.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "Game/System/WPad.hpp"
#include "Game/System/WPadHolder.hpp"
#include "Game/System/WPadRumble.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "Game/System/HeapMemoryWatcher.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "JSystem/JKernel/JKRExpHeap.hpp"
#include <aurora/exception.hpp>
#include <aurora/wpad.hpp>
#include <stdexcept>

namespace smgpc::compat {
namespace {
thread_local WPadOwnership* current_wpad_owner = nullptr;


}

struct WPadOwnership::State {
    explicit State(std::shared_ptr<JkrAllocationDomain> allocation_domain) : domain(std::move(allocation_domain)) {
        JkrAllocationScope game(domain);
        holder = new WPadHolder;
    }
    ~State() {
        JkrAllocationScope game(domain);
        delete holder;
    }
    std::shared_ptr<JkrAllocationDomain> domain;
    WPadHolder* holder = nullptr;
};

WPadOwnership::WPadOwnership(std::shared_ptr<JkrAllocationDomain> domain) {
    JkrHostAllocationScope host;
    _previous = current_wpad_owner;
    for (std::size_t channel = 0; channel < _previous_rumble.size(); ++channel)
        _previous_rumble[channel] = _previous ? _previous->pad(channel).getRumbleInstance() : nullptr;
    try {
        _state = std::make_unique<State>(std::move(domain));
    } catch (...) {
        for (std::size_t channel = 0; channel < _previous_rumble.size(); ++channel)
            if (_previous_rumble[channel]) _previous_rumble[channel]->registInstance();
        throw;
    }
    current_wpad_owner = this;
}
WPadOwnership::~WPadOwnership() {
    _state.reset();
    current_wpad_owner = _previous;
    for (std::size_t channel = 0; channel < _previous_rumble.size(); ++channel) {
        if (_previous_rumble[channel]) _previous_rumble[channel]->registInstance();
    }
}
void WPadOwnership::update_samples() {
    JkrAllocationScope game(_state->domain);
    aurora::wpad_service().dispatch_callbacks();
    _state->holder->update();
}
WPad& WPadOwnership::pad(int channel) {
    auto* pad = holder().getWPad(channel);
    if (!pad)
        aurora::throw_host_exception<std::out_of_range>("WPad channel is outside the original two Game slots.");
    return *pad;
}
WPadHolder& WPadOwnership::holder() { return *_state->holder; }
JKRHeap& WPadOwnership::heap() { return _state->domain->heap(); }
WPadHolder& require_wpad_holder() {
    if (auto* system = SingletonHolder<GameSystem>::get()) {
        if (!system->mObjHolder || !system->mObjHolder->mWPadHolder)
            aurora::throw_host_exception<std::logic_error>("The original process has not created its WPad holder.");
        return *system->mObjHolder->mWPadHolder;
    }
    if (!current_wpad_owner)
        aurora::throw_host_exception<std::logic_error>("Original WPad records require an active input owner.");
    return current_wpad_owner->holder();
}
JKRHeap& require_wpad_heap() {
    if (SingletonHolder<GameSystem>::get()) {
        auto* heaps = SingletonHolder<HeapMemoryWatcher>::get();
        if (!heaps || !heaps->mWPadHeap)
            aurora::throw_host_exception<std::logic_error>("The original process has not created its WPad heap.");
        return *heaps->mWPadHeap;
    }
    if (!current_wpad_owner)
        aurora::throw_host_exception<std::logic_error>("WPad SDK allocation requires an active input owner.");
    return current_wpad_owner->heap();
}
} // namespace smgpc::compat

namespace MR {
void* allocFromWPadHeap(u32 size) {
    return smgpc::compat::require_wpad_heap().alloc(size, 0);
}
u8 freeFromWPadHeap(void* allocation) {
    smgpc::compat::require_wpad_heap().free(allocation);
    return 1;
}
}
