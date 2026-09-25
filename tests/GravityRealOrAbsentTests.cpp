#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/Scene/StageDataHolder.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "Game/Gravity/GravityInfo.hpp"
#include "OriginalSceneControllerFixture.hpp"
#include "SceneExecutionFixture.hpp"
#include "Game/Gravity/GraviryFollower.hpp"
#include "Game/Gravity/GravityCreator.hpp"
#include "Game/Gravity/GlobalGravityObj.hpp"
#include "Game/Gravity/PlanetGravity.hpp"
#include "Game/Gravity/PlanetGravityManager.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/BaseMatrixFollowTargetHolder.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/JMapLinkInfo.hpp"
#include "resource/BcsvTable.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/nameobj/NameObjFactory.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            aurora::throw_host_exception<std::runtime_error>(std::string(message));
        }
    }

#ifndef NDEBUG
    class ScopedEnvironmentVariable final {
    public:
        ScopedEnvironmentVariable(const char* name, const std::string& value)
            : _name(name) {
            if (const auto* previous = std::getenv(_name.c_str()); previous != nullptr) {
                _previous = previous;
            }
            if (::setenv(_name.c_str(), value.c_str(), 1) != 0) {
                throw std::runtime_error("failed to configure test environment variable: " + _name);
            }
        }

        ~ScopedEnvironmentVariable() {
            if (_previous.has_value()) {
                (void)::setenv(_name.c_str(), _previous->c_str(), 1);
            } else {
                (void)::unsetenv(_name.c_str());
            }
        }

        ScopedEnvironmentVariable(const ScopedEnvironmentVariable&) = delete;
        ScopedEnvironmentVariable& operator=(const ScopedEnvironmentVariable&) = delete;

    private:
        std::string _name;
        std::optional<std::string> _previous;
    };
