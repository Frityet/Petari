#include "Game/Util/MessageUtil.hpp"
#include "runtime/RuntimeServices.hpp"

#include <JSystem/JUtility/JUTVideo.hpp>

#include <array>
#include <exception>
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

    void test_missing_message_stays_absent() {
        auto messages = smgpc::runtime::MessageService{};
        constexpr auto missing = std::string_view{"Missing_Message_ID"};

        require(messages.message(missing) == nullptr && messages.message_utf16(missing) == nullptr &&
                    messages.message_raw_utf16(missing) == nullptr && messages.message_info(missing) == nullptr &&
                    messages.message_control_tags(missing) == nullptr,
                "a missing BMG identifier must have no text or metadata representation");
        require(messages.format_message_utf16(missing, {}).empty(),
                "formatting a missing BMG identifier must produce no visible text");

        require(MR::getSystemMessageDirect(missing.data()) == nullptr &&
                    MR::getGameMessageDirect(missing.data()) == nullptr &&
                    MR::getLayoutMessageDirect(missing.data()) == nullptr &&
                    !MR::isExistGameMessage(missing.data()),
                "the retail MessageUtil surface must return absence when no runtime message archive exists");
    }

    void test_present_message_retains_real_text() {
        auto messages = smgpc::runtime::MessageService{};
        messages.set_message("Known_Message_ID", u"Real localized text");

        const auto* utf16 = messages.message_utf16("Known_Message_ID");
        const auto* raw = messages.message_raw_utf16("Known_Message_ID");
        require(utf16 != nullptr && *utf16 == u"Real localized text" && raw != nullptr && *raw == u"Real localized text",
                "a present message must retain its archive-provided text");
        require(messages.format_message_utf16("Known_Message_ID", {}) == u"Real localized text",
                "formatting a present message must use its real raw text");
    }

    void test_video_manager_requires_runtime_owner() {
        require(JUTVideo::getManager() == nullptr,
                "JUTVideo lookup must not create a process-global substitute without its runtime owner");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };
}  // namespace

int main() {
    constexpr auto tests = std::array{
        TestCase{"missing message remains absent", test_missing_message_stays_absent},
        TestCase{"present message retains real text", test_present_message_retains_real_text},
        TestCase{"video manager requires runtime owner", test_video_manager_requires_runtime_owner},
    };

    auto failures = 0;
    for (const auto& test : tests) {
        try {
            test.run();
            std::cout << "[ok] " << test.name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[fail] " << test.name << ": " << error.what() << '\n';
        }
    }

    if (failures != 0) {
        std::cerr << failures << " message real-or-absent test(s) failed\n";
        return 1;
    }

    std::cout << tests.size() << " message real-or-absent test(s) passed\n";
    return 0;
}
