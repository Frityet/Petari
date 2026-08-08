#include "Game/Util/MessageUtil.hpp"
#include "resource/BmgMessageArchive.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeServices.hpp"

#include <JSystem/JUtility/JUTVideo.hpp>

#include <array>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    [[nodiscard]] std::optional<std::filesystem::path> find_real_message_archive() {
        for (auto root = std::filesystem::current_path(); !root.empty(); root = root.parent_path()) {
            const std::filesystem::path candidates[]{
                root / "orig/RMGK02/files/KrKorean/MessageData/Message.arc",
                root / "orig/RMGK01/files/KrKorean/MessageData/Message.arc",
                root / "container/orig/RMGK01/files/KrKorean/MessageData/Message.arc",
                root / "pc-port/container/orig/RMGK01/files/KrKorean/MessageData/Message.arc",
            };
            for (const auto &candidate : candidates) {
                auto error = std::error_code{};
                if (std::filesystem::is_regular_file(candidate, error)) {
                    return candidate;
                }
            }
            if (root == root.root_path()) {
                break;
            }
        }
        return std::nullopt;
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

    void test_retail_date_and_time_tags(const std::filesystem::path &archive_path) {
        const auto archive = smgpc::resource::RarcArchive::from_file(archive_path);
        const auto messages = smgpc::resource::BmgMessageArchive::from_message_archive(archive);

        const auto format = [&messages](std::string_view id, std::vector<smgpc::resource::BmgFormatArg> args) {
            const auto *message = messages.find(id);
            require(message != nullptr, "retail Message.arc is missing a required date/time message");
            require(!message->control_tags.empty(), "retail date/time text must retain its MessageEditor control tags");
            for (const auto &tag : message->control_tags) {
                require(tag.type == 6U && tag.size_bytes == 14U,
                        "retail date/time placeholders must be the RMGK02 number-group tag format");
            }
            return smgpc::resource::format_bmg_text(message->raw_text, args);
        };

        require(format("System_Date000", {smgpc::resource::BmgFormatArg::number(2026),
                                           smgpc::resource::BmgFormatArg::number(8),
                                           smgpc::resource::BmgFormatArg::number(7)}) == u"2026/08/07",
                "retail System_Date000 tags must select the year, month, and day arguments");
        require(format("System_Time002", {smgpc::resource::BmgFormatArg::number(14),
                                           smgpc::resource::BmgFormatArg::number(5)}) == u"14:05",
                "retail System_Time002 tags must retain two-digit minute formatting");
        require(format("System_Time001", {smgpc::resource::BmgFormatArg::number(0),
                                           smgpc::resource::BmgFormatArg::number(2),
                                           smgpc::resource::BmgFormatArg::number(3)}) == u"02:03",
                "retail System_Time001 tags must select minute and second arguments");
        require(format("System_Time000", {smgpc::resource::BmgFormatArg::number(0),
                                           smgpc::resource::BmgFormatArg::number(2),
                                           smgpc::resource::BmgFormatArg::number(3),
                                           smgpc::resource::BmgFormatArg::number(4)}) == u"02:03:04",
                "retail System_Time000 tags must select minute, second, and centisecond arguments");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };
}  // namespace

int main() {
    const auto tests = std::array{
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

    if (const auto archive_path = find_real_message_archive(); archive_path.has_value()) {
        try {
            test_retail_date_and_time_tags(*archive_path);
            std::cout << "[ok] retail date/time message tags\n";
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[fail] retail date/time message tags: " << error.what() << '\n';
        }
    } else {
        std::cout << "[skip] retail date/time message tags\n";
    }

    if (failures != 0) {
        std::cerr << failures << " message real-or-absent test(s) failed\n";
        return 1;
    }

    std::cout << tests.size() << " message real-or-absent test(s) passed\n";
    return 0;
}
