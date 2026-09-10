#include "compat/JkrAllocationDomain.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "runtime/RuntimeServices.hpp"

#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
    void require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }

    bool outside_game_heap(const void* memory) {
        return JKRHeap::findFromRoot(const_cast<void*>(memory)) == nullptr;
    }
}

int main() {
    using namespace smgpc;
    constexpr auto names = std::array<std::string_view, 4>{
        "Retained scene transition: opening beyond the small string buffer",
        "Retained scene transition: closing beyond the small string buffer",
        "Retained scene transition: forced open beyond the small string buffer",
        "Retained scene transition: forced close beyond the small string buffer",
    };
    auto wipe = runtime::WipeService{};
    {
        auto heaps = compat::JkrHeapRuntime::create(1U << 20);
        auto scene = compat::JkrAllocationDomain::create(heaps, 1U << 18);
        {
            const auto game = compat::JkrAllocationScope(scene);
            wipe.begin_frame(27U);
            wipe.open(names[0], 12);
            wipe.close(names[1], 8);
            wipe.force_open(names[2]);
            wipe.force_close(names[3]);
            require(wipe.events().size() == names.size(), "every transition must retain its event");
            require(outside_game_heap(wipe.events().data()), "retained event storage must outlive the calling scene heap");
            require(outside_game_heap(wipe.current_name().data()), "current transition name must outlive the calling scene heap");
            for (const auto& event : wipe.events()) {
                require(outside_game_heap(event.name.data()), "each retained event string must have process lifetime");
            }
        }
    }
    // The original scene and root heaps are gone. This service still owns its
    // complete history and may safely destroy or replace its strings later.
    require(wipe.is_blank() && wipe.current_name() == names.back(), "scene teardown must preserve the current transition identity");
    for (std::size_t i = 0; i < names.size(); ++i) {
        require(wipe.events()[i].name == names[i] && wipe.events()[i].frame_index == 27U,
                "scene teardown must preserve every retained event payload");
    }
    wipe.force_open(names[0]);
    require(wipe.is_open() && wipe.events().size() == 5U, "retained service must remain usable after original scene retirement");
    std::cout << "PASS native wipe event storage and strings survive original scene heap retirement\n";
}
