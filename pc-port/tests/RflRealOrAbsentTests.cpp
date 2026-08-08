#include "resource/RarcArchive.hpp"
#include "runtime/RflService.hpp"

#include <RVLFaceLib.h>
#include <aurora/rfl/ResourceArchive.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
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

    [[nodiscard]] bool is_zeroed(const void *value, std::size_t size) {
        const auto *bytes = static_cast<const std::uint8_t *>(value);
        return std::all_of(bytes, bytes + size, [](std::uint8_t byte) { return byte == 0U; });
    }

    [[nodiscard]] std::optional<std::filesystem::path> find_real_mii_face_archive() {
        for (auto root = std::filesystem::current_path(); !root.empty(); root = root.parent_path()) {
            const std::filesystem::path candidates[]{
                root / "container/orig/RMGK01/files/ObjectData/MiiFaceDatabase.arc",
                root / "orig/RMGK02/files/ObjectData/MiiFaceDatabase.arc",
                root / "pc-port/container/orig/RMGK01/files/ObjectData/MiiFaceDatabase.arc",
            };
            for (const auto &candidate : candidates) {
                std::error_code error {};
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

    void test_missing_database_stays_empty_and_reports_no_data() {
        auto nand = smgpc::runtime::NandFileSystemService{};
        auto rfl = smgpc::runtime::RflService{nand};
        rfl.begin_frame(37U);

        require(smgpc::runtime::RflService::work_size(false) == 0x4CF24U &&
                    smgpc::runtime::RflService::work_size(true) == 0x65F24U,
                "RFL work sizes must match the retail SDK heaps plus its PPC manager");
        require(smgpc::runtime::RflService::model_buffer_size(RFLResolution_64, RFLExpFlag_Normal) == 0xA280U &&
                    smgpc::runtime::RflService::model_buffer_size(
                        RFLResolution_256M, RFLExpFlag_Normal | RFLExpFlag_Blink) == 0x5D2A0U,
                "RFL model-buffer sizes must use the retail resource, texture-object, mask, and alignment formula");
        const auto retail_red = RFLGetFavoriteColor(RFLFavoriteColor_Red);
        const auto retail_black = RFLGetFavoriteColor(RFLFavoriteColor_Black);
        require(retail_red.r == 184U && retail_red.g == 64U && retail_red.b == 48U && retail_red.a == 255U &&
                    retail_black.r == 24U && retail_black.g == 24U && retail_black.b == 20U && retail_black.a == 255U,
                "favorite-color values must match the retail SDK table");

        const auto &status = rfl.db_status();
        require(status.nand_bound, "the RFL service should retain its real NAND binding");
        require(!status.db_present && status.entry_count == 0U,
                "a missing RFL_DB.dat must remain an empty database");
        require(status.last_error == RFLErrcode_DBNodata,
                "a missing RFL_DB.dat must report DBNodata");
        require(rfl.valid_miis().empty(),
                "a missing RFL_DB.dat must not manufacture an official or default Mii");
        require(!status.resource_initialized && !rfl.available() && rfl.async_status() == RFLErrcode_NotAvailable,
                "constructing an RFL service must not pretend that RFL_Res.dat was initialized");

        auto info = RFLAdditionalInfo{};
        std::memset(&info, 0xA5, sizeof(info));
        require(rfl.additional_info(info, RFLDataSource_Official, nullptr, 0U) == RFLErrcode_DBNodata,
                "missing official Mii info must report DBNodata");
        require(is_zeroed(&info, sizeof(info)),
                "missing official Mii info must not leak fabricated identity fields");

        auto index = u16{0xFFFFU};
        const auto empty_id = RFLCreateID{};
        require(!rfl.search_official_data(empty_id, index) && index == 0xFFFFU,
                "an empty create ID must not resolve to a fabricated official Mii");
        require(!rfl.is_available_official_data(0U),
                "official index zero must remain unavailable when the database is absent");

        auto model = RFLCharModel{};
        auto model_work = std::array<std::uint8_t, 32U>{};
        require(rfl.init_char_model(model, RFLDataSource_Official, nullptr, 0U, model_work.data(),
                                    RFLResolution_64, RFLExpFlag_Normal) == RFLErrcode_NotAvailable,
                "a character model cannot initialize before a real face resource exists");
        require(model.initialized == FALSE,
                "a failed character-model request must remain uninitialized");

        const auto setting = RFLIconSetting{
            .width = 64U,
            .height = 64U,
            .bgType = RFLIconBG_Direct,
            .bgColor = GXColor{0U, 0U, 0U, 0U},
            .drawXluOnly = FALSE,
        };
        const auto icon = rfl.make_icon_texture(RFLDataSource_Official, nullptr, 0U,
                                                RFLExp_Normal, setting);
        require(icon.result == RFLErrcode_NotAvailable && !icon.texture_available && icon.rgb5a3.empty(),
                "a Mii icon must be absent while the real face resource is unavailable");

        rfl.clear_trace();
        const auto page = rfl.mii_select_page_state({}, std::nullopt, 0U, std::nullopt);
        require(page.icons.empty() && page.icon_count == 0U && page.page_count == 0U,
                "the Mii selector must expose an empty page when no real records exist");
        require(!rfl.trace().empty() && rfl.trace().back().kind == smgpc::runtime::RflOperationKind::MiiSelectPage &&
                    rfl.trace().back().result == RFLErrcode_NotAvailable,
                "the empty Mii page trace must preserve the missing-resource result");
    }

    void test_real_resource_init_is_independent_from_missing_database() {
        auto nand = smgpc::runtime::NandFileSystemService{};
        auto malformed_rfl = smgpc::runtime::RflService{nand};
        auto work = std::vector<std::uint8_t>(smgpc::runtime::RflService::work_size(false));
        auto malformed_resource = std::array<std::uint8_t, 4U>{1U, 2U, 3U, 4U};
        require(malformed_rfl.db_status().last_error == RFLErrcode_DBNodata,
                "the retry regression must begin with a populated NAND status cache");
        const auto malformed_archive = aurora::rfl::ResourceArchive::copy_from(malformed_resource);
        require(!malformed_archive.valid() &&
                    malformed_archive.error() == aurora::rfl::ResourceArchiveError::HeaderTooSmall,
                "the generalized RFL archive parser must preserve a precise malformed-header result");
        require(malformed_rfl.init_resources(work.data(), malformed_resource.data(), malformed_resource.size(), false, false) ==
                    RFLErrcode_Broken,
                "arbitrary non-empty bytes must not be accepted as an initialized RFL resource");
        require(!malformed_rfl.available() && malformed_rfl.async_status() == RFLErrcode_Broken &&
                    !malformed_rfl.db_status().resource_initialized,
                "a malformed RFL resource must remain unavailable and report an error");

        auto exit_rfl = smgpc::runtime::RflService{nand};
        require(exit_rfl.init_resources(work.data(), malformed_resource.data(), malformed_resource.size(), false, false) ==
                    RFLErrcode_Broken,
                "the exit regression must begin with a real malformed-resource failure");
        exit_rfl.exit();
        require(!exit_rfl.available() && exit_rfl.async_status() == RFLErrcode_NotAvailable &&
                    exit_rfl.last_reason() == 0 && exit_rfl.db_status().last_reason == 0,
                "RFLExit must clear a prior resource failure instead of leaking Broken into the next lifetime");

        const auto archive_path = find_real_mii_face_archive();
        if (!archive_path.has_value()) {
            std::cout << "[skip] real MiiFaceDatabase.arc resource check\n";
            return;
        }

        const auto archive = smgpc::resource::RarcArchive::from_file(*archive_path);
        const auto resource = archive.resource_data("/RFL_Res.dat");
        require(!resource.empty(), "the retail MiiFaceDatabase archive must contain RFL_Res.dat");

        require(malformed_rfl.init_resources(work.data(), resource.data(), resource.size(), false, false) ==
                    RFLErrcode_Success &&
                    malformed_rfl.available() && malformed_rfl.resource_archive() != nullptr,
                "a valid resource retry must replace a cached malformed-resource error on the same service");

        auto rfl = smgpc::runtime::RflService{nand};
        rfl.begin_frame(90U);
        require(rfl.init_resources(work.data(), resource.data(), resource.size(), false, true) == RFLErrcode_Busy,
                "the real retail face resource should begin asynchronous initialization");
        require(rfl.async_status() == RFLErrcode_Busy && !rfl.available(),
                "the face resource must not report success before its asynchronous completion");
        const auto init_trace = std::find_if(rfl.trace().begin(), rfl.trace().end(), [](const auto &operation) {
            return operation.kind == smgpc::runtime::RflOperationKind::InitResource;
        });
        require(init_trace != rfl.trace().end() &&
                    init_trace->path == "/ObjectData/MiiFaceDatabase.arc/RFL_Res.dat" &&
                    init_trace->byte_count == resource.size(),
                "resource initialization evidence must identify the actual disc archive entry");

        rfl.begin_frame(91U);
        const auto &status = rfl.db_status();
        require(rfl.async_status() == RFLErrcode_Success && rfl.available() && status.resource_initialized,
                "the real retail face resource should initialize successfully");
        const auto *owned_archive = rfl.resource_archive();
        require(owned_archive != nullptr && owned_archive->valid() && owned_archive->version() != 0U &&
                    owned_archive->bytes().size() == resource.size() && owned_archive->bytes().data() != resource.data(),
                "RFL must own a parsed copy of the retail resource instead of borrowing the RARC buffer");
        for (auto archive_index = std::size_t{};
             archive_index < aurora::rfl::ResourceArchive::archive_count; ++archive_index) {
            const auto archive_id = static_cast<aurora::rfl::ResourceArchiveId>(archive_index);
            const auto &section = owned_archive->section(archive_id);
            const auto first_file = owned_archive->file(archive_id, 0U);
            require(section.file_count != 0U && section.largest_file_size != 0U &&
                        first_file.has_value() && !first_file->empty(),
                    "every retail RFL resource archive must expose its first real file");
            require(!owned_archive->file(archive_id, section.file_count).has_value(),
                    "RFL resource file lookup must reject an out-of-range index");
        }
        require(!status.db_present && status.last_error == RFLErrcode_DBNodata && rfl.valid_miis().empty(),
                "successful RFL_Res initialization must remain independent from an absent user RFL_DB.dat");

        auto model = RFLCharModel{};
        require(rfl.init_char_model(model, RFLDataSource_Official, nullptr, 0U, work.data(),
                                    RFLResolution_64, RFLExpFlag_Normal) == RFLErrcode_DBNodata &&
                    model.initialized == FALSE,
                "a real face resource must not manufacture a character when the user database is empty");

        auto icon_buffer = std::array<std::uint8_t, 64U>{};
        icon_buffer.fill(0xA5U);
        const auto icon_before = icon_buffer;
        const auto icon_setting = RFLIconSetting{
            .width = 64U,
            .height = 64U,
            .bgType = RFLIconBG_Direct,
            .bgColor = GXColor{0U, 0U, 0U, 0U},
            .drawXluOnly = FALSE,
        };
        require(rfl.make_icon(icon_buffer.data(), RFLDataSource_Official, nullptr, 0U, RFLExp_Normal, icon_setting) ==
                    RFLErrcode_DBNodata && icon_buffer == icon_before,
                "an absent Mii icon must not procedurally draw into the caller's buffer");

        rfl.clear_trace();
        const auto page = rfl.mii_select_page_state({}, std::nullopt, 0U, std::nullopt);
        require(page.icons.empty() && !rfl.trace().empty() && rfl.trace().back().result == RFLErrcode_DBNodata,
                "an initialized face resource with no user database must expose an empty no-data selector page");

        rfl.exit();
        require(rfl.resource_archive() == nullptr && !rfl.available(),
                "RFLExit must release the owned resource archive and invalidate availability");
    }

    void test_private_host_database_format_is_not_accepted_at_retail_path() {
        auto nand = smgpc::runtime::NandFileSystemService{};
        const auto private_header = std::array<std::uint8_t, 8U>{'S', 'R', 'F', 'L', 0U, 1U, 0U, 0U};
        nand.write_file(smgpc::runtime::NandFileSystemService::rfl_db_path(), private_header);
        nand.clear_trace();

        auto rfl = smgpc::runtime::RflService{nand};
        const auto &status = rfl.db_status();
        require(status.db_present && status.byte_count == private_header.size() &&
                    status.last_error == RFLErrcode_NotAvailable && status.entry_count == 0U,
                "the removed private SRFL format must be recognized only as unsupported data at the retail path");
        require(rfl.valid_miis().empty(),
                "private host bytes at RFL_DB.dat must not create user Mii records");
        require(std::none_of(nand.trace().begin(), nand.trace().end(), [](const auto &operation) {
                    return operation.kind == smgpc::runtime::NandOperationKind::Write;
                }),
                "loading RFL_DB.dat must not rewrite it using a private host format");
    }

    void test_no_runtime_never_reports_rfl_success() {
        auto work = std::vector<std::uint8_t>(RFLGetWorkSize(FALSE));
        auto resource = std::array<std::uint8_t, 4U>{1U, 2U, 3U, 4U};
        require(!work.empty(), "the SDK work-size query is data-independent and must remain available");
        require(RFLGetModelBufferSize(RFLResolution_64, RFLExpFlag_Normal) != 0U,
                "the SDK model-size query is data-independent and must remain available");
        require(RFLInitRes(work.data(), resource.data(), static_cast<u32>(resource.size()), FALSE) ==
                    RFLErrcode_NotAvailable,
                "synchronous RFL initialization must not succeed without an active runtime");
        require(RFLInitResAsync(work.data(), resource.data(), static_cast<u32>(resource.size()), FALSE) ==
                    RFLErrcode_NotAvailable,
                "asynchronous RFL initialization must not succeed without an active runtime");
        require(RFLAvailable() == FALSE && RFLGetAsyncStatus() == RFLErrcode_NotAvailable &&
                    RFLWaitAsync() == RFLErrcode_NotAvailable,
                "availability and wait APIs must report an absent runtime");

        auto info = RFLAdditionalInfo{};
        std::memset(&info, 0xA5, sizeof(info));
        require(RFLGetAdditionalInfo(&info, RFLDataSource_Official, nullptr, 0U) == RFLErrcode_DBNodata &&
                    is_zeroed(&info, sizeof(info)),
                "the no-runtime info query must return empty DBNodata output");

        auto model = RFLCharModel{};
        require(RFLInitCharModel(&model, RFLDataSource_Official, nullptr, 0U, work.data(),
                                 RFLResolution_64, RFLExpFlag_Normal) == RFLErrcode_NotAvailable,
                "the no-runtime character-model API must report NotAvailable");
        auto icon_buffer = std::vector<std::uint8_t>(0x4000U, 0U);
        const auto setting = RFLIconSetting{
            .width = 64U,
            .height = 64U,
            .bgType = RFLIconBG_Direct,
            .bgColor = GXColor{0U, 0U, 0U, 0U},
            .drawXluOnly = FALSE,
        };
        require(RFLMakeIcon(icon_buffer.data(), RFLDataSource_Official, nullptr, 0U,
                            RFLExp_Normal, &setting) == RFLErrcode_NotAvailable,
                "the no-runtime icon API must report NotAvailable");

    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };
}  // namespace

int main() {
    constexpr auto tests = std::array{
        TestCase{"missing database is empty", test_missing_database_stays_empty_and_reports_no_data},
        TestCase{"real resource is independent from user database", test_real_resource_init_is_independent_from_missing_database},
        TestCase{"private database format is rejected", test_private_host_database_format_is_not_accepted_at_retail_path},
        TestCase{"no-runtime APIs report absence", test_no_runtime_never_reports_rfl_success},
    };

    auto failures = 0;
    for (const auto &test : tests) {
        try {
            test.run();
            std::cout << "[pass] " << test.name << '\n';
        } catch (const std::exception &error) {
            ++failures;
            std::cerr << "[fail] " << test.name << ": " << error.what() << '\n';
        }
    }

    if (failures != 0) {
        std::cerr << failures << " RFL real-or-absent test(s) failed\n";
        return 1;
    }

    std::cout << tests.size() << " RFL real-or-absent tests passed\n";
    return 0;
}
