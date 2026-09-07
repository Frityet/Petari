#include "compat/WPadOwnership.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "Game/System/WPad.hpp"
#include "Game/System/WPadHolder.hpp"
#include "Game/System/WPadPointer.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "runtime/RuntimeServices.hpp"
#include <aurora/exception.hpp>
#include <aurora/wpad.hpp>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void require(bool value, const char* message) {
    if (!value) aurora::throw_host_exception<std::runtime_error>(message);
}
void near(float actual, float expected, const char* message) {
    require(std::abs(actual - expected) < 0.00001F, message);
}
void sample_history(const std::shared_ptr<smgpc::compat::JkrHeapRuntime>& heaps) {
    auto domain = smgpc::compat::JkrAllocationDomain::create(heaps, 512U << 10);
    auto& input = aurora::wpad_service();
    input.clear();
    smgpc::compat::WPadOwnership owner(domain);
    input.set_pointer_resolution(0, 640, 480);
    auto& pad = *MR::getWPad(0);
    require(pad.mPointer && pad.mButton && pad.mCorePadAccel && pad.mCorePadSwing && pad._18 && pad._1C &&
            pad.mSubPadAccel && pad.mSubPadSwing && pad.mStick && pad.mLeaveWatcher && pad.mInfoChecker,
            "the actual WPad constructor creates every original typed child");
    require(JKRHeap::findFromRoot(pad.mPointer) == &domain->heap(), "original WPad history has an explicit retained heap owner");
    for (int i = 0; i < 6; ++i) {
        input.begin_frame();
        input.set_pointer(0, 480, 120, true);
        owner.update_pointer_samples();
        require(MR::isCorePadPointInScreen(0) == (i == 5), "original five-sample confirmation delays pointing until the sixth sample");
    }
    TVec2f position, horizon;
    MR::getCorePadPointingPos(&position, 0);
    pad.mPointer->getHorizonVec(&horizon);
    near(position.x, .5F, "KPAD converts pixel x to normalized coordinates");
    near(position.y, -.5F, "KPAD converts pixel y to normalized coordinates");
    require(horizon.x == 1 && horizon.y == 0 && MR::getCorePadEnablePastCount(0) == 1,
            "KB+M upright orientation belongs to the actual accepted sample, without replaying previous frames");
    input.begin_frame();
    input.set_pointer(0, 400, 240, true, 0, 1);
    owner.update_pointer_samples();
    pad.mPointer->getHorizonVec(&horizon);
    require(horizon.x == 0 && horizon.y == 1 && pad.mPointer->_45 == 1,
            "a producer-supplied horizon and original speed threshold survive KPAD and the original filter");
    require(input.past_pointer(0, 1).horizon_x == 1, "sample history retains the previous orientation independently");

    auto& records = *pad.mReadInfo;
    for (u32 i = 0; i < 120; ++i) {
        records.mStatusArray[i] = KPADStatus{};
        records.mStatusArray[i].dpd_valid_fg = 2;
        records.mStatusArray[i].pos = {float(i), -float(i)};
        records.mStatusArray[i].horizon = {float(i + 1), float(i + 2)};
    }
    records.mValidStatusCount = 121;
    pad.mPointer->update();
    require(pad.mPointer->mEnablePastCount == 120, "original read loop caps the batch to its 120 allocated samples");
    pad.mPointer->getHorizonVec(&horizon);
    require(horizon.x == 1 && horizon.y == 2 && pad.mPointer->mPointingPosArray[0].x == 119,
            "newest-first KPAD storage becomes oldest-first original history and newest getter output");
    records.mValidStatusCount = 1;
    records.mStatusArray[0].dpd_valid_fg = -1;
    for (int i = 0; i < 22; ++i) pad.mPointer->update();
    pad.mPointer->getHorizonVec(&horizon);
    require(!pad.mPointer->mIsPointInScreen && pad.mPointer->_2C == 20 && horizon.x == 0 && horizon.y == 0,
            "signed invalid samples saturate the original dropout counter and expose zero only through the original invalid-history getter");
    input.begin_frame();
    input.set_button_mask(0, WPAD_BUTTON_A);
    owner.update_pointer_samples();
    for (int elapsed = 1; elapsed <= 36; ++elapsed) {
        input.begin_frame();
        input.set_button_mask(0, WPAD_BUTTON_A);
        owner.update_pointer_samples();
        require(input.is_button_repeated(0, WPAD_BUTTON_A) == (elapsed == 25 || elapsed == 35),
                "actual WPadButton delay and pulse configure the generalized KPAD repeat clock");
    }
    input.set_connected(0, false);
    owner.update_pointer_samples();
    require(pad.mReadInfo->mValidStatusCount == 0 && pad.mPointer->_2C == 0 && pad.mPointer->mEnablePastCount == 0,
            "disconnection has no fabricated sample and invokes original reset");
}
void message_storage(const std::shared_ptr<smgpc::compat::JkrHeapRuntime>& heaps) {
    smgpc::runtime::MessageService messages;
    auto domain = smgpc::compat::JkrAllocationDomain::create(heaps, 128U << 10);
    const wchar_t* first;
    {
        smgpc::compat::JkrAllocationScope game(domain);
        messages.set_message("first", u"Retained Guidance text, longer than any small string buffer\nSecond line");
        first = messages.message_raw_wide("first")->c_str();
        messages.set_message("second", u"Different text");
        for (int i = 0; i < 256; ++i) messages.set_message("other" + std::to_string(i), u"Other message text");
        require(JKRHeap::findFromRoot(const_cast<wchar_t*>(first)) == nullptr,
                "retained message character storage escapes Game callback allocation");
    }
    domain.reset();
    require(messages.message_raw_wide("first")->c_str() == first && std::wstring(first).starts_with(L"Retained Guidance"),
            "another message lookup and Game heap teardown preserve the actual borrowed pointer");
    require(std::string(messages.message_id_for_wide_pointer(first)) == "first" &&
            messages.message_id_for_wide_pointer(L"unrelated") == nullptr,
            "message pointer provenance is per retained record, not the last query");
    require(messages.message_raw_wide("absent") == nullptr, "absent localized text remains absent");
}
}
int main() {
    try {
        auto heaps = smgpc::compat::JkrHeapRuntime::create(4U << 20);
        const auto free = heaps->root_heap().getFreeSize();
        sample_history(heaps);
        sample_history(heaps);
        message_storage(heaps);
        require(heaps->root_heap().getFreeSize() == free, "two input owners and retained-message source domains release all Game storage");
        std::cout << "Original pointer history, typed ownership, KPAD horizon, and retained messages passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
