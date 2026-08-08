#include "Game/Map/FileSelectFunc.hpp"
#include "Game/Map/FileSelectIconID.hpp"
#include "Game/Map/FileSelectItemDelegator.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "JSystem/JGeometry/TVec.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    class DelegatorReceiver {
    public:
        void receive(FileSelectItem* item, s32 action) {
            received_item = item;
            received_action = action;
        }

        FileSelectItem* received_item = reinterpret_cast< FileSelectItem* >(1);
        s32 received_action = -1;
    };

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
    require(fellow.isFellow() && !fellow.isMii() && fellow.getFellowID() == FileSelectIconID::Mario,
            "the exact fellow identifier must retain its retail discriminator and payload");
    auto fellow_copy = FileSelectIconID(fellow);
    require(fellow_copy == fellow, "the exact icon copy constructor must preserve the identifier");
    auto fellow_name = std::array< u16, 11U >{};
    require_unavailable([&] { FileSelectFunc::copyMiiName(fellow_name.data(), fellow); },
                        "a fellow name must remain unavailable without its real BMG message");
    ++passed;

    auto mii = FileSelectIconID{};
    mii.setMiiIndex(0U);
    require(mii.isMii() && !mii.isFellow() && mii.getMiiIndex() == 0U,
            "the exact Mii identifier must retain its retail discriminator and payload");
    auto mii_name = std::array< u16, 11U >{};
    mii_name.fill(0xA5A5U);
    FileSelectFunc::copyMiiName(mii_name.data(), mii);
    require(std::ranges::all_of(mii_name, [](u16 value) { return value == 0xA5A5U; }),
            "missing official RFL data must not copy a synthetic or cleared name");
    ++passed;

    auto receiver = DelegatorReceiver{};
    auto delegator = FileSelectItemDelegator< DelegatorReceiver >(&receiver, &DelegatorReceiver::receive);
    delegator.notify(nullptr, 7);
    require(receiver.received_item == nullptr && receiver.received_action == 7,
            "the exact file-select item delegator must dispatch its retail callback arguments");
    ++passed;

    Mtx transform{};
    transform[0][3] = 12.0F;
    transform[1][3] = -4.0F;
    transform[2][3] = 30.0F;
    auto translation = TVec3f{};
    translation.setTrans(transform);
    require(translation.x == 12.0F && translation.y == -4.0F && translation.z == 30.0F,
            "TVec3::setTrans must read the retail matrix translation column");
    ++passed;

    std::cout << "File-select name real-or-absent tests passed: " << passed << "/5\n";
    return 0;
}