#endif

    template <typename Exception, typename Function>
    void require_throws(Function&& function, std::string_view message) {
        auto rejected = false;
        try {
            function();
        } catch (const Exception&) {
            rejected = true;
        }
        require(rejected, message);
    }

    void write_be32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 24U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 16U);
        bytes[offset + 2U] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 3U] = static_cast<std::uint8_t>(value);
    }

    void write_be16(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint16_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value);
    }

    void write_be_float(std::vector<std::uint8_t>& bytes, std::size_t offset, float value) {
        write_be32(bytes, offset, std::bit_cast<std::uint32_t>(value));
    }

    void write_field(std::vector<std::uint8_t>& bytes, std::size_t index, std::string_view name,
                     std::uint16_t offset, smgpc::resource::BcsvFieldType type) {
        const auto field_offset = 0x10U + index * 0x0cU;
        write_be32(bytes, field_offset, smgpc::resource::jmap_hash(name));
        write_be32(bytes, field_offset + 4U, 0xffffffffU);
        write_be16(bytes, field_offset + 8U, offset);
        bytes[field_offset + 10U] = 0U;
        bytes[field_offset + 11U] = static_cast<std::uint8_t>(type);
    }

    JMapInfo make_complete_gravity_jmap() {
        constexpr auto field_count = 7U;
        constexpr auto data_offset = 0x10U + field_count * 0x0cU;
        constexpr auto entry_size = 80U;
        auto bytes = std::vector<std::uint8_t>(data_offset + entry_size, 0U);
        write_be32(bytes, 0U, 1U);
        write_be32(bytes, 4U, field_count);
        write_be32(bytes, 8U, data_offset);
        write_be32(bytes, 12U, entry_size);
        write_field(bytes, 0U, "Range", 0U, smgpc::resource::BcsvFieldType::Float);
        write_field(bytes, 1U, "Distant", 4U, smgpc::resource::BcsvFieldType::Float);
        write_field(bytes, 2U, "Priority", 8U, smgpc::resource::BcsvFieldType::Int32);
        write_field(bytes, 3U, "Gravity_id", 12U, smgpc::resource::BcsvFieldType::Int32);
        write_field(bytes, 4U, "Gravity_type", 16U, smgpc::resource::BcsvFieldType::InlineString);
        write_field(bytes, 5U, "Power", 48U, smgpc::resource::BcsvFieldType::InlineString);
        write_field(bytes, 6U, "Inverse", 76U, smgpc::resource::BcsvFieldType::Int32);
        write_be_float(bytes, data_offset, 250.0F);
        write_be_float(bytes, data_offset + 4U, 10.0F);
        write_be32(bytes, data_offset + 8U, 7U);
        write_be32(bytes, data_offset + 12U, 42U);
        const auto gravity_type = std::string_view{"Shadow"};
        const auto power = std::string_view{"Light"};
        std::ranges::copy(gravity_type, bytes.begin() + data_offset + 16U);
        std::ranges::copy(power, bytes.begin() + data_offset + 48U);
        write_be32(bytes, data_offset + 76U, 1U);
        return JMapInfo::from_bcsv(bytes);
    }

    class ConstantGravity final : public PlanetGravity {
    public:
        ConstantGravity(TVec3f direction, float distance) : _direction(direction), _distance(distance) {
        }

        bool calcOwnGravityVector(TVec3f* destination, f32* scalar,
                                  const TVec3f&) const override {
            if (destination != nullptr) {
                destination->set(_direction);
            }
            if (scalar != nullptr) {
                *scalar = _distance;
            }
            return true;
        }

    private:
        TVec3f _direction;
        float _distance;
    };

    class FollowBindingProbe final : public BaseMatrixFollower {
    public:
        FollowBindingProbe(NameObj* owner, const JMapInfoIter& iter)
            : BaseMatrixFollower(owner, iter) {
        }

        void setGravityFollowHost(const NameObj* host) override {
            bound_host = host;
        }

        const NameObj* bound_host = nullptr;
    };

    class GravityScene final {
    public:
        GravityScene()
            : heaps(smgpc::compat::JkrHeapRuntime::create(32U << 20)), original(heaps),
              active(scheduler), domain(smgpc::compat::JkrAllocationDomain::create(heaps, 4U << 20)),
              execution(scheduler, domain,
                        &original.scene, original.controller().mObjHolder),
              manager(static_cast<PlanetGravityManager*>(
                  MR::createSceneObj(SceneObj_PlanetGravityManager))) {
            require(manager != nullptr && MR::createSceneObj(SceneObj_ClippingDirector) != nullptr,
                    "a bound stage scene owns the exact gravity manager and LiveActor clipping director");

        }

        std::shared_ptr<smgpc::compat::JkrHeapRuntime> heaps;
        smgpc::test::OriginalSceneControllerFixture original;
        smgpc::runtime::SceneScheduler scheduler;
        smgpc::runtime::SceneSchedulerBinding active;
        std::shared_ptr<smgpc::compat::JkrAllocationDomain> domain;
        smgpc::test::SceneExecutionFixture execution;
        PlanetGravityManager* manager;
    };

    void test_absent_manager_is_explicit() {
        const auto heaps = smgpc::compat::JkrHeapRuntime::create(16U << 20);
        smgpc::test::OriginalSceneControllerFixture original(heaps);
        smgpc::runtime::SceneScheduler scheduler;
        smgpc::runtime::SceneSchedulerBinding active(scheduler);
        auto actor = LiveActor("gravity-absence-probe");
        auto destination = TVec3f{3.0F, 4.0F, 5.0F};
        require_throws<std::logic_error>(
            [&] { (void)MR::calcGravityVector(&actor, &destination, nullptr, 0U); },
            "gravity queries must reject a missing scene-owned manager");
        require(destination.epsilonEquals(TVec3f{3.0F, 4.0F, 5.0F}, 0.0F),
                "an unavailable gravity query must not fabricate a zero vector");

        auto gravity = ConstantGravity(TVec3f{0.0F, -1.0F, 0.0F}, 100.0F);
        require_throws<std::logic_error>([&] { MR::registerGravity(&gravity); },
                                         "registration must reject a missing scene owner");
        require_throws<std::invalid_argument>([&] { MR::registerGravity(nullptr); },
                                              "null registration must be rejected before scene lookup");
        require_throws<std::invalid_argument>(
            [&] { (void)MR::calcGravityVector(static_cast<const LiveActor*>(nullptr), &destination, nullptr, 0U); },
            "a null actor must not be treated as zero gravity");

        const auto domain = smgpc::compat::JkrAllocationDomain::create(heaps, 1U << 20);
        smgpc::test::SceneExecutionFixture binding(scheduler, domain,
                                                  &original.scene, original.controller().mObjHolder);
        destination.set(3.0F, 4.0F, 5.0F);
        require_throws<std::logic_error>(
            [&] { (void)MR::calcGravityVector(&actor, &destination, nullptr, 0U); },
            "a bound holder must not lazily fabricate a missing gravity manager");
        require(destination.epsilonEquals(TVec3f{3.0F, 4.0F, 5.0F}, 0.0F),
                "a missing manager in an active scene must leave the query destination untouched");
    }

    void test_real_manager_rules_and_info() {
        const auto heaps = smgpc::compat::JkrHeapRuntime::create(16U << 20);
        smgpc::test::OriginalSceneControllerFixture original(heaps);
        smgpc::runtime::SceneScheduler scheduler;
        smgpc::runtime::SceneSchedulerBinding active(scheduler);
        const auto domain = smgpc::compat::JkrAllocationDomain::create(heaps, 1U << 20);
        smgpc::test::SceneExecutionFixture scene(scheduler, domain,
                                                &original.scene, original.controller().mObjHolder);
        require(MR::createSceneObj(SceneObj_PlanetGravityManager) != nullptr,
                "gravity query coverage requires the original scene-owned manager");

        auto caller = NameObj("gravity-caller");
        auto destination = TVec3f{9.0F, 9.0F, 9.0F};
        require(!MR::calcGravityVector(&caller, TVec3f{}, &destination, nullptr, 0U) &&
                    destination.epsilonEquals(TVec3f{}, 0.0F),
                "a real empty manager should retain retail false-and-zero behavior");
        require(!MR::calcGravityVector(nullptr, TVec3f{}, &destination, nullptr, 0U) &&
                    destination.epsilonEquals(TVec3f{}, 0.0F),
                "position-only queries permit no requesting actor and still use the real empty manager");

        auto low = ConstantGravity(TVec3f{1.0F, 0.0F, 0.0F}, 100.0F);
        low.mPriority = 1;
        auto high = ConstantGravity(TVec3f{0.0F, 1.0F, 0.0F}, 100.0F);
        high.mPriority = 5;
        high.mGravityPower = GRAVITY_POWER_HEAVY;
        auto strongest = ConstantGravity(TVec3f{0.0F, 0.0F, 1.0F}, 50.0F);
        strongest.mPriority = 5;
        strongest.mGravityPower = GRAVITY_POWER_LIGHT;
        MR::registerGravity(&low);
        MR::registerGravity(&high);
        MR::registerGravity(&strongest);

        auto info = GravityInfo{};
        require(MR::calcGravityVector(&caller, TVec3f{}, &destination, &info, 0U),
                "registered normal gravities should be queryable through MR");
        require(destination.epsilonEquals(TVec3f{0.0F, 0.24253562F, 0.97014248F}, 0.0001F),
                "only equal highest-priority vectors should combine before normalization");
        auto position_only = TVec3f{};
        require(MR::calcGravityVector(nullptr, TVec3f{}, &position_only, nullptr, 0U) &&
                    position_only.epsilonEquals(destination, 0.0001F),
                "original named-position placement can query registered gravity without an actor");
        require(info.mGravityInstance == &strongest && info.mLargestPriority == 5 &&
                    MR::isLightGravity(info),
                "GravityInfo should identify the strongest winning field and expose its real power");

        strongest.mHost = &caller;
        info.init();
        require(MR::calcGravityVector(&caller, TVec3f{}, &destination, &info, 0U) &&
                    destination.epsilonEquals(TVec3f{0.0F, 1.0F, 0.0F}, 0.0001F) &&
                    info.mGravityInstance == &high && !MR::isLightGravity(info),
                "the requester's own gravity host should be excluded using retail host identity rules");

        if constexpr (sizeof(std::uintptr_t) > sizeof(u32)) {
            const auto caller_id = reinterpret_cast<std::uintptr_t>(&caller);
            const auto other_id = caller_id ^ static_cast<std::uintptr_t>(std::uint64_t{1} << 32U);
            require(MR::calcGravityVector(&caller, TVec3f{}, &destination, &info, caller_id) &&
                        info.mGravityInstance == &high,
                    "an explicit full-width requester must exclude exactly its own gravity host");
            require(MR::calcGravityVector(&caller, TVec3f{}, &destination, &info, other_id) &&
                        info.mGravityInstance == &strongest,
                    "distinct requester identities sharing their low 32 bits must not exclude one another");

            // mHost is an opaque comparison token and is never dereferenced.
            // This checks the implicit caller path independently of explicit IDs.
            strongest.mHost = reinterpret_cast<const void*>(other_id);
            require(MR::calcGravityVector(&caller, TVec3f{}, &destination, &info, 0U) &&
                        info.mGravityInstance == &strongest,
                    "implicit requester identity must preserve the caller's upper address bits");
            require(MR::calcGravityVector(&caller, TVec3f{}, &destination, &info, other_id) &&
                        info.mGravityInstance == &high,
                    "an explicit alternate host must override the requesting actor's identity");
            strongest.mHost = &caller;
        }

        auto shadow = ConstantGravity(TVec3f{-1.0F, 0.0F, 0.0F}, 25.0F);
        shadow.mPriority = 9;
        shadow.mGravityType = GRAVITY_TYPE_SHADOW;
        MR::registerGravity(&shadow);
        require(MR::calcDropShadowVector(&caller, TVec3f{}, &destination, nullptr, 0U) &&
                    destination.epsilonEquals(TVec3f{-1.0F, 0.0F, 0.0F}, 0.0001F),
                "gravity type masks must select real shadow-only fields");

        require_throws<std::logic_error>([&] { MR::registerGravity(&high); },
                                         "duplicate registration must not corrupt manager ordering");
        require_throws<std::invalid_argument>([&] { MR::registerGravity(nullptr); },
                                               "null registration must be explicitly rejected");
    }

    void test_jmap_parameters_are_real() {
        auto gravity = PlanetGravity{};
        const auto jmap = make_complete_gravity_jmap();
        MR::settingGravityParamFromJMap(&gravity, JMapInfoIter(&jmap, 0));
        require(std::abs(gravity.mRange - 250.0F) < 0.0001F &&
                    std::abs(gravity.mDistant - 10.0F) < 0.0001F && gravity.mPriority == 7 &&
                    gravity.mGravityId == 42 && gravity.mGravityType == GRAVITY_TYPE_SHADOW &&
                    gravity.mGravityPower == GRAVITY_POWER_LIGHT && gravity.mIsInverse,
                "all retail gravity JMap parameters must be applied to PlanetGravity state");

        auto info = GravityInfo{};
        info.mGravityInstance = &gravity;
        require(MR::isLightGravity(info),
                "isLightGravity must inspect the selected PlanetGravity instead of returning a constant");
        require_throws<std::invalid_argument>(
            [&] { MR::settingGravityParamFromJMap(nullptr, JMapInfoIter(&jmap, 0)); },
            "JMap setters must reject a missing PlanetGravity instead of silently doing nothing");
    }

    void test_generic_scene_obj_post_placement_binds_followers() {
        auto scene = GravityScene{};
        auto follower_owner = NameObj("follow-binding-probe");
        auto owned_follower = std::make_unique<FollowBindingProbe>(&follower_owner, JMapInfoIter{});
        auto& follower = *owned_follower;
        // Explicit link input for this matrix/registration boundary test. Real
        // authored row-to-zone resolution is exercised in the process probe.
        follower.mLinkInfo->_0 = 42;
        follower.mLinkInfo->_4 = 3;
        follower.mLinkInfo->_8 = 0;
        follower.mFollowID = 7;
        auto target = LiveActor("follow-target");
        auto explicit_host_mtx = TPos3f{};
        explicit_host_mtx.identity();
        explicit_host_mtx.setTrans(16.0F, 27.0F, 41.0F);

        MR::addBaseMatrixFollower(owned_follower.release());
        TPos3f placement;
        placement.identity();
        placement.setTrans(10.0F, 20.0F, 30.0F);
        follower.mFollowTarget->set(&target, placement, &explicit_host_mtx, nullptr);
        require(follower.bound_host == nullptr,
                "followers must remain unbound until the scene post-placement phase");
        scene.execution.init_after_placement();
        require(follower.bound_host == &target && follower.getFollowTargetActor() == &target,
                "the generic SceneObjHolder pass must run the exact BaseMatrix follower binding");

        auto follow_mtx = TPos3f{};
        follower.calcFollowMatrix(&follow_mtx);
        auto follow_translation = TVec3f{};
        follow_mtx.getTrans(follow_translation);
        require(follow_translation.epsilonEquals(TVec3f{6.0F, 7.0F, 11.0F}, 0.0001F),
                "the exact follower must compute host * inverse(placement) with in-place matrix inversion");
        require(target.getBaseMtx() == nullptr,
                "a model-less LiveActor must retain the retail absent base-matrix result");
        // The actual holder owns the follower/target; retirement removes their
        // borrowed actor references. No scheduler step follows these objects.
        // Actual ModelManager/base-matrix ownership is covered by the
        // original-process player, NPC and map-object probes. This pure
        // follower fixture owns no model archive.
    }

