#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/Scene/PlacementInfoOrdered.hpp"
#include "scene/OriginalPlacementCoverage.hpp"
#include "scene/nameobj/NameObjFactory.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
    void require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }

    JMapInfo table(const char* name, s32 zone) {
        const auto bytes = std::array<std::uint8_t, 16>{0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0};
        auto info = JMapInfo::from_bcsv(bytes);
        info.setName(name);
        info.setPlacedZoneId(zone);
        return info;
    }

    // Original queues use scene-heap reclamation. This standalone owner frees
    // their arrays explicitly after the original non-owning links retire.
    struct Queue final : PlacementInfoOrdered {
        Queue() : PlacementInfoOrdered(16) {}
        ~Queue() {
            delete[] mIndexArray;
            delete[] mSetArray;
            delete[] mIdentiferArray;
        }
    };

    void test_actual_queue_membership_and_strict_boundary() {
        using namespace smgpc::scene;
        using Availability = OriginalPlacementAvailability;
        require(nameobj::original_name_obj_registered("rEsTaRtCuBe") &&
                    NameObjFactory::getCreator("RestartCube") == nullptr,
                "null-archive retail actors must remain distinguishable from genuinely unknown names");
        require(!nameobj::original_name_obj_registered("UnregisteredCoverageProbe") &&
                    NameObjFactory::getCreator("UnregisteredCoverageProbe") == nullptr,
                "unknown names must retain original null-creator behavior");

        auto actors = table("ObjInfo", 3);
        auto areas = table("AreaObjInfo", 4);
        auto demos = table("jmp/placement/layera/DemoObjInfo.bcsv", 8);
        auto zones = table("STAGEOBJINFO", 0);
        auto misleading = table("MyDemoObjInfo", 5);
        auto first = Queue{};
        auto second = Queue{};
        first.insert({"Mario", -1}, {&actors, 0});
        first.insert({"RestartCube", -1}, {&areas, 0});
        first.insert({"RestartCube", -1}, {&areas, 0});
        first.insert({"UnregisteredCoverageProbe", -1}, {&actors, 0});
        first.insert({"DemoGroup", -1}, {&demos, 0});
        first.insert({"UnregisteredCoverageProbe", -1}, {&zones, 0});
        second.insert({"AssemblyBlock", 2}, {&actors, 0});
        second.insert({"UnregisteredCoverageProbe", 2}, {&actors, 0});
        second.insert({"RestartCube", -1}, {&misleading, 0});
        const auto queues = std::array{
            OriginalPlacementQueue{"first", &first}, OriginalPlacementQueue{"second", &second},
        };
        const auto before_first = first._4;
        const auto before_second = second._4;
        const auto entries = inspect_original_placement_queues(queues);
        require(entries.size() == 9 && first._4 == before_first && second._4 == before_second,
                "coverage must retain every real link, including repeated occurrences, without modifying the queues");
        std::array<std::size_t, 4> counts{};
        for (const auto& entry : entries) ++counts[static_cast<std::size_t>(entry.availability)];
        require(counts == std::array<std::size_t, 4>{2, 4, 2, 1},
                "coverage must distinguish ordinary/model creators and exclude only exact non-actor table names");
        const auto demo = std::ranges::find_if(entries, [](const auto& entry) { return entry.object == "DemoGroup"; });
        require(demo != entries.end() && demo->availability == Availability::Supported,
                "DemoObjInfo must retain actual original factory actors rather than inherit standalone-host metadata policy");
        require(entries.front().phase == "first" && entries.front().object == "Mario" &&
                    entries.front().zone == 3 && entries.front().row == 0 && entries.back().phase == "second",
                "coverage must preserve original queue order and authored row identity");
        bool rejected = false;
        try { require_original_placement_closure(entries); }
        catch (const std::runtime_error& error) {
            rejected = std::string_view(error.what()).find("RestartCube") != std::string_view::npos;
        }
        require(rejected, "strict validation must reject a known-unlinked real actor before construction");
        std::vector<OriginalPlacementCoverageEntry> no_unlinked;
        for (const auto& entry : entries)
            if (entry.availability != Availability::KnownUnlinked) no_unlinked.push_back(entry);
        require_original_placement_closure(no_unlinked);
        require(NameObjFactory::getCreator("UnregisteredCoverageProbe") == nullptr,
                "strict coverage must not turn unknown rows into fabricated successful actors");

        // Corrupt the retained count, not a duplicate scanner fixture, to prove
        // the report cannot silently count a truncated/cyclic original group.
        auto* group = first.mIdentiferArray[0];
        ++group->mList.mCount;
        rejected = false;
        try { (void)inspect_original_placement_queues(queues); }
        catch (const std::logic_error&) { rejected = true; }
        --group->mList.mCount;
        require(rejected, "coverage must reject inconsistent original linked-row ownership");
    }
}

int main() try {
    test_actual_queue_membership_and_strict_boundary();
    std::puts("PASS original placement coverage and strict boundary");
    return 0;
} catch (const std::exception& error) {
    std::fprintf(stderr, "FAIL original placement coverage: %s\n", error.what());
    return 1;
}
