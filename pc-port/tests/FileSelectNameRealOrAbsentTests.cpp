#include "Game/Map/FileSelectFunc.hpp"
#include "Game/Map/FileSelectIconID.hpp"
#include "Game/Util/MemoryUtil.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void require_unavailable(const std::function< void() >& operation, std::string_view message) {
        auto unavailable = false;
        try {
            operation();
        } catch (const std::logic_error&) {
            unavailable = true;
        }
        require(unavailable, message);
    }
}  // namespace

int main() {
    auto passed = 0;

    require(FileSelectFunc::getMiiNameBufferSize() == 11U,
            "the exact file-select name buffer must retain the retail RFL width");
    auto source = std::array< u16, 3U >{0x41U, 0x42U, 0U};
    auto destination = std::array< u16, 3U >{};
    MR::copyMemory(destination.data(), source.data(), static_cast< u32 >(sizeof(source)));
    require(destination == source, "the Wii memory bridge must copy actual bytes");
    require_unavailable([&] { MR::copyMemory(destination.data(), nullptr, 1U); },
                        "copyMemory must reject an absent source instead of fabricating bytes");
    ++passed;

    auto fellow = FileSelectIconID{};
    fellow.setFellowID(FileSelectIconID::Mario);
    auto fellow_name = std::array< u16, 11U >{};
    require_unavailable([&] { FileSelectFunc::copyMiiName(fellow_name.data(), fellow); },
                        "a fellow name must remain unavailable without its real BMG message");
    ++passed;

    auto mii = FileSelectIconID{};
    mii.setMiiIndex(0U);
    auto mii_name = std::array< u16, 11U >{};
    mii_name.fill(0xA5A5U);
    FileSelectFunc::copyMiiName(mii_name.data(), mii);
    require(std::ranges::all_of(mii_name, [](u16 value) { return value == 0xA5A5U; }),
            "missing official RFL data must not copy a synthetic or cleared name");
    ++passed;

    std::cout << "File-select name real-or-absent tests passed: " << passed << "/3\n";
    return 0;
}
