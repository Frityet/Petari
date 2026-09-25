#include "Game/Util/FileUtil.hpp"
#include "Game/System/StationedFileInfo.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/System/ResourceHolderManager.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "OriginalStageResourceProcessFixture.hpp"
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
        auto& service = *SingletonHolder<ResourceHolderManager>::get();
        std::vector<ResourceHolder*> resources;
        for (auto* info = MR::getStationedFileInfoTable(); info->mArchive; ++info)
            if (info->mLoadType == 2) resources.push_back(service.createAndAdd(std::filesystem::path(info->mArchive).filename().c_str(), nullptr));
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
            require(resources[index]->mArchive == MR::receiveArchive((std::string("/ObjectData/") + std::string(expected_names[index])).c_str()),
                    "stationed archives must retain retail table order and exact resolved identity");
            require(!resources[index]->nativeResourceSource().entries().empty(),
                    "each real Mario stationed archive must contain a parsed RARC file table");
            auto& holder = *resources[index];
            require(holder.mArchive != nullptr && &holder.heap() == holder.mHeap &&
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
            require(count == holder.nativeResourceSource().entries().size() && total_size == holder.mTotalResourceSize,
                    "original resource tables cover every authored file exactly once");
            require(holder.mMotionResTable->mCount == 0 || holder.mBckCtrl != nullptr,
                    "motion archives construct the actual BckCtrl with retained control-table names");
        }

        std::size_t index = 0;
        for (auto* info = MR::getStationedFileInfoTable(); info->mArchive; ++info)
            if (info->mLoadType == 2)
                require(service.createAndAdd(std::filesystem::path(info->mArchive).filename().c_str(), nullptr) == resources[index++],
                        "the original manager deduplicates repeated stationed requests");

    }

}  // namespace

int main() {
    try {
        test_exact_mario_stationed_rows();
        std::cout << "[ok] exact Mario stationed table\n";
        const auto result = smgpc::test::run_stage_resource_process("stationed-archive", test_real_mario_stationed_archives);
        if (result) return result;
        std::cout << "[ok] real Mario stationed archives\n";
        std::cout << "Stationed archive real-or-absent tests passed (2/2).\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "[failed] " << error.what() << '\n';
        return 1;
    }
}
