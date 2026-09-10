#include "compat/WPadOwnership.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "Game/System/WPad.hpp"
#include "Game/System/WPadHolder.hpp"
#include "Game/System/WPadRumble.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include <aurora/exception.hpp>
#include <aurora/wpad.hpp>
#include <stdexcept>

namespace smgpc::compat {
void destroy_wpad_children(WPad&) noexcept;
namespace {
thread_local WPadOwnership* current_wpad_owner = nullptr;

// Explicit instantiation supplies native lifetime access without changing the
// original private Game declaration or depending on a Wii object layout.
struct RumbleCallbackTable {
    using type = WPadRumble***;
    friend type input_static(RumbleCallbackTable);
};
template <typename Tag, typename Tag::type Member> struct InputStaticAccess {
    friend typename Tag::type input_static(Tag) { return Member; }
};
template struct InputStaticAccess<RumbleCallbackTable, &WPadRumble::sInstanceForCallback>;

WPadRumble** rumble_callbacks() { return *input_static(RumbleCallbackTable{}); }

void initialize_input_statics() {
    // The original constructor lazily allocates a process-wide callback table.
    // Run it once on the host heap so no scene owns that table's storage. All
    // scene input objects below still use their retained original JKR heap.
    static const bool initialized = [] {
        JkrHostAllocationScope host;
        WPad pad(0);
        destroy_wpad_children(pad);
        return true;
    }();
    (void)initialized;
}
}

struct WPadOwnership::State {
    explicit State(std::shared_ptr<JkrAllocationDomain> allocation_domain) : domain(std::move(allocation_domain)) {
        JkrAllocationScope game(domain);
        holder = new WPadHolder;
    }
    ~State() {
        JkrAllocationScope game(domain);
        for (auto* pad : holder->mPad) {
            destroy_wpad_children(*pad);
            delete pad;
        }
        for (s32 channel = 0; channel < WPAD_MAX_CONTROLLERS; ++channel)
            delete[] holder->mReadDataInfoArray[channel].mStatusArray;
        delete[] holder->mReadDataInfoArray;
        delete holder;
    }
    aurora::WpadClientScope callbacks;
    std::shared_ptr<JkrAllocationDomain> domain;
    WPadHolder* holder = nullptr;
};

WPadOwnership::WPadOwnership(std::shared_ptr<JkrAllocationDomain> domain) {
    JkrHostAllocationScope host;
    initialize_input_statics();
    _previous = current_wpad_owner;
    for (std::size_t channel = 0; channel < _previous_rumble.size(); ++channel)
        _previous_rumble[channel] = rumble_callbacks()[channel];
    try {
        _state = std::make_unique<State>(std::move(domain));
    } catch (...) {
        for (std::size_t channel = 0; channel < _previous_rumble.size(); ++channel)
            rumble_callbacks()[channel] = _previous_rumble[channel];
        throw;
    }
    current_wpad_owner = this;
}
WPadOwnership::~WPadOwnership() {
    _state.reset();
    current_wpad_owner = _previous;
    for (std::size_t channel = 0; channel < _previous_rumble.size(); ++channel) {
        rumble_callbacks()[channel] = _previous_rumble[channel];
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
    if (!current_wpad_owner)
        aurora::throw_host_exception<std::logic_error>("Original WPad records require an active input owner.");
    return current_wpad_owner->holder();
}
JKRHeap& require_wpad_heap() {
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
