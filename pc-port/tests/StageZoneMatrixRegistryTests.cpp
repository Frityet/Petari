#include "compat/StageZoneMatrixRegistry.hpp"

#include "Game/Util/SceneUtil.hpp"

#include <array>
#include <exception>
#include <iostream>
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

    template <typename Function>
    void require_rejected(Function&& function, std::string_view message) {
        try {
            function();
        } catch (const std::logic_error&) {
            return;
        }
        throw std::runtime_error(std::string(message));
    }

    JMapInfo make_row() {
        // One valid BCSV row; column values are irrelevant to table ownership.
        constexpr auto bytes = std::array<std::uint8_t, 20>{
            0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 4,
            0, 0, 0, 0,
        };
        return JMapInfo::from_bcsv(bytes);
    }

    smgpc::scene::StageHolderOccurrence make_holder(
        std::size_t id, std::optional<std::size_t> parent, s32 zone, float translation) {
        auto holder = smgpc::scene::StageHolderOccurrence{};
        holder.instance_id = id;
        holder.parent_instance_id = parent;
        holder.zone_id = zone;
        holder.zone_transform.matrix[3] = translation;
        return holder;
    }

    smgpc::scene::StagePlacementTable make_table(std::size_t holder, s32 zone) {
        auto table = smgpc::scene::StagePlacementTable{};
        table.holder_instance_id = holder;
        table.zone_id = zone;
        table.jmap_info = make_row();
        table.jmap_info.setPlacedZoneId(zone);
        return table;
    }

    void test_occurrences_and_lifetime() {
        auto holders = std::vector<smgpc::scene::StageHolderOccurrence>{
            make_holder(0, std::nullopt, 0, 0.0F),
            make_holder(1, 0, 4, 10.0F),
            make_holder(2, 0, 4, 20.0F),
            make_holder(3, 0, 8, 30.0F),
            make_holder(4, 1, 9, 40.0F),
        };
        holders[0].children = {1, 2, 3};
        holders[1].children = {4};
        const auto tables = std::vector<smgpc::scene::StagePlacementTable>{
            make_table(1, 4), make_table(2, 4), make_table(4, 9),
        };
        auto copied_info = tables[1].jmap_info;
        auto foreign_info = make_row();
        foreign_info.setPlacedZoneId(4);
        require_rejected([] { (void)MR::getZonePlacementMtx(0); },
                         "zone matrices must not exist outside an active stage binding");
        {
            auto binding = smgpc::compat::StageZoneMatrixBinding(holders, tables);
            auto* first = MR::getZonePlacementMtx(4);
            require(MR::getZonePlacementMtx(0)->mMtx[0][0] == 1.0F && first->mMtx[0][3] == 10.0F,
                    "zone IDs must select the root and the first matching immediate child");
            require(MR::getZonePlacementMtx(JMapInfoIter(&copied_info, 0))->mMtx[0][3] == 20.0F,
                    "copied JMap rows must retain exact repeated-zone holder ownership");
            require(MR::getZonePlacementMtx(8)->mMtx[0][3] == 30.0F,
                    "empty child holders must retain their authored placement matrices");
            require(MR::getZonePlacementMtx(JMapInfoIter(&tables[2].jmap_info, 0))->mMtx[0][3] == 40.0F,
                    "iterator ownership must reach a nested holder even when integer zone lookup cannot");
            require_rejected([] { (void)MR::getZonePlacementMtx(9); },
                             "integer zone lookup must preserve the original immediate-child boundary");
            require_rejected([] { (void)MR::getZonePlacementMtx(77); },
                             "unknown zone IDs must not produce a fabricated identity matrix");
            require_rejected([&] { (void)MR::getZonePlacementMtx(JMapInfoIter(&foreign_info, 0)); },
                             "equal zone metadata must not authorize a foreign JMap data owner");
            require_rejected([&] { (void)MR::getZonePlacementMtx(JMapInfoIter(&copied_info, 1)); },
                             "an out-of-range row must not acquire a holder matrix");
            first->mMtx[0][3] = 11.0F;
            require(MR::getZonePlacementMtx(JMapInfoIter(&tables[0].jmap_info, 0)) == first &&
                        first->mMtx[0][3] == 11.0F,
                    "integer and iterator lookups must expose the same stable mutable holder matrix");
            {
                const auto nested_holders = std::array{make_holder(0, std::nullopt, 0, 100.0F)};
                auto nested = smgpc::compat::StageZoneMatrixBinding(nested_holders, {});
                require(MR::getZonePlacementMtx(0)->mMtx[0][3] == 100.0F,
                        "a nested stage binding must expose its own holder catalog");
                require_rejected([&] { (void)MR::getZonePlacementMtx(JMapInfoIter(&copied_info, 0)); },
                                 "a nested stage must not borrow a suspended stage's JMap ownership");
            }
            require(MR::getZonePlacementMtx(4) == first && first->mMtx[0][3] == 11.0F,
                    "leaving a nested stage must restore the original registry and matrix addresses");
        }
        require_rejected([] { (void)MR::getZonePlacementMtx(0); },
                         "teardown must withdraw all stage-owned matrix lookup");
    }
}

int main() {
    try {
        test_occurrences_and_lifetime();
        std::cout << "stage zone matrix registry tests passed\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "stage zone matrix registry tests failed: " << exception.what() << '\n';
        return 1;
    }
}
