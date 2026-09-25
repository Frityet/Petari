#include "Game/Map/HitInfo.hpp"
#include "Game/Map/CollisionParts.hpp"
#include "OriginalStageResourceProcessFixture.hpp"
#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/Scene/StageDataHolder.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Map/CollisionCategorizedKeeper.hpp"
#include "Game/Util/CollisionPartsFilter.hpp"
#include <aurora/allocation.hpp>
#include <aurora/main.h>
#include "Game/AreaObj/AreaObj.hpp"
#include "Game/AreaObj/AreaObjContainer.hpp"
#include "Game/Map/FileSelector.hpp"
#include "Game/MapObj/InvisiblePolygonObj.hpp"
#include "Game/MapObj/InvisiblePolygonObjGCapture.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/NameObj/NameObjArchiveListCollector.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/NPC/DemoRabbit.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/FileUtil.hpp"
#include "JSystem/JKernel/JKRMemArchive.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/System/ResourceHolderManager.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "resource/GameResourceRuntime.hpp"
#include <aurora/aurora.h>
#include "runtime/RuntimeServices.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Util/MapUtil.hpp"

#include <aurora/dvd.h>
#include <aurora/exception.hpp>
#include <dolphin/dvd.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <stdlib.h>
#include <unistd.h>
#include <cstdio>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

static_assert(std::is_base_of_v<LiveActor, FileSelector>,
              "the exact retail FileSelector declaration must coexist with the host boundary");

namespace aurora { extern AuroraConfig g_config; }

namespace {

    void require(bool condition, std::string_view message) {
        if (!condition) {
            aurora::throw_host_exception<std::runtime_error>(std::string(message));
        }
    }

    class ScopedEnvironmentVariable final {
    public:
        ScopedEnvironmentVariable(const char *name, std::string value) : _name(name) {
            if (const auto *previous = std::getenv(name); previous != nullptr) {
                _previous = previous;
            }
            if (setenv(_name.c_str(), value.c_str(), 1) != 0) {
                throw std::runtime_error("could not set test environment variable " + _name);
            }
        }

        ~ScopedEnvironmentVariable() {
            if (_previous.has_value()) {
                (void)setenv(_name.c_str(), _previous->c_str(), 1);
            } else {
                (void)unsetenv(_name.c_str());
            }
        }

    private:
        std::string _name;
        std::optional<std::string> _previous;
    };

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