#ifndef NDEBUG
    struct GravityProcessProbe {
        static constexpr std::uint64_t terminal_frame = 119;
        bool exercised = false;
        std::vector<const NameObj*> identities;

        void remember(const NameObj* object) {
            const aurora::allocation::HostAllocationScope host;
            identities.push_back(object);
        }

        void run(GameSystem& system, std::uint64_t frame) {
            if (frame != terminal_frame) return;
            auto* controller = system.mSceneController;
            require(controller && controller->mSceneInitializeState == SceneInitializeState_End &&
                        dynamic_cast<GameScene*>(controller->mScene) && system.mObjHolder,
                    "terminal gravity probe requires completed original GameScene and process owners");
            auto* stage = MR::getStageDataHolder();
            auto* manager = static_cast<PlanetGravityManager*>(MR::getSceneObjHolder()->getObj(SceneObj_PlanetGravityManager));
            auto* followers = static_cast<BaseMatrixFollowTargetHolder*>(MR::createSceneObj(SceneObj_BaseMatrixFollowTargetHolder));
            require(stage && manager && followers && MR::isExistSceneObj(SceneObj_DemoDirector),
                    "real stage, gravity, follower and DemoDirector owners are present");
            JMapInfoIter point_row, rail_row, target_row;
            for (s32 zone = 0; zone < MR::getZoneNum(); ++zone) {
                const auto* owner = stage->getStageDataHolderFromZoneId(zone);
                if (!owner) continue;
                for (const auto& table : owner->mPlacementObjs) {
                    for (s32 row = 0; row < table.getNumEntries(); ++row) {
                        JMapInfoIter iter(&table, row);
                        const char* name = nullptr;
                        s32 a = -1, b = -1, sleep = -1, path = -1, arg = -1;
                        (void)iter.getValue("SW_A", &a); (void)iter.getValue("SW_B", &b);
                        (void)iter.getValue("SW_SLEEP", &sleep);
                        if (a >= 0 || b >= 0 || sleep >= 0 || MR::isValidFollowID(iter)) continue;
                        if (!target_row.isValid()) {
                            JMapLinkInfo link(iter, true);
                            if (link.isValid() && !followers->findFollowTarget(&link)) target_row = iter;
                        }
                        if (MR::getObjectName(&name, iter) && !point_row.isValid() &&
                            std::strcmp(name, "GlobalPointGravity") == 0) point_row = iter;
                        if (!rail_row.isValid() && iter.getValue("CommonPath_ID", &path) && path >= 0 &&
                            iter.getValue("Obj_arg0", &arg) && arg >= 0 && arg <= 128) rail_row = iter;
                    }
                }
            }
            require(point_row.isValid() && rail_row.isValid() && target_row.isValid() &&
                        stage->findPlacedStageDataHolder(point_row) && stage->findPlacedStageDataHolder(rail_row),
                    "controlled creator variants consume real retained SRT and rail rows with original zone provenance");
            {
                const aurora::allocation::HostAllocationScope host;
                for (auto* object : smgpc::compat::snapshot_name_obj_runtime_objects()) {
                    auto* actor = dynamic_cast<GlobalGravityObj*>(object);
                    if (!actor || !actor->mGravityCreator) continue;
                    auto* field = actor->getGravity();
                    require(field && !(field->mGravityType & GRAVITY_TYPE_MAGNET),
                            "actual stage has initialized fields and no magnet field competing with the isolated query");
                }
            }
            constexpr std::array creators{
                "GlobalCubeGravity", "GlobalConeGravity", "GlobalDiskGravity", "GlobalDiskTorusGravity",
                "GlobalPlaneGravity", "GlobalPlaneGravityInBox", "GlobalPlaneGravityInCylinder",
                "GlobalPointGravity", "GlobalSegmentGravity", "GlobalWireGravity",
            };
            smgpc::runtime::DvdFileSystemService dvd("/");
            require(NameObjFactory::getCreator("GlobalPointGravity") == MR::createGlobalPointGravityObj &&
                        NameObjFactory::getCreator("GlobalCubeGravity") == MR::createGlobalCubeGravityObj &&
                        NameObjFactory::getCreator("GlobalConeGravity") == MR::createGlobalConeGravityObj &&
                        NameObjFactory::getCreator("GlobalDiskGravity") == MR::createGlobalDiskGravityObj &&
                        NameObjFactory::getCreator("GlobalDiskTorusGravity") == MR::createGlobalDiskTorusGravityObj &&
                        NameObjFactory::getCreator("GlobalPlaneGravity") == MR::createGlobalPlaneGravityObj &&
                        NameObjFactory::getCreator("GlobalPlaneGravityInBox") == MR::createGlobalPlaneInBoxGravityObj &&
                        NameObjFactory::getCreator("GlobalPlaneGravityInCylinder") == MR::createGlobalPlaneInCylinderGravityObj &&
                        NameObjFactory::getCreator("GlobalSegmentGravity") == MR::createGlobalSegmentGravityObj &&
                        NameObjFactory::getCreator("GlobalWireGravity") == MR::createGlobalWireGravityObj,
                    "the host factory must expose the exact retail gravity actor creators");
            for (const char* name : creators) {
                auto object = smgpc::scene::nameobj::create_name_obj(dvd, name, name);
                auto* actor = dynamic_cast<GlobalGravityObj*>(object.get());
                require(actor && actor->mGravityCreator, "ordinary factory retains each exact gravity wrapper/creator");
                remember(actor);
                const auto& iter = std::strcmp(name, "GlobalWireGravity") == 0 ? rail_row : point_row;
                actor->init(iter);
                require(actor->getGravity() && actor->getGravity()->mIsRegistered,
                        "exact original init registers each real field");
                if (auto* wire = dynamic_cast<WireGravityCreator*>(actor->mGravityCreator)) {
                    s32 samples = 20;
                    MR::getJMapInfoArg0NoInit(iter, &samples);
                    require(wire->mRailRider && wire->mRailRider->mBezierRail &&
                                wire->mRailRider->mBezierRail->mNumRailParts > 0 &&
                                wire->mGravityInstance->mPoints.size() == samples + 1,
                            "Wire creator retains its actual rail and original authored sampling count");
                }
            }
            auto followed = smgpc::scene::nameobj::create_name_obj(dvd, "GlobalPointGravity", "followed-gravity-probe");
            auto* actor = static_cast<GlobalGravityObj*>(followed.get());
            remember(actor);
            actor->init(point_row);
            auto* field = dynamic_cast<PointGravity*>(actor->getGravity());
            auto* follower = new GraviryFollower(actor, JMapInfoIter{});
            *follower->mLinkInfo = JMapLinkInfo(target_row, true);
            follower->mFollowID = 7;
            MR::addBaseMatrixFollower(follower);
            require(field && follower->mFollowTarget && follower->mFollowTarget->mLinkInfo->isValid() &&
                        follower->mGravity == field && follower->mFollowID == 7,
                    "the actual holder retains the original follower and its authored target identity");
            // Only this new fixture field changes context; authored fields and
            // their priority/order remain untouched. No game frame follows.
            field->mGravityType = GRAVITY_TYPE_MAGNET;
            const auto query = field->mTranslation + TVec3f(0, 50, 0);
            TVec3f gravity;
            GravityInfo info;
            require(field->mIsRegistered && manager->calcTotalGravityVector(&gravity, &info, query, GRAVITY_TYPE_MAGNET, 0) &&
                        info.mGravityInstance == field && gravity.epsilonEquals(TVec3f(0, -1, 0), 0.0001F),
                    "actual manager queries the registered field while its original actor owns it");
            followed.reset();
            exercised = true;
            std::fprintf(stderr, "[gravity-process] PASS terminal original creator/rail/follower/query checks frame=%llu\n",
                         static_cast<unsigned long long>(frame));
        }

        void verify_retirement() {
            require(exercised && !MR::getSceneObjHolder(),
                    "actual process retires its original scene and gravity owners");
            for (const auto* identity : identities)
                require(!smgpc::compat::has_name_obj_runtime_state(identity), "all temporary original actor identities retire");
        }
    };

    void test_actual_process_gravity() {
        const auto* disc = std::getenv("SMGPC_REAL_DISC");
        if (!disc || !*disc) {
            std::cout << "[skip] original-process gravity requires SMGPC_REAL_DISC; no actor lifecycle claimed\n";
            return;
        }
        const auto save = std::filesystem::temp_directory_path() / ("petari-gravity-process-" + std::to_string(getpid()));
        require(!std::filesystem::exists(save), "actual gravity probe requires fresh console settings");
        ScopedEnvironmentVariable save_dir("SMGPC_SAVE_DIR", save.string());
        for (const char* name : {"SMGPC_NAND_DIR", "SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "SMGPC_DEBUG_WPAD_POINTER_SCRIPT",
                                "SMGPC_DEBUG_WPAD_STICK_SCRIPT", "SMGPC_DEBUG_WPAD_INPUT_FILE", "SMGPC_STRICT_PLACEMENT"}) unsetenv(name);
        const smgpc::app::BootstrapConfiguration configuration{
            .window_width = 640, .window_height = 456, .window_title = "Original gravity ownership",
            .arguments = {"gravity-owner-test", "--stage", "HeavensDoorGalaxy", "--scenario", "1", "--max-frames", "120"},
            .disc_image = disc,
        };
        auto logger = smgpc::logging::create_default_logger();
        smgpc::app::ensure_disc_image_open(configuration, *logger);
        struct DiscLifetime { ~DiscLifetime() { smgpc::app::close_disc_image(); } } close;
        GravityProcessProbe probe;
        const smgpc::app::OriginalGameDebugObserver observer{
            .context = &probe,
            .after_frame = +[](void* context, GameSystem& system, std::uint64_t frame) {
                static_cast<GravityProcessProbe*>(context)->run(system, frame);
            },
        };
        require(smgpc::app::run_original_game(configuration, *logger, observer) == 0,
                "actual original process completes its bounded gravity probe");
        probe.verify_retirement();
        std::cout << "[ok] actual-process gravity: original creators, original field/rail/follower behavior, terminal queries and retirement\n";
    }
