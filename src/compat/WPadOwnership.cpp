#include "compat/WPadOwnership.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "Game/System/WPad.hpp"
#include "Game/System/WPadHolder.hpp"
#include "Game/System/WPadPointer.hpp"
#include "Game/System/WPadAcceleration.hpp"
#include "Game/System/WPadButton.hpp"
#include "Game/System/WPadHVSwing.hpp"
#include "Game/System/WPadRumble.hpp"
#include "Game/System/WPadStick.hpp"
#include "Game/System/WPadLeaveWatcher.hpp"
#include "Game/System/WPadInfoChecker.hpp"
#include <aurora/exception.hpp>
#include <aurora/wpad.hpp>
#include <array>
#include <stdexcept>

namespace smgpc::compat {
void destroy_wpad_children(WPad&) noexcept;
namespace {
thread_local WPadOwnership* current_wpad_owner = nullptr;
}

struct WPadOwnership::State {
    explicit State(std::shared_ptr<JkrAllocationDomain> allocation_domain) : domain(std::move(allocation_domain)) {
        JkrAllocationScope game(domain);
        try {
            for (s32 channel = 0; channel < 2; ++channel) {
                records[channel] = new WPadReadDataInfo;
                pads[channel] = new WPad(channel);
                pads[channel]->setReadInfo(records[channel]);
            }
        } catch (...) {
            release();
            throw;
        }
    }
    ~State() { release(); }
    void release() noexcept {
        for (auto* pad : pads) {
            if (!pad) continue;
            destroy_wpad_children(*pad);
            delete pad;
        }
        for (auto* record : records) {
            if (!record) continue;
            delete[] record->mStatusArray;
            delete record;
        }
    }
    std::shared_ptr<JkrAllocationDomain> domain;
    std::array<WPad*, 2> pads{};
    std::array<WPadReadDataInfo*, 2> records{};
};

WPadOwnership::WPadOwnership(std::shared_ptr<JkrAllocationDomain> domain) {
    JkrHostAllocationScope host;
    _previous = current_wpad_owner;
    try {
        _state = std::make_unique<State>(std::move(domain));
    } catch (...) {
        if (_previous) for (auto* pad : _previous->_state->pads) pad->_18->registInstance();
        throw;
    }
    current_wpad_owner = this;
}
WPadOwnership::~WPadOwnership() {
    _state.reset();
    current_wpad_owner = _previous;
    if (_previous) for (auto* pad : _previous->_state->pads) pad->_18->registInstance();
}
void WPadOwnership::update_samples() {
    JkrAllocationScope game(_state->domain);
    for (s32 channel = 0; channel < 2; ++channel) {
        auto& pad = *_state->pads[channel];
        auto& record = *_state->records[channel];
        record.mValidStatusCount = KPADRead(channel, record.mStatusArray, 120);
        pad.mIsConnected = aurora::wpad_service().is_connected(channel);
        pad.mIsSubPadConnected = MR::isDeviceFreeStyle(pad.getKPadStatus(0));
        pad.mButton->update();
        pad.mPointer->update();
        pad.mStick->update();
    }
}
WPad& WPadOwnership::pad(int channel) {
    if (channel < 0 || channel >= 2)
        aurora::throw_host_exception<std::out_of_range>("WPad channel is outside the original two Game slots.");
    return *_state->pads[channel];
}
} // namespace smgpc::compat

namespace MR {
bool isDeviceFreeStyle(const KPADStatus* pStatus) {
    return pStatus != nullptr && pStatus->wpad_err == WPAD_ERR_NONE && pStatus->dev_type == WPAD_DEV_FREESTYLE;
}

WPad* getWPad(s32 channel) {
    if (!smgpc::compat::current_wpad_owner)
        aurora::throw_host_exception<std::logic_error>("Original WPad records require an active input owner.");
    return &smgpc::compat::current_wpad_owner->pad(channel);
}
} // namespace MR
