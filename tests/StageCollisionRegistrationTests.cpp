#include "OriginalSphereQueryFixture.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Map/CollisionParts.hpp"
#include "Game/MapObj/DynamicCollisionObj.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "resource/KCollisionResource.hpp"
#include "scene/StageCollisionService.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <aurora/exception.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            aurora::throw_host_exception<std::runtime_error>(std::string(message));
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
        smgpc::scene::StageCollisionService collision;
        collision.activate();
        require(collision.empty() && collision.stats().mesh_count == 0 &&
                    collision.stats().triangle_count == 0 && collision.stats().rejected_triangle_count == 0,
                "absence must not be represented as synthesized or rejected geometry");
        require(!collision.surface(0), "an empty registry exposes no diagnostic surface");
        collision.require_published_geometry();
    }

    void test_only_explicit_valid_kcl_registration_adds_collision() {
        smgpc::test::OriginalSphereQueryFixture original;
        const auto kcl = smgpc::test::OriginalSphereQueryFixture::triangles();
        smgpc::test::OriginalSphereQueryFixture::Part part(original, kcl);
        auto registration = std::make_shared<smgpc::scene::StageCollisionRegistrationState>(nullptr, &part.parts);
        smgpc::scene::StageCollisionService collision;
        const std::array<std::uint8_t, 4> malformed{};
        require(!collision.add_kcl(malformed, cIdentity, "Missing.kcl") && collision.empty(),
                "a malformed exact resource must remain absent");
        require(collision.register_kcl(kcl, cIdentity, "Exact.kcl", registration).accepted,
                "an explicit valid original KCL registration is accepted");
        const auto surface = collision.surface(&part.parts, 0);
        require(surface && collision.stats().mesh_count == 1 && collision.stats().triangle_count == 1 &&
                    surface->source_name == "Exact.kcl" && surface->prism_index == 0,
                "registration publishes metadata for exactly the requested original prism");
        registration->release_owner();
    }

    void test_triangle_source_matrix_lifetime() {
        smgpc::test::OriginalSphereQueryFixture original;
        const auto kcl = smgpc::test::OriginalSphereQueryFixture::triangles();
        const std::array<float, 12> matrix{0, 0, 2, 10, 0, 3, 0, 20, -4, 0, 0, 30};
        smgpc::test::OriginalSphereQueryFixture::Part part(original, kcl, matrix);
        auto registration = std::make_shared<smgpc::scene::StageCollisionRegistrationState>(nullptr, &part.parts);
        smgpc::scene::StageCollisionService collision;
        require(collision.register_kcl(kcl, matrix, "transformed source", registration).accepted,
                "register an exact rotated and scaled source");
        const auto surface = collision.surface(&part.parts, 0);
        require(surface.has_value(), "registered part retains a diagnostic surface");
        const auto id = surface->triangle_index;
        auto* matrices = &collision.matrices_for_triangle(id);
        const TVec3f position(10.5F, 20.0F, 29.0F);
        TVec3f local, restored;
        PSMTXMultVec(matrices->inverse, &position, &local);
        require(local.epsilonEquals(TVec3f(.25F, 0, .25F), .00001F),
                "the borrowed inverse preserves authored scale and rotation");
        PSMTXMultVec(matrices->base, &local, &restored);
        require(restored.epsilonEquals(position, .00001F), "the full base reconstructs the world point");
        PSMTXMultVec(matrices->previous, &local, &restored);
        require(restored.epsilonEquals(position, .00001F), "static sources preserve their previous matrix");
        for (unsigned i = 0; i < 64; ++i)
            require(collision.add_kcl(kcl, cIdentity, "additional source"), "grow the source registry");
        require(&collision.matrices_for_triangle(id) == matrices,
                "borrowed transforms remain stable across source-vector relocation");
        registration->set_enabled(false);
        require_unavailable([&] { (void)collision.matrices_for_triangle(id); }, "disabled source transforms are unavailable");
        require(collision.surface(&part.parts, 0).has_value(), "disabled metadata remains diagnostic");
        registration->set_enabled(true);
        require(&collision.matrices_for_triangle(id) == matrices, "reenabling preserves matrix identity");
        registration->release_owner();
        require_unavailable([&] { (void)collision.matrices_for_triangle(id); }, "released transforms are unavailable");
        collision.clear();
        require(!collision.surface(id), "clearing removes retained surface identities");
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
        parts.mServer->init(resource->native_file(), nullptr);
        parts.mHitSensor = sensor;
        actor.mParts = &parts;
        actor.syncCollision();
        auto registration = std::make_shared<smgpc::scene::StageCollisionRegistrationState>(nullptr, &parts);
        smgpc::scene::StageCollisionService collision;
        require(collision.register_generated_kcl(resource, *parts.mServer, cIdentity,
                    "original generated square", registration, sensor, 0).accepted,
                "the original writer's separate arrays must register as generated geometry");
        const auto first = collision.surface(&parts, 0);
        const auto second = collision.surface(&parts, 1);
        require(first && second && first->sensor == sensor && first->parts == &parts,
                "generated surfaces retain their actual original part and opaque sensor identity");
        const auto first_id = first->triangle_index;
        const auto second_id = second->triangle_index;
        const auto check_surface = [&](float z) {
            const auto surface = collision.surface(first_id);
            require(surface && surface->parts == &parts && surface->prism_index == 0,
                    "publication retains the actual generated part and prism");
            for (const auto& vertex : surface->vertices)
                require(std::abs(vertex.z - z) < .001F, "cached vertices follow the original generated writer");
            collision.require_published_geometry();
        };
        check_surface(0);
        for (auto& vertex : vertices) vertex.z = 3;
        actor.syncCollision();
        collision.update_registered_geometry(*registration);
        check_surface(3);
        require(collision.surface(&parts, 0)->triangle_index == first_id &&
                    collision.surface(&parts, 1)->triangle_index == second_id,
                "geometry refresh must preserve global surface and original local prism identities");
        registration->set_enabled(false);
        require(!collision.surface(first_id),
                "disabled generated membership has no enabled diagnostic surface");
        registration->set_enabled(true);
        check_surface(3);
        // Original KCollision represents a collapsed/near-parallel face by a
        // nonpositive height. Its stable slot must survive deactivation.
        const auto saved = vertices;
        for (auto& vertex : vertices) vertex.z = 5;
        vertices[3] = vertices[0];
        actor.syncCollision();
        collision.update_registered_geometry(*registration);
        check_surface(5);
        require(!collision.surface(second_id) && !collision.surface(&parts, 1) &&
                    collision.stats().triangle_count == 2,
                "collapsed original prism must retain its identity slot without exposing stale geometry");
        vertices = saved;
        actor.syncCollision();
        collision.update_registered_geometry(*registration);
        require(collision.surface(&parts, 1)->triangle_index == second_id,
                "restored original geometry must reactivate its prior stable prism identity");
        check_surface(3);
        for (std::size_t i = 1; i < allocation->prisms.size(); ++i)
            allocation->prisms[i].mHeight = -std::abs(allocation->prisms[i].mHeight);
        collision.update_registered_geometry(*registration);
        require(!collision.surface(first_id) && !collision.surface(second_id) &&
                    collision.stats().triangle_count == 2,
                "inactive generated prisms retain their identity slots without stale surfaces");
        auto shifted = cIdentity;
        shifted[3] = 20;
        collision.update_registered_transform(*registration, shifted, cIdentity);
        actor.syncCollision();
        collision.update_registered_geometry(*registration);
        require(collision.surface(&parts, 0)->triangle_index == first_id &&
                    collision.surface(&parts, 0)->vertices[0].x == 20,
                "reactivated geometry must use a transform committed while all its prisms were inactive");
        collision.update_registered_transform(*registration, cIdentity, shifted);
        check_surface(3);
        // Malformed references remain errors. No partial cache mutation may
        // escape before the full retained resource passes validation.
        allocation->prisms[2].mNormalIndex = 99;
        bool rejected = false;
        try { collision.update_registered_geometry(*registration); }
        catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, "invalid generated references must fail before publication");
        require(collision.surface(first_id)->vertices[0].z == 3,
                "failed geometry publication must leave the previous cached surface intact");
        require_unavailable([&] { collision.require_published_geometry(); },
                            "rejected mutations quarantine the original query publication boundary");
        registration->set_enabled(false);
        collision.require_published_geometry();
        require(!collision.surface(first_id), "disabled quarantined geometry remains absent");
        registration->set_enabled(true);
        require_unavailable([&] { collision.require_published_geometry(); },
                            "reenabling cannot clear failed-publication quarantine");
        actor.syncCollision();
        collision.update_registered_geometry(*registration);
        check_surface(3);
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
        collision.require_published_geometry();
        require(!collision.surface(first_id), "retired surfaces never dereference a destroyed original server");
        collision.clear();
        require(retired.expired() && !smgpc::resource::is_native_kcollision_file(identity),
                "final service retirement releases generated arrays and typed file identity");
    }

    void test_owner_name_and_zone_remain_separate_from_resources() {
        smgpc::test::OriginalSphereQueryFixture original;
        require(MR::createSceneObj(SceneObj_ClippingDirector) != nullptr,
                "metadata uses the actual original actor-registration owner");
        LiveActor lower("LowerActor"), upper("UpperActor");
        lower.makeActorAppeared(); upper.makeActorAppeared();
        HitSensor lower_sensor(0, 0, 1, &lower), upper_sensor(0, 0, 1, &upper);
        const auto kcl = smgpc::test::OriginalSphereQueryFixture::triangles();
        smgpc::test::OriginalSphereQueryFixture::Part lower_part(original, kcl, cIdentity, &lower_sensor);
        smgpc::test::OriginalSphereQueryFixture::Part upper_part(original, kcl, cIdentity, &upper_sensor);
        auto lower_registration = std::make_shared<smgpc::scene::StageCollisionRegistrationState>(
            &lower.mFlag.mIsDead, &lower_part.parts);
        auto upper_registration = std::make_shared<smgpc::scene::StageCollisionRegistrationState>(
            &upper.mFlag.mIsDead, &upper_part.parts);
        smgpc::scene::StageCollisionService collision;
        require(collision.register_kcl(kcl, cIdentity, "Shared.kcl", lower_registration, {}, &lower_sensor, 5).accepted &&
                    collision.register_kcl(kcl, cIdentity, "Shared.kcl", upper_registration, {}, &upper_sensor, 9).accepted,
                "two actual owners may share one resource identity and retain distinct placement zones");
        const auto first = collision.surface(&lower_part.parts, 0);
        const auto second = collision.surface(&upper_part.parts, 0);
        require(first && second && first->triangle_index != second->triangle_index &&
                    first->source_name == second->source_name && first->sensor == &lower_sensor &&
                    second->sensor == &upper_sensor && first->placement_zone_id == 5 && second->placement_zone_id == 9,
                "resource identity, sensor ownership, global identity and zone provenance stay separate");
        lower.setName("RenamedLowerActor");
        require(std::string_view(collision.surface(first->triangle_index)->sensor->mHost->mName) == "RenamedLowerActor",
                "publication retains the current owner rather than a copied name");
        upper.makeActorDead();
        require(!collision.surface(second->triangle_index) && collision.surface(&upper_part.parts, 0),
                "dead actor surfaces remain retained but disabled");
        upper.makeActorAppeared();
        require(collision.surface(second->triangle_index).has_value(), "appearance restores the same live identity");
        upper_registration->release_owner();
        require(!collision.surface(second->triangle_index) && !collision.surface(&upper_part.parts, 0),
                "release withdraws owner lookup before destruction");
        lower_registration->release_owner();
        collision.clear();
        require(!collision.surface(first->triangle_index), "scene clearing withdraws old identities");
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
