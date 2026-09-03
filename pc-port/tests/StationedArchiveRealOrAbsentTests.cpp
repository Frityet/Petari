#include "Game/System/StationedFileInfo.hpp"
#include "compat/ResourceHolderCompat.hpp"
#include "resource/GameResourceRuntime.hpp"
#include <aurora/aurora.h>
#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeServices.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace aurora { extern AuroraConfig g_config; }

namespace {

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    [[nodiscard]] std::optional<std::filesystem::path> find_real_disc() {
        if (const auto *configured = std::getenv("SMGPC_REAL_DISC");
            configured != nullptr && configured[0] != '\0') {
            return std::filesystem::path(configured);
        }

        auto error = std::error_code{};
        auto directory = std::filesystem::current_path(error);
        if (error) {
            return std::nullopt;
        }
        while (true) {
            for (const auto name : {"RMGK01.iso", "RMGK01.wbfs"}) {
                const auto candidate = directory / name;
                if (std::filesystem::is_regular_file(candidate, error) && !error) {
                    return candidate;
                }
                error.clear();
            }
            const auto parent = directory.parent_path();
            if (parent == directory || parent.empty()) {
                break;
            }
            directory = parent;
        }
        return std::nullopt;
    }

    void test_exact_mario_stationed_rows() {
        constexpr auto expected = std::array{
            std::string_view{"/ObjectData/MarioAnime.arc"},
            std::string_view{"/ObjectData/BoneMario.arc"},
            std::string_view{"/ObjectData/Mario.arc"},
            std::string_view{"/ObjectData/MarioFace.arc"},
            std::string_view{"/ObjectData/MarioShadow.arc"},
            std::string_view{"/ObjectData/MarioTornado.arc"},
        };

        auto index = std::size_t{};
        for (auto *info = MR::getStationedFileInfoTable(); info->mArchive != nullptr; ++info) {
            if (info->mLoadType != 2) {
                continue;
            }
            require(index < expected.size(), "retail Mario stationed table gained an unexpected row");
            require(info->mArchive == expected[index], "retail Mario stationed archive order changed");
            ++index;
        }
        require(index == expected.size(), "retail Mario stationed archive set must contain six rows");
    }

    void test_real_mario_stationed_archives() {
        const auto disc_path = find_real_disc();
        if (!disc_path.has_value()) {
            std::cout << "[skip] real Mario stationed archive test (set SMGPC_REAL_DISC or place RMGK01.iso in a workspace ancestor)\n";
            return;
        }

        aurora_dvd_close();
        const auto path = disc_path->string();
        require(aurora_dvd_open(path.c_str()), "the selected real-disc fixture should be a readable SMG image");
        struct DiscCloseGuard final {
            ~DiscCloseGuard() {
                aurora_dvd_close();
            }
        } close_guard;
        DVDInit();

        auto dvd = smgpc::runtime::DvdFileSystemService{"/"};
        aurora::g_config.mem1Size = 24U * 1024U * 1024U;
        auto resource_runtime = smgpc::resource::GameResourceRuntime{};
        auto service = smgpc::compat::ResourceHolderService{dvd, resource_runtime.create_cohort(), resource_runtime.mem1_heap()};
        const auto resources = service.create_and_add_stationed(2);
        require(resources.size() == 6U, "stationed load type 2 must resolve all six retail Mario archives");

        constexpr auto expected_names = std::array{
            std::string_view{"MarioAnime.arc"},
            std::string_view{"BoneMario.arc"},
            std::string_view{"Mario.arc"},
            std::string_view{"MarioFace.arc"},
            std::string_view{"MarioShadow.arc"},
            std::string_view{"MarioTornado.arc"},
        };
        for (auto index = std::size_t{}; index < resources.size(); ++index) {
            require(resources[index] != nullptr, "a real stationed archive must produce a ResourceHolder");
            require(service.backing(*resources[index]).resolved_path().filename() == expected_names[index],
                    "stationed archives must retain retail table order and exact resolved identity");
            require(!service.backing(*resources[index]).archive().entries().empty(),
                    "each real Mario stationed archive must contain a parsed RARC file table");
            auto& holder = *resources[index];
            require(holder.mArchive != nullptr && holder.mHeap == &service.allocation_domain()->heap() &&
                        JKRHeap::findFromRoot(&holder) == holder.mHeap,
                    "stationed resources must construct the actual Game holder in their shared archive cohort");
            std::size_t count = 0, total_size = 0;
            for (const auto* table : {holder.mModelResTable, holder.mMotionResTable, holder.mBtkResTable,
                                      holder.mBpkResTable, holder.mBtpResTable, holder.mBlkResTable,
                                      holder.mBrkResTable, holder.mBasResTable, holder.mBmtResTable,
                                      holder.mBvaResTable, holder.mBanmtResTable, holder.mFileInfoTable}) {
                count += table->mCount;
                for (u32 row = 0; row < table->mCount; ++row) {
                    const auto* info = table->getFileInfo(row);
                    require(info->_8 == holder.mArchive->getResource(static_cast<u16>(info->_C)) &&
                                info->_4 == holder.mArchive->getResSize(info->_8),
                            "every original table row retains its authored file ID, raw pointer and size");
                    total_size += info->_4;
                }
            }
            require(count == service.backing(holder).archive().entries().size() && total_size == holder.mTotalResourceSize,
                    "original resource tables cover every authored file exactly once");
            require(holder.mMotionResTable->mCount == 0 || holder.mBckCtrl != nullptr,
                    "motion archives construct the actual BckCtrl with retained control-table names");
        }

        const auto repeated = service.create_and_add_stationed(2);
        require(repeated.size() == resources.size(), "repeated stationed loads must preserve the exact row set");
        for (auto index = std::size_t{}; index < resources.size(); ++index) {
            require(repeated[index] == resources[index],
                    "ResourceHolder ownership must deduplicate repeated stationed archive requests");
        }
        require(service.create_and_add_stationed(0x7fffffff).empty(),
                "an unknown stationed load type must remain genuinely absent");
        std::cout << "stationed cohort used=" << service.allocation_domain()->heap().mSize -
                         service.allocation_domain()->heap().getTotalFreeSize()
                  << " MEM1 available=" << resource_runtime.mem1_heap()->available_bytes() << '\n';
    }

}  // namespace

int main() {
    try {
        test_exact_mario_stationed_rows();
        std::cout << "[ok] exact Mario stationed table\n";
        test_real_mario_stationed_archives();
        std::cout << "[ok] real Mario stationed archives\n";
        std::cout << "Stationed archive real-or-absent tests passed (2/2).\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "[failed] " << error.what() << '\n';
        return 1;
    }
}