#endif

    struct TestCase {
        std::string_view name;
        void (*run)();
    };
}  // namespace

int main(int argc, char** argv) {
    bool queries_only = false, process_only = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view option(argv[i]);
        if (option == "--queries-only") queries_only = true;
        else if (option == "--process-only") process_only = true;
        else if ((option == "-ApplePersistenceIgnoreState" || option == "-NSQuitAlwaysKeepsWindows") && i + 1 < argc) ++i;
        else { std::cerr << "Unknown gravity test option: " << option << '\n'; return 2; }
    }
    if (queries_only && process_only) return 2;
    const auto tests = std::array{
        TestCase{"absent manager is explicit", test_absent_manager_is_explicit},
        TestCase{"real manager rules and info", test_real_manager_rules_and_info},
        TestCase{"JMap parameters are real", test_jmap_parameters_are_real},
        TestCase{"generic scene object post-placement follower binding", test_generic_scene_obj_post_placement_binds_followers},
    };

    auto failures = 0;
    auto executed = 0;
    for (const auto& test : tests) {
        if (process_only) continue;
        if (queries_only && test.run != test_absent_manager_is_explicit && test.run != test_real_manager_rules_and_info) {
            continue;
        }
        ++executed;
        try {
            test.run();
            std::cout << "[ok] " << test.name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[fail] " << test.name << ": " << error.what() << '\n';
        }
    }
#ifndef NDEBUG
    if (!queries_only) {
        try { test_actual_process_gravity(); }
        catch (const std::exception& error) { ++failures; std::cerr << "[fail] original-process gravity: " << error.what() << '\n'; }
    }
#endif
    if (failures != 0) {
        std::cerr << failures << " gravity real-or-absent test(s) failed\n";
        return 1;
    }
    std::cout << executed << " gravity real-or-absent test(s) passed\n";
    return 0;
}
