#include "OriginalSphereQueryFixture.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Map/CollisionParts.hpp"
#include "Game/MapObj/DynamicCollisionObj.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "compat/HitInfoCompat.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "resource/KCollisionResource.hpp"
#include "scene/StageCollisionService.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
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

    template <typename Operation>
    void require_unavailable(Operation&& operation, std::string_view message) {
        auto unavailable = false;
        try {
            operation();
        } catch (const std::logic_error&) {
            unavailable = true;
        }
        require(unavailable, message);
    }

    void write_be16(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint16_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value);
    }

    void write_be32(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint32_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 24U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 16U);
        bytes[offset + 2U] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 3U] = static_cast<std::uint8_t>(value);
    }

    void write_be_float(std::vector<std::uint8_t> &bytes, std::size_t offset, float value) {
        write_be32(bytes, offset, std::bit_cast<std::uint32_t>(value));
    }

    [[nodiscard]] std::vector<std::uint8_t> make_single_triangle_kcl() {
        constexpr auto position_offset = 0x38U;
        constexpr auto normal_offset = 0x44U;
        constexpr auto prism_offset = 0x74U;
        constexpr auto octree_offset = 0x84U;
        auto bytes = std::vector<std::uint8_t>(0x88U, 0U);
        write_be32(bytes, 0x00U, position_offset);
        write_be32(bytes, 0x04U, normal_offset);
        write_be32(bytes, 0x08U, prism_offset - 0x10U);
        write_be32(bytes, 0x0cU, octree_offset);
        write_be_float(bytes, 0x10U, 2.0F);

        const auto write_vec3 = [&](std::size_t offset, float x, float y, float z) {
            write_be_float(bytes, offset, x);
            write_be_float(bytes, offset + 4U, y);
            write_be_float(bytes, offset + 8U, z);
        };
        write_vec3(position_offset, 0.0F, 0.0F, 0.0F);
        write_vec3(normal_offset + 0x00U, 0.0F, 1.0F, 0.0F);
        write_vec3(normal_offset + 0x0cU, 1.0F, 0.0F, 0.0F);
        write_vec3(normal_offset + 0x18U, 0.0F, 0.0F, 1.0F);
        constexpr auto diagonal = 0.70710678118F;
        write_vec3(normal_offset + 0x24U, diagonal, 0.0F, diagonal);

        write_be_float(bytes, prism_offset, diagonal);
        write_be16(bytes, prism_offset + 4U, 0U);
        write_be16(bytes, prism_offset + 6U, 0U);
        write_be16(bytes, prism_offset + 8U, 1U);
        write_be16(bytes, prism_offset + 10U, 2U);
        write_be16(bytes, prism_offset + 12U, 3U);
        write_be16(bytes, prism_offset + 14U, 7U);
        return bytes;
    }

    constexpr auto cIdentity = std::array<float, 12U>{
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
    };

    void test_collision_is_absent_without_explicit_registration() {
        auto collision = smgpc::scene::StageCollisionService{};
        collision.build();
        collision.activate();

        require(collision.empty(), "a new stage collision registry must remain empty");
        require(collision.stats().mesh_count == 0U && collision.stats().triangle_count == 0U &&
                    collision.stats().rejected_triangle_count == 0U,
                "absence must not be represented as a synthesized or rejected collision mesh");
        require(!collision.line_cast(TVec3f{0.0F, 1.0F, 0.0F}, TVec3f{0.0F, -2.0F, 0.0F}),
                "line queries must miss when CollisionParts has registered nothing");
        require(collision.sphere_contacts(TVec3f{}, 50.0F).empty(),
                "sphere queries must have no contacts when collision is absent");
        smgpc::scene::StageCollisionHit unchanged;
        unchanged.position.set(7.0F, 8.0F, 9.0F);
        unchanged.normal.set(4.0F, 5.0F, 6.0F);
        require(!collision.line_cast(TVec3f(0, 1, 0), TVec3f(0, -2, 0), &unchanged) &&
                    unchanged.position.epsilonEquals(TVec3f(7, 8, 9), 0.0F) &&
                    unchanged.normal.epsilonEquals(TVec3f(4, 5, 6), 0.0F),
                "an empty host registry must not fabricate hit output values");
    }

    void test_only_explicit_valid_kcl_registration_adds_collision() {
        auto collision = smgpc::scene::StageCollisionService{};
        const auto malformed = std::array<std::uint8_t, 4U>{};
        require(!collision.add_kcl(malformed, cIdentity, "/ObjectData/Missing.arc:/Missing.kcl") &&
                    collision.empty(),
                "a missing or malformed exact CollisionParts resource must remain absent");

        const auto kcl = make_single_triangle_kcl();
        require(collision.add_kcl(kcl, cIdentity, "/ObjectData/Exact.arc:/Exact/Exact.kcl"),
                "an explicit valid CollisionParts KCL registration should be accepted");
        require(!collision.line_cast(TVec3f{0.25F, 1.0F, 0.25F}, TVec3f{0.0F, -2.0F, 0.0F}),
                "an explicit registration should not be queryable until its owner completes the build");

        collision.build();
        auto hit = smgpc::scene::StageCollisionHit{};
        require(collision.stats().mesh_count == 1U && collision.stats().triangle_count == 1U &&
                    collision.line_cast(TVec3f{0.25F, 1.0F, 0.25F}, TVec3f{0.0F, -2.0F, 0.0F}, &hit) &&
                    hit.attribute == 7U,
                "only the explicitly registered exact KCL should become stage collision");
    }

    void test_triangle_source_matrix_lifetime() {
        auto collision = smgpc::scene::StageCollisionService{};
        auto registration = std::make_shared<smgpc::scene::StageCollisionRegistrationState>();
        const auto kcl = make_single_triangle_kcl();
        const auto matrix = std::array<float, 12>{0, 0, 2, 10, 0, 3, 0, 20, -4, 0, 0, 30};
        require(collision.register_kcl(kcl, matrix, "transformed source", registration).accepted,
                "register exact rotated and scaled source");
        collision.build();
        collision.activate();
        smgpc::scene::StageCollisionHit hit;
        require(collision.line_cast(TVec3f(10.5F, 25.0F, 29.0F), TVec3f(0.0F, -10.0F, 0.0F), &hit),
                "source geometry must actually produce the transformed hit");
        const auto triangle = smgpc::compat::make_collision_triangle(collision, hit.triangle_index);
        const auto copied = triangle;
        auto* base = triangle.getBaseMtx();
        auto* inverse = triangle.getBaseInvMtx();
        auto* previous = triangle.getPrevBaseMtx();
        TVec3f local;
        PSMTXMultVec(*inverse, &hit.position, &local);
        require(local.epsilonEquals(TVec3f(0.25F, 0.0F, 0.25F), 0.00001F),
                "original ground recording must use the full inverse including scale");
        TVec3f restored;
        PSMTXMultVec(*base, &local, &restored);
        require(restored.epsilonEquals(hit.position, 0.00001F), "recorded local point must return to its actual world hit");
        PSMTXMultVec(*previous, &local, &restored);
        require(restored.epsilonEquals(hit.position, 0.00001F), "static source retains its original previous matrix");
        for (unsigned i = 0; i < 64; ++i) {
            require(collision.add_kcl(kcl, cIdentity, "additional source"), "append source to grow registry");
        }
        collision.build();
        require(copied.getBaseMtx() == base && copied.getBaseInvMtx() == inverse && copied.getPrevBaseMtx() == previous,
                "copied triangle transforms must remain stable across source registry relocation");
        registration->set_enabled(false);
        require_unavailable([&] { (void)copied.getBaseInvMtx(); }, "disabled source must not expose retired ground transforms");
        registration->set_enabled(true);
        require(copied.getBaseMtx() == base, "re-enabled same source must preserve matrix identity");
        registration->release_owner();
        require_unavailable([&] { (void)copied.getBaseMtx(); }, "released source must not expose ground transforms");
        collision.clear();
        require_unavailable([&] { (void)copied.getPrevBaseMtx(); }, "cleared source cannot supply a fabricated identity matrix");
    }

    void test_repeated_line_queries_preserve_game_heap() {
        auto collision = smgpc::scene::StageCollisionService{};
        require(collision.add_kcl(make_single_triangle_kcl(), cIdentity, "line allocation ownership"),
                "line allocation fixture must register actual source geometry");
        collision.build();
        collision.activate();
        auto runtime = smgpc::compat::JkrHeapRuntime::create(1U << 20U);
        auto domain = smgpc::compat::JkrAllocationDomain::create(runtime, 64U << 10U);
        const auto retired = std::weak_ptr(domain);
        const auto start = TVec3f(0.25F, 1.0F, 0.25F);
        const auto offset = TVec3f(0.0F, -2.0F, 0.0F);
        {
            const smgpc::compat::JkrAllocationScope game(domain);
            const auto free_before = domain->heap().getFreeSize();
            for (unsigned query = 0; query < 2048U; ++query) {
                require(collision.line_cast(start, offset),
                        "repeated host line queries must retain actual hits");
                require(!collision.line_cast(TVec3f(2.0F, 1.0F, 2.0F), offset),
                        "repeated host line queries must retain actual misses");
                require(domain->heap().getFreeSize() == free_before,
                        "native line traversal storage must not consume the selected Game heap");
            }
            auto* original = new int(9);
            require(JKRHeap::findFromRoot(original) == &domain->heap(),
                    "line queries must restore original allocation routing");
            delete original;
        }
        domain.reset();
        require(retired.expired(), "native line queries must not retain the original heap");
        runtime.reset();
        require(collision.line_cast(start, offset),
                "native collision geometry must remain valid after unrelated Game heap retirement");
    }

    void test_generated_geometry_publication_and_retirement() {
        smgpc::test::OriginalSphereQueryFixture original;
        require(MR::createSceneObj(SceneObj_ClippingDirector) != nullptr,
                "the generated writer uses real original actor-registration owners");
        // Run the original geometry writer against separately allocated arrays.
        // This fixture exercises publication below CollisionParts::init; it does
        // not claim full AreaPolygon/Mario/process initialization coverage.
        struct Geometry {
            KCLFile file{};
            std::vector<TVec3f> positions = std::vector<TVec3f>(2);
            std::vector<TVec3f> normals = std::vector<TVec3f>(8);
            std::vector<KC_PrismData> prisms = std::vector<KC_PrismData>(3);
            std::array<u16, 5> octree{0x8000, 2, 2, 1, 0};
            Geometry() {
                file.mPos = positions.data();
                file.mNorms = normals.data();
                file.mPrisms = prisms.data();
                file.mOctree = octree.data();
                file.mThickness = 40;
                file.mBlockXShift = file.mBlockXYShift = -1;
            }
        };
        const smgpc::compat::JkrAllocationScope game(original.domain);
        DynamicCollisionObj actor("generated geometry publication fixture");
        // StageCollisionService retains this field solely as opaque identity;
        // no method dereferences it. This is deliberately not a constructed
        // HitSensor and proves neither actor nor sensor initialization.
        alignas(HitSensor) std::array<std::byte, sizeof(HitSensor)> sensor_identity{};
        auto* sensor = reinterpret_cast<HitSensor*>(sensor_identity.data());
        std::array<TVec3f, 4> vertices{TVec3f(0, 0, 0), TVec3f(4, 0, 0), TVec3f(4, 4, 0), TVec3f(0, 4, 0)};
        std::array<TVec3f, 2> face_normals;
        std::array<DynamicCollisionObj::TriangleIndexing, 2> indices;
        const std::array<std::array<u16, 3>, 2> values{{{0, 1, 2}, {0, 2, 3}}};
        for (std::size_t i = 0; i < indices.size(); ++i)
            std::copy(values[i].begin(), values[i].end(), indices[i].mIndex);
        auto allocation = std::make_shared<Geometry>();
        const auto retired = std::weak_ptr(allocation);
        const auto* identity = &allocation->file;
        actor.mKCLFile = &allocation->file;
        actor.mPositions = vertices.data();
        actor.mPositionNum = vertices.size();
        actor._94 = indices.size();
        actor._9C = face_normals.data();
        actor.mIndices = indices.data();
        actor.updateCollisionHeader();
        actor.updateTriangle();
        auto resource = std::make_shared<smgpc::resource::GeneratedKCollisionResource>(
            allocation->file, allocation->positions, allocation->normals, allocation->prisms,
            allocation->octree, allocation);
        CollisionParts parts;
        std::unique_ptr<KCollisionServer> server_owner(parts.mServer);
        std::unique_ptr<JMapInfo> map_owner(parts.mServer->mapInfo);
        parts.mServer->init(resource->native_file(), nullptr);
        parts.mHitSensor = sensor;
        actor.mParts = &parts;
        actor.syncCollision();
        auto registration = std::make_shared<smgpc::scene::StageCollisionRegistrationState>(nullptr, &parts);
        smgpc::scene::StageCollisionService collision;
        require(collision.register_generated_kcl(resource, *parts.mServer, cIdentity,
                    "original generated square", registration, sensor, 0).accepted,
                "the original writer's separate arrays must register as generated geometry");
        collision.build();
        const auto first = collision.surface(&parts, 0);
        const auto second = collision.surface(&parts, 1);
        require(first && second && first->sensor == sensor && first->parts == &parts,
                "generated surfaces retain their actual original part and opaque sensor identity");
        const auto first_id = first->triangle_index;
        const auto second_id = second->triangle_index;
        const auto check_hit = [&](float z, float fraction) {
            const auto start = TVec3f(3, 1, 10);
            const auto offset = TVec3f(0, 0, -20);
            const auto hits = collision.line_hits(start, offset);
            require(!hits.empty() && hits.front().triangle_index == first_id &&
                        std::abs(hits.front().position.z - z) < 0.001F,
                    "original authored octree traversal must reach the live generated face");
            smgpc::scene::StageCollisionHit hit;
            require(collision.line_cast(start, offset, &hit) && hit.triangle_index == first_id &&
                        std::abs(hit.fraction - fraction) < 0.001F,
                    "refitted native broad phase must agree with original generated traversal");
        };
        check_hit(0, 0.5F);
        for (auto& vertex : vertices) vertex.z = 3;
        actor.syncCollision();
        collision.update_registered_geometry(*registration);
        check_hit(3, 0.35F);
        require(collision.surface(&parts, 0)->triangle_index == first_id &&
                    collision.surface(&parts, 1)->triangle_index == second_id,
                "geometry refresh must preserve global surface and original local prism identities");
        registration->set_enabled(false);
        require(!collision.surface(first_id) && collision.line_hits(TVec3f(3, 1, 10), TVec3f(0, 0, -20)).empty(),
                "disabled generated membership must leave all query paths");
        registration->set_enabled(true);
        check_hit(3, 0.35F);
        // Original KCollision represents a collapsed/near-parallel face by a
        // nonpositive height. Its stable slot must survive deactivation.
        const auto saved = vertices;
        for (auto& vertex : vertices) vertex.z = 5;
        vertices[3] = vertices[0];
        actor.syncCollision();
        collision.update_registered_geometry(*registration);
        check_hit(5, 0.25F);
        require(!collision.surface(second_id) && !collision.surface(&parts, 1) &&
                    collision.stats().triangle_count == 2,
                "collapsed original prism must retain its identity slot without exposing stale geometry");
        require(collision.line_hits(TVec3f(1, 3, 10), TVec3f(0, 0, -20)).empty() &&
                    !collision.line_cast(TVec3f(1, 3, 10), TVec3f(0, 0, -20)),
                "original and native queries must skip the collapsed face");
        vertices = saved;
        actor.syncCollision();
        collision.update_registered_geometry(*registration);
        require(collision.surface(&parts, 1)->triangle_index == second_id,
                "restored original geometry must reactivate its prior stable prism identity");
        check_hit(3, 0.35F);
        for (std::size_t i = 1; i < allocation->prisms.size(); ++i)
            allocation->prisms[i].mHeight = -std::abs(allocation->prisms[i].mHeight);
        collision.update_registered_geometry(*registration);
        require(!collision.surface(first_id) && !collision.surface(second_id) &&
                    collision.line_hits(TVec3f(3, 1, 10), TVec3f(0, 0, -20)).empty() &&
                    !collision.line_cast(TVec3f(3, 1, 10), TVec3f(0, 0, -20)) &&
                    collision.sphere_contacts(TVec3f(3, 1, 3), 1).empty(),
                "all-inactive generated resources must build an empty query index while retaining prism slots");
        auto shifted = cIdentity;
        shifted[3] = 20;
        collision.update_registered_transform(*registration, shifted, cIdentity);
        actor.syncCollision();
        collision.update_registered_geometry(*registration);
        require(collision.surface(&parts, 0)->triangle_index == first_id &&
                    collision.surface(&parts, 0)->vertices[0].x == 20 &&
                    collision.line_cast(TVec3f(23, 1, 10), TVec3f(0, 0, -20)) &&
                    !collision.line_hits(TVec3f(23, 1, 10), TVec3f(0, 0, -20)).empty(),
                "reactivated geometry must use a transform committed while all its prisms were inactive");
        collision.update_registered_transform(*registration, cIdentity, shifted);
        check_hit(3, 0.35F);
        // Malformed references remain errors. No partial cache mutation may
        // escape before the full retained resource passes validation.
        allocation->prisms[2].mNormalIndex = 99;
        bool rejected = false;
        try { collision.update_registered_geometry(*registration); }
        catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, "invalid generated references must fail before publication");
        require(collision.surface(first_id)->vertices[0].z == 3,
                "failed geometry publication must leave the previous cached surface intact");
        const std::array<TVec3f, 2> area{TVec3f(0, 0, 2), TVec3f(4, 4, 4)};
        require_unavailable([&] { collision.line_hits(TVec3f(3, 1, 10), TVec3f(0, 0, -20)); },
                            "rejected mutations must quarantine original line traversal of the live arrays");
        require_unavailable([&] { collision.area_polygons(area, 512); },
                            "rejected mutations must quarantine original area traversal of the live arrays");
        require_unavailable([&] { collision.line_cast(TVec3f(3, 1, 10), TVec3f(0, 0, -20)); },
                            "quarantined geometry cannot silently succeed using the previous native ray cache");
        require_unavailable([&] { collision.sphere_contacts(TVec3f(3, 1, 3), 1); },
                            "quarantined geometry cannot silently succeed using the previous native sphere cache");
        registration->set_enabled(false);
        require(collision.line_hits(TVec3f(3, 1, 10), TVec3f(0, 0, -20)).empty() &&
                    collision.area_polygons(area, 512).empty(),
                "disabled quarantined geometry is absent without reading its rejected arrays");
        registration->set_enabled(true);
        require_unavailable([&] { collision.line_hits(TVec3f(3, 1, 10), TVec3f(0, 0, -20)); },
                            "membership reactivation cannot clear failed-publication quarantine");
        actor.syncCollision();
        collision.update_registered_geometry(*registration);
        check_hit(3, 0.35F);
        require(!collision.area_polygons(area, 512).empty(),
                "corrected successful publication restores actual original area traversal");
        resource.reset();
        allocation.reset();
        require(!retired.expired(), "registered generated geometry retains the actual independent allocations");
        registration->release_owner();
        require(!collision.surface(first_id) && !collision.surface(&parts, 0),
                "retirement makes retained generated surfaces inert before original part destruction");
        require_unavailable([&] { collision.update_registered_geometry(*registration); },
                            "retired generated owner cannot mutate the scene cache");
        map_owner.reset();
        server_owner.reset();
        require(collision.line_hits(TVec3f(3, 1, 10), TVec3f(0, 0, -20)).empty() &&
                    collision.sphere_contacts(TVec3f(3, 1, 3), 1).empty(),
                "retired registrations must not dereference a destroyed original server");
        collision.clear();
        require(retired.expired() && !smgpc::resource::is_native_kcollision_file(identity),
                "final service retirement releases generated arrays and typed file identity");
    }

    std::array<float, 12> floor_matrix(float height = 0) {
        return {100, 0, 0, 0, 0, 1, 0, height, 0, 0, 100, 0};
    }

    struct CollisionOwner {
        explicit CollisionOwner(const char* name)
            : actor(name), sensor(0U, 0U, 1.0F, &actor),
              registration(std::make_shared<smgpc::scene::StageCollisionRegistrationState>(&actor.mFlag.mIsDead)) {
            actor.makeActorAppeared();
        }

        ~CollisionOwner() {
            registration->release_owner();
        }

        bool add(smgpc::scene::StageCollisionService& collision,
                 std::span<const std::uint8_t> kcl, const std::array<float, 12U>& matrix,
                 std::int32_t zone_id = 0) {
            return collision.register_kcl(kcl, matrix, "/ObjectData/Shared.arc:/Shared.kcl",
                                          registration, {}, &sensor, zone_id).accepted;
        }

        LiveActor actor;
        HitSensor sensor;
        std::shared_ptr<smgpc::scene::StageCollisionRegistrationState> registration;
    };

    void test_owner_name_and_zone_remain_separate_from_resources() {
        smgpc::test::OriginalSphereQueryFixture original;
        require(MR::createSceneObj(SceneObj_ClippingDirector) != nullptr,
                "host metadata test uses the original actor-registration owner");
        auto collision = smgpc::scene::StageCollisionService{};
        const auto kcl = make_single_triangle_kcl();
        auto lower = CollisionOwner("LowerActor");
        auto upper = std::make_unique<CollisionOwner>("UpperActor");
        require(lower.add(collision, kcl, floor_matrix(), 5) &&
                    upper->add(collision, kcl, floor_matrix(2.0F), 9),
                "two actors in different zones must be able to share the exact same KCL resource");
        collision.build();
        collision.activate();
        const auto read_triangle = [&collision](const TVec3f& start) {
            smgpc::scene::StageCollisionHit hit;
            require(collision.line_cast(start, TVec3f(0.0F, -1.5F, 0.0F), &hit),
                    "the explicit host registry must return its live floor surface");
            auto triangle = smgpc::compat::make_collision_triangle(collision, hit.triangle_index);
            return triangle;
        };
        auto lower_triangle = read_triangle(TVec3f(10.0F, 1.0F, 10.0F));
        auto upper_triangle = read_triangle(TVec3f(10.0F, 3.0F, 10.0F));
        require(std::string_view(lower_triangle.getHostName()) == "LowerActor" &&
                    std::string_view(upper_triangle.getHostName()) == "UpperActor" &&
                    lower_triangle.getHostPlacementZoneID() == 5 && upper_triangle.getHostPlacementZoneID() == 9,
                "Triangle must retain each actual sensor host and its creation zone");
        const auto lower_surface = collision.surface(lower_triangle.mIdx);
        const auto upper_surface = collision.surface(upper_triangle.mIdx);
        require(lower_surface.has_value() && upper_surface.has_value() &&
                    lower_surface->source_name == "/ObjectData/Shared.arc:/Shared.kcl" &&
                    lower_surface->source_name == upper_surface->source_name &&
                    lower_surface->sensor == &lower.sensor && upper_surface->sensor == &upper->sensor &&
                    lower_surface->placement_zone_id == 5 && upper_surface->placement_zone_id == 9,
                "diagnostic resource identity must remain unchanged and independent of actor/zone metadata");

        lower.actor.setName("RenamedLowerActor");
        require(std::string_view(lower_triangle.getHostName()) == "RenamedLowerActor",
                "the host getter must read the current NameObj name rather than a stale copied label");
        lower.sensor.mHost = nullptr;
        require(lower_triangle.getHostName() == nullptr && lower_triangle.getHostPlacementZoneID() == 5,
                "a missing sensor host has no name and does not change the part's retained zone");
        lower.sensor.mHost = &lower.actor;
        collision.build();
        require(lower_triangle.getHostPlacementZoneID() == 5 && upper_triangle.getHostPlacementZoneID() == 9,
                "rebuilding the query structure must preserve triangle owner provenance");

        upper->actor.makeActorDead();
        require(!upper_triangle.isValid() && upper_triangle.getHostName() == nullptr,
                "inactive actor collision must not publish stale owner pointers");
        upper->actor.makeActorAppeared();
        require(upper_triangle.isValid() && upper_triangle.getHostPlacementZoneID() == 9,
                "reappearing an actor must recover its original zone identity");
        upper.reset();
        require(!upper_triangle.isValid() && upper_triangle.getHostName() == nullptr,
                "released registrations must not dereference a destroyed sensor or actor");

        collision.clear();
        require(!lower_triangle.isValid() && lower_triangle.getHostName() == nullptr,
                "clearing the stage must withdraw previous triangle provenance");
        require(collision.add_kcl(kcl, floor_matrix(), "geometry-only.kcl"), "an unowned geometry fixture must still register");
        collision.build();
        const auto unowned = read_triangle(TVec3f(10.0F, 1.0F, 10.0F));
        require(unowned.getHostName() == nullptr && lower_triangle.getHostName() == nullptr,
                "geometry-only registrations must not pretend resource names are actor names or revive old triangles");
        auto zone_unavailable = false;
        try {
            (void)unowned.getHostPlacementZoneID();
        } catch (const std::logic_error&) {
            zone_unavailable = true;
        }
        require(zone_unavailable, "an absent authored zone must fail explicitly when requested");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };
}  // namespace

