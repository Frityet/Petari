#include "compat/JkrAllocationDomain.hpp"
#include "Game/System/WPad.hpp"
#include "Game/System/WPadHolder.hpp"
#include "Game/System/WPadPointer.hpp"
#include "Game/System/WPadStick.hpp"
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
        message_storage(heaps);
        require(heaps->root_heap().getFreeSize() == free, "retained-message source domains release all Game storage");
        std::cout << "Retained message storage and pointer provenance passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