    [[nodiscard]] bool line_query_hits_registered_wall(
        Triangle* hit = nullptr, const CollisionPartsFilterBase* filter = nullptr) {
        constexpr auto coordinates = std::array{-750.0F, -500.0F, -250.0F, 0.0F,
                                                250.0F, 500.0F, 750.0F, 1000.0F};
        for (auto axis = 0U; axis < 3U; ++axis) {
            for (const auto first : coordinates) {
                for (const auto second : coordinates) {
                    for (const auto direction : {-1.0F, 1.0F}) {
                        auto start = TVec3f{};
                        auto offset = TVec3f{};
                        auto *start_values = &start.x;
                        auto *offset_values = &offset.x;
                        start_values[axis] = -direction * 2000.0F;
                        offset_values[axis] = direction * 4000.0F;
                        start_values[(axis + 1U) % 3U] = first;
                        start_values[(axis + 2U) % 3U] = second;
                        TVec3f position;
                        Triangle triangle;
                        if (MR::getFirstPolyOnLineToMap(&position, &triangle, start, offset, filter, nullptr)) {
                            if (hit != nullptr) {
                                *hit = triangle;
                            }
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

#ifndef NDEBUG
    // This deliberately injects one unchanged FileSelect row into a bounded
    // original-process test. It does not claim the wall is authored in Gateway.
    class PlacementTableRangeBinding final {
    public:
        PlacementTableRangeBinding(StageDataHolder& holder, const JMapInfo& table)
            : _holder(holder), _begin(holder._E4), _end(holder._E8) {
            const auto begin = reinterpret_cast<std::uintptr_t>(table.getData());
            const auto size = table.getDataSize();
            require(begin && size && begin + size > begin,
                    "Injected placement requires its retained exact JMap byte range");
            _holder._E4 = begin;
            _holder._E8 = begin + size;
        }
        ~PlacementTableRangeBinding() {
            _holder._E4 = _begin;
            _holder._E8 = _end;
        }
    private:
        StageDataHolder& _holder;
        std::uintptr_t _begin;
        std::uintptr_t _end;
    };

    class OnlyWall final : public CollisionPartsFilterBase {
    public:
        explicit OnlyWall(const CollisionParts* parts) : _parts(parts) {}
        bool isInvalidParts(const CollisionParts* parts) const override { return parts != _parts; }
    private:
        const CollisionParts* _parts;
    };

    struct WallProbe {
        bool exercised = false;
        const InvisiblePolygonObj* retired_actor = nullptr;
        Triangle retired_triangle;

        void after_frame(GameSystem& system, std::uint64_t frame) {
            // Inject only at the terminal completed frame, after all ordinary
            // game callbacks. All temporary owners retire before returning.
            if (exercised || frame != 119) return;
            auto* controller = system.mSceneController;
            require(controller && controller->mSceneInitializeState == SceneInitializeState_End &&
                        controller->getCurrentSceneForExecute() == controller->mScene &&
                        dynamic_cast<GameScene*>(controller->mScene) && MR::isEqualStageName("HeavensDoorGalaxy"),
                    "Terminal wall probe requires the fully initialized ordinary Gateway process");
            const aurora::allocation::HostAllocationScope host;
            auto* stage = MR::getStageDataHolder();
            auto* collision = MR::getCollisionDirector();
            auto* resources = SingletonHolder<ResourceHolderManager>::get();
            const auto domain = MR::getSceneObjHolder()->nativeAllocationDomain();
            require(stage && stage->mZoneID == 0 && collision && resources && domain,
                    "Actual process owns the root stage, collision, resource manager and scene heap");

            JKRMemArchive* file_select = nullptr;
            smgpc::test::on_resource_worker([&] {
                file_select = MR::mountArchive("/StageData/FileSelect.arc", nullptr);
            });
            require(file_select != nullptr, "The original FileLoader owns the FileSelect archive");
            auto* placement_data = file_select->getResource('????', "/jmp/placement/common/objinfo");
            require(placement_data != nullptr, "FileSelect retains its original common object table");
            JMapInfo wall_table;
            wall_table.attach(placement_data);
            const auto iter = wall_table.findElement<const char*>("name", "InvisibleWall10x10", 0);
            require(iter.isValid() && NameObjFactory::getCreator("InvisibleWall10x10") != nullptr,
                    "The injected original row has its actual wall creator");
            ResourceHolder* wall_resources = nullptr;
            smgpc::test::on_resource_worker([&] {
                wall_resources = resources->createAndAdd("InvisibleWall10x10.arc", nullptr);
            });
            const auto& archive = wall_resources->nativeResourceSource();
            require(archive.resource_data("InvisibleWall10x10.kcl").size() == 1222U &&
                        archive.resource_data("InvisibleWall10x10.pa").size() == 96U &&
                        archive.resource_data("CollisionVersion").size() == 7U,
                    "Actual resource owner exposes the real RMGK01 wall KCL, attributes and version");
            const auto* kcl_entry = archive.find_resource("InvisibleWall10x10.kcl");
            require(kcl_entry != nullptr, "The exact wall KCL entry is retained in the resource archive");
            const auto expected_source = std::string("/ObjectData/InvisibleWall10x10.arc:/") + kcl_entry->path;
            const auto begin_before = stage->_E4;
            const auto end_before = stage->_E8;
            const auto zone_before = MR::getCurrentPlacementZoneId();
            const auto actors_before = smgpc::compat::actor_runtime_state_count();
            {
                const PlacementTableRangeBinding range(*stage, wall_table);
                require(stage->findPlacedStageDataHolder(iter) == stage && MR::getPlacedZoneId(iter) == 0,
                        "The exact injected row resolves through the original root holder's temporary range");
                struct RestorePlacementState {
                    GameSystemSceneController& controller;
                    SceneInitializeState previous_state;
                    s32 previous_zone = MR::getCurrentPlacementZoneId();
                    ~RestorePlacementState() {
                        controller.setSceneInitializeState(previous_state);
                        MR::setCurrentPlacementZoneId(previous_zone);
                    }
                } placement{*controller, controller->mSceneInitializeState};
                MR::setCurrentPlacementZoneId(0);
                controller->setSceneInitializeState(SceneInitializeState_Placement);
                std::unique_ptr<NameObj> object_owner;
                InvisiblePolygonObj* actor = nullptr;
                {
                    const smgpc::compat::JkrAllocationScope game(domain);
                    const auto creator = NameObjFactory::getCreator("InvisibleWall10x10");
                    require(creator != nullptr, "The retail factory provides the exact wall creator");
                    object_owner.reset(creator("InvisibleWall10x10"));
                    auto* object = object_owner.get();
                    actor = dynamic_cast<InvisiblePolygonObj*>(object);
                    require(actor != nullptr, "The retail wall creator constructs InvisiblePolygonObj");
                    actor->init(iter);
                }
                const auto source = actor->mCollisionParts ? actor->mCollisionParts->nativeResourceSource() : std::string_view{};
                const auto radius = actor->mCollisionParts ? MR::getCollisionBoundingSphereRange(actor) : -1.0F;
                std::fprintf(stderr, "[wall-probe] initialized actor=%p parts=%p registered=%d radius=%g source=%.*s\n",
                             static_cast<void*>(actor), static_cast<void*>(actor->mCollisionParts),
                             !actor->nativeCollisionParts().empty(), static_cast<double>(radius),
                             static_cast<int>(source.size()), source.data());
                require(actor->mCollisionParts && !actor->nativeCollisionParts().empty() &&
                            source == expected_source &&
                            MR::getCollisionBoundingSphereRange(actor) > 0.0F,
                        "Unchanged actor init creates actual CollisionParts and original clipping bounds");
                auto* parts = actor->mCollisionParts;
                auto* original_zone = parts->mZone;
                const auto zone_count = original_zone->mNumParts;
                const OnlyWall only_wall(parts);
                require(line_query_hits_registered_wall(nullptr, &only_wall),
                        "Original CollisionParts registration immediately publishes wall line queries");
                actor->initAfterPlacement();
                require(actor->nativeCollisionParts().size() == 1 &&
                            line_query_hits_registered_wall(&retired_triangle, &only_wall),
                        "Original post-placement callback retains exactly one additional KCL source");
                require(retired_triangle.mParts == parts && retired_triangle.getHostName() == actor->mName &&
                            retired_triangle.getHostPlacementZoneID() == 0 &&
                            actor->mCollisionParts->nativeResourceSource() == expected_source,
                        "Original triangle retains its injected host zone and exact resource provenance");

                actor->makeActorDead();
                require(!line_query_hits_registered_wall(nullptr, &only_wall),
                        "Dead original actors remove their parts from map queries");
                actor->makeActorAppeared();
                require(line_query_hits_registered_wall(nullptr, &only_wall),
                        "Original appearance restores retained collision membership");
                MR::invalidateCollisionParts(actor);
                MR::invalidateCollisionParts(actor);
                require(!line_query_hits_registered_wall(nullptr, &only_wall),
                        "Repeated invalidation remains absent from original queries");
                actor->makeActorDead();
                actor->makeActorAppeared();
                require(actor->mCollisionParts == parts && line_query_hits_registered_wall(nullptr, &only_wall),
                        "Appearance revalidates the same original CollisionParts identity");
                MR::invalidateCollisionParts(actor);
                require(!line_query_hits_registered_wall(nullptr, &only_wall),
                        "Explicit invalidation removes collision after reappearance");
                retired_actor = actor;
                object_owner.reset();
                require(!line_query_hits_registered_wall(nullptr, &only_wall) &&
                            !retired_triangle.isValid() && retired_triangle.getHostName() == nullptr &&
                            original_zone->mNumParts + 1 == zone_count &&
                            !smgpc::compat::has_actor_runtime_state(retired_actor),
                        "Actor retirement removes actual keeper membership, queries and retained Triangle identities");
            }
            require(stage->_E4 == begin_before && stage->_E8 == end_before &&
                        MR::getCurrentPlacementZoneId() == zone_before &&
                        controller->mSceneInitializeState == SceneInitializeState_End &&
                        smgpc::compat::actor_runtime_state_count() == actors_before,
                    "Injected row bounds, placement zone, initialization state and actor ownership restore exactly");
            exercised = true;
            std::fprintf(stderr, "[wall-probe] PASS terminal injected FileSelect row: original creator/init, line queries, appearance/death/invalidation/retirement and restored Gateway owners; frame=%llu\n",
                         static_cast<unsigned long long>(frame));
        }
    };
#endif

    void test_real_file_select_invisible_wall_collision_lifecycle() {
#ifdef NDEBUG
        require(false, "Original-process wall lifecycle diagnostic requires a debug build");
#else
        const auto disc_path = find_real_disc();
        if (!disc_path) {
            std::cout << "[skip] real wall lifecycle test (set SMGPC_REAL_DISC)\n";
            return;
        }
        auto pattern = (std::filesystem::temp_directory_path() / "petari-wall-process-XXXXXX").string();
        const auto* directory = mkdtemp(pattern.data());
        require(directory, "Wall process fixture requires a fresh private native console directory");
        const ScopedEnvironmentVariable save("SMGPC_SAVE_DIR", directory);
        const ScopedEnvironmentVariable nand("SMGPC_NAND_DIR", "");
        const ScopedEnvironmentVariable buttons("SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "");
        const ScopedEnvironmentVariable pointer("SMGPC_DEBUG_WPAD_POINTER_SCRIPT", "");
        const ScopedEnvironmentVariable stick("SMGPC_DEBUG_WPAD_STICK_SCRIPT", "");
        const ScopedEnvironmentVariable input_file("SMGPC_DEBUG_WPAD_INPUT_FILE", "");
        const smgpc::app::BootstrapConfiguration configuration{
            .window_width = 640, .window_height = 456, .window_title = "Original wall lifecycle fixture",
            .arguments = {"original-wall-test", "--stage", "HeavensDoorGalaxy", "--scenario", "1", "--max-frames", "120"},
            .disc_image = *disc_path,
        };
        auto logger = smgpc::logging::create_default_logger();
        smgpc::app::ensure_disc_image_open(configuration, *logger);
        struct DiscLifetime { ~DiscLifetime() { smgpc::app::close_disc_image(); } } disc_lifetime;
        WallProbe probe;
        const smgpc::app::OriginalGameDebugObserver observer{
            .context = &probe,
            .after_frame = +[](void* context, GameSystem& system, std::uint64_t frame) {
                static_cast<WallProbe*>(context)->after_frame(system, frame);
            },
        };
        require(smgpc::app::run_original_game(configuration, *logger, observer) == 0 && probe.exercised,
                "Actual original process completes the terminal wall fixture and bounded frame loop");
        require(!smgpc::compat::has_actor_runtime_state(probe.retired_actor) &&
                    !probe.retired_triangle.isValid() && probe.retired_triangle.getHostName() == nullptr,
                "Normal original-process teardown preserves retired wall identities and releases the scene collision owner");
#endif
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };

}  // namespace

int main(int, char**) {
    constexpr auto tests = std::array{
        TestCase{"injected retail wall lifecycle under original Gateway process", test_real_file_select_invisible_wall_collision_lifecycle},
    };

    auto failures = 0;
    for (const auto &test : tests) {
        try {
            test.run();
            std::cout << "[ok] " << test.name << '\n';
        } catch (const std::exception &error) {
            ++failures;
            std::cerr << "[fail] " << test.name << ": " << error.what() << '\n';
        }
    }
    if (failures != 0) {
        std::cerr << failures << " NameObj placement test(s) failed\n";
        return 1;
    }
    std::cout << tests.size() << " NameObj placement test(s) passed\n";
    return 0;
}