int main(int argc, char** argv) {
    const bool generated_only = argc == 2 && std::string_view(argv[1]) == "--generated-only";
    if (argc > 1 && !generated_only) return 2;
    constexpr auto tests = std::array{
        TestCase{"collision absent without registration", test_collision_is_absent_without_explicit_registration},
        TestCase{"only explicit valid KCL registers", test_only_explicit_valid_kcl_registration_adds_collision},
        TestCase{"triangle source matrix lifetime", test_triangle_source_matrix_lifetime},
        TestCase{"host owner/zone/resource separation and retirement", test_owner_name_and_zone_remain_separate_from_resources},
        TestCase{"line traversal Game heap ownership", test_repeated_line_queries_preserve_game_heap},
        TestCase{"generated original geometry publication and retirement", test_generated_geometry_publication_and_retirement},
    };

    auto failures = 0;
    auto executed = 0;
    for (const auto &test : tests) {
        if (generated_only && test.run != test_generated_geometry_publication_and_retirement) continue;
        ++executed;
        try {
            test.run();
            std::cout << "[ok] " << test.name << '\n';
        } catch (const std::exception &error) {
            ++failures;
            std::cerr << "[fail] " << test.name << ": " << error.what() << '\n';
        }
    }
    if (failures != 0) {
        std::cerr << failures << " stage collision registration test(s) failed\n";
        return 1;
    }
    std::cout << executed << " stage collision registration test(s) passed\n";
    return 0;
}
