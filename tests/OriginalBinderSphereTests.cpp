#include "OriginalSphereQueryFixture.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/TriangleFilter.hpp"

#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace {
    using Fixture = smgpc::test::OriginalSphereQueryFixture;

    void require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }

    void require_vector(const TVec3f& actual, const TVec3f& expected, const char* message) {
        require(std::isfinite(actual.x) && std::isfinite(actual.y) && std::isfinite(actual.z) &&
                    actual.epsilonEquals(expected, 0.0001f), message);
    }

    class RejectPart final : public TriangleFilterBase {
    public:
        explicit RejectPart(const CollisionParts* part) : rejected(part) {}
        bool isInvalidTriangle(const Triangle* triangle) const override {
            require(triangle && triangle->mParts, "original filter receives the real queried CollisionParts");
            ++calls;
            return triangle->mParts == rejected;
        }
        const CollisionParts* rejected;
        mutable unsigned calls = 0;
    };

    void margin_and_initial_step() {
        Fixture fixture;
        Fixture::Part floor(fixture, Fixture::triangles());
        const TVec3f position(2.0f, 0.75f, 2.0f);
        const TVec3f gravity(0.0f, -1.0f, 0.0f);
        Binder binder(nullptr, &position, &gravity, 1.0f, 0.0f, 4);
        require_vector(binder.bind(TVec3f(0.0f)), TVec3f(0.0f, 1.45f, 0.0f),
                       "original Binder adds its 1.2 margin to the returned 0.25 penetration");
        require(binder.mPlaneNum == 1 && binder.isBindedGround() &&
                    binder.mGroundInfo.mParentTriangle.mParts == &floor.parts &&
                    std::fabs(binder.mGroundInfo._60 - 1.45f) < 0.0001f,
                "Binder caches the actual original keeper hit and adjusted depth");

        binder._1EC._5 = true;
        require_vector(binder.bind(TVec3f(0.0f)), TVec3f(0.0f, 0.25f, 0.0f),
                       "one-shot no-margin binding uses the actual penetration only");
        require(!binder._1EC._5, "original bind consumes the one-shot no-margin flag");

        binder._1EC._3 = true;
        const TVec3f upward(0.0f, 2.0f, 0.0f);
        require_vector(binder.bind(upward), upward, "skip-initial binding tests the separated endpoint during takeoff");
        require(binder.mPlaneNum == 0 && !binder.isBindedGround() && binder._1EC._3,
                "a new bind clears old contacts but retains the persistent skip-initial flag");

        const TVec3f separated(2.0f, 1.1f, 2.0f);
        Binder outside(nullptr, &separated, &gravity, 1.0f, 0.0f, 4);
        require_vector(outside.bind(TVec3f(0.0f)), TVec3f(0.0f),
                       "Binder does not enlarge its query radius by the contact margin");
        require(outside.mPlaneNum == 0, "a separated sphere produces no support contact");
    }

    void classification_and_storage() {
        const std::array<std::array<float, 12>, 3> transforms{{
            Fixture::matrix(),
            {0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0},
            {1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0},
        }};
        const std::array<TVec3f, 3> positions{TVec3f(2, 0.75f, 2), TVec3f(0.75f, -2, 2), TVec3f(2, -0.75f, -2)};
        const std::array<TVec3f, 3> reactions{TVec3f(0, 1.45f, 0), TVec3f(1.45f, 0, 0), TVec3f(0, -1.45f, 0)};
        const TVec3f gravity(0, -1, 0);
        for (unsigned axis = 0; axis < transforms.size(); ++axis) {
            Fixture fixture;
            Fixture::Part part(fixture, Fixture::triangles(), transforms[axis]);
            for (unsigned capacity : {0U, 4U}) {
                Binder binder(nullptr, &positions[axis], &gravity, 1.0f, 0.0f, capacity);
                require_vector(binder.bind(TVec3f(0.0f)), reactions[axis], "original Binder resolves transformed floor/wall/roof penetration");
                require(binder.isBindedGround() == (axis == 0) && binder.isBindedWall() == (axis == 1) &&
                            binder.isBindedRoof() == (axis == 2), "original gravity-relative classification chooses exactly one contact cache");
                HitInfo* copied[4]{};
                require(binder.copyPlaneArrayAndSortingSensor(copied, 4) == 1 &&
                            copied[0]->mParentTriangle.mParts == &part.parts,
                        "both temporary and allocated plane storage expose the actual keeper contact");
            }
        }
    }

    void filtering_and_capacity() {
        Fixture fixture;
        Fixture::Part lower(fixture, Fixture::triangles());
        Fixture::Part upper(fixture, Fixture::triangles(), Fixture::matrix(1.0f, TVec3f(0, 0.5f, 0)));
        const TVec3f position(2, 0.75f, 2);
        const TVec3f gravity(0, -1, 0);
        Binder binder(nullptr, &position, &gravity, 1.0f, 0.0f, 4);
        RejectPart filter(&upper.parts);
        binder.setTriangleFilter(&filter);
        require_vector(binder.bind(TVec3f(0.0f)), TVec3f(0, 1.45f, 0),
                       "rejecting the deeper original triangle leaves the later accepted penetration");
        require(filter.calls >= 2 && binder.mPlaneNum == 1 &&
                    binder.mGroundInfo.mParentTriangle.mParts == &lower.parts,
                "the original triangle filter rejects before the contact reaches Binder storage");

        Binder limited(nullptr, &position, &gravity, 1.0f, 0.0f, 1);
        (void)limited.bind(TVec3f(0.0f));
        require(limited.mPlaneNum == 1, "multiple keeper contacts obey the original Binder plane capacity");
        require(limited.mPlane[0].mParentTriangle.mParts == &lower.parts,
                "capacity preserves original keeper order rather than depth-sorting contacts");
    }

    void rejected_parts_do_not_consume_hit_slots() {
        Fixture fixture;
        std::vector<std::unique_ptr<Fixture::Part>> rejected;
        for (unsigned i = 0; i < 32; ++i)
            rejected.push_back(std::make_unique<Fixture::Part>(fixture, Fixture::triangles()));
        Fixture::Part accepted(fixture, Fixture::triangles());
        struct OnlyPart final : TriangleFilterBase {
            bool isInvalidTriangle(const Triangle* triangle) const override { return triangle->mParts != accepted; }
            const CollisionParts* accepted = nullptr;
        } filter;
        filter.accepted = &accepted.parts;
        const TVec3f position(2, 0.75f, 2), gravity(0, -1, 0);
        require(Collision::checkStrikeBallToMap(position, 1, nullptr, &filter) == 1 &&
                    Collision::getStrikeInfoMap(0)->mParentTriangle.mParts == &accepted.parts,
                "32 rejected separate parts leave an output slot for the later accepted part");
        Binder binder(nullptr, &position, &gravity, 1.0f, 0.0f, 1);
        binder.setTriangleFilter(&filter);
        (void)binder.bind(TVec3f(0.0f));
        require(binder.mPlaneNum == 1 && binder.mGroundInfo.mParentTriangle.mParts == &accepted.parts,
                "one-plane Binder capacity is available after rejected separate parts");
    }

    void reaction_range_and_moving_parts() {
        const TVec3f position(0.0f), gravity(0, -1, 0);
        Binder binder(nullptr, &position, &gravity, 1.0f, 0.0f, 4);
        std::array<HitInfo, 4> planes;
        for (unsigned index : {0U, 3U}) {
            planes[index].mParentTriangle.mNormals[0].set(1, 1, 1);
            planes[index]._60 = 1000.0f;
        }
        planes[1].mParentTriangle.mNormals[0].set(0.6f, 0.8f, 0.0f);
        planes[1]._60 = 2;
        planes[1]._7C.set(4.0f, -3.0f, 0.5f);
        planes[2].mParentTriangle.mNormals[0].set(-0.8f, 0.0f, 0.6f);
        planes[2]._60 = 1;
        planes[2]._7C.set(-2, 1, -5);
        binder.mPlaneNum = 3;
        TVec3f reaction(99.0f);
        binder._1EC._1 = false;
        binder.obtainMomentFixReaction(planes.data(), 1, &reaction, 1);
        require_vector(reaction, TVec3f(0.4f, 1.6f, 0.6f),
                       "reaction uses the original suffix range and ignores its unused capacity argument");
        binder._1EC._1 = true;
        binder.obtainMomentFixReaction(planes.data(), 0, &reaction, 1);
        require_vector(reaction, TVec3f(2, -1.4f, -4.4f),
                       "moving-part reactions use component extrema, without normalizing or summing contacts");
        binder.obtainMomentFixReaction(planes.data(), 4, &reaction, 3);
        require_vector(reaction, TVec3f(0.0f), "empty original reaction range clears previous output");
    }

    void offsets_and_duplicate_contacts() {
        Fixture fixture;
        Fixture::Part floor(fixture, Fixture::triangles());
        const TVec3f gravity(0, -1, 0);
        const TVec3f position(2, 4.25f, 2);
        Mtx raw{{1, 0, 0, 2}, {0, -2, 0, 4.25f}, {0, 0, -1, 2}};
        Binder offset(raw, &position, &gravity, 0.5f, 2.0f, 4);
        require_vector(offset.bind(TVec3f(0.0f)), TVec3f(0, 1.45f, 0),
                       "Binder uses the raw scaled negative matrix column for its offset");

        const TVec3f point(2, -1, 2);
        Binder zero_radius(nullptr, &point, &gravity, 0, 0, 0);
        require_vector(zero_radius.bind(TVec3f(0.0f)), TVec3f(0, 2.2f, 0),
                       "zero-radius original Binder retains strict-interior prism depth plus margin");

        const TVec3f start(2, 0.75f, 2), movement(0, -0.5f, 0);
        Binder single(nullptr, &start, &gravity, 0.5f, 0, 4);
        const auto one = single.bind(movement);
        Fixture::Part duplicate(fixture, Fixture::triangles());
        Binder many(nullptr, &start, &gravity, 0.5f, 0, 4);
        require_vector(many.bind(movement), one, "coincident original contacts use component extrema rather than summed reactions");
        require(many.mPlaneNum == 2, "coincident original prism encounters remain distinct contacts");
        Binder limited(nullptr, &start, &gravity, 0.5f, 0, 1);
        require_vector(limited.bind(movement), one, "one-plane storage preserves the coincident-face response");
        require(limited.mPlaneNum == 1, "original plane capacity bounds copied contacts");
    }

    void derived_movement_observes_same_frame_binding() {
        Fixture fixture;
        require(MR::createSceneObj(SceneObj_ClippingDirector) != nullptr,
                "the movement probe requires the real original clipping owner");
        Fixture::Part floor(fixture, Fixture::triangles());
        struct Actor final : LiveActor {
            Actor() : LiveActor("original derived movement probe") {}
            void control() override { ++controls; mVelocity.set(0.0f, -0.5f, 0.0f); }
            void movement() override {
                LiveActor::movement();
                after_base = mPosition;
                saw_ground = mBinder->isBindedGround() && mBinder->mPlaneNum == 1;
            }
            unsigned controls = 0;
            bool saw_ground = false;
            TVec3f after_base;
        } actor;
        actor.mPosition.set(2.0f, 0.75f, 2.0f);
        actor.mFlag.mIsDead = false;
        actor.initBinder(0.5f, 0, 1);
        fixture.scheduler.connect_name_obj(actor, MR::MovementType_MapObj, -1, -1, -1);
        fixture.scene.complete_initialization();
        fixture.scene.apply_connections();
        // Exercise the original registered callback category. This geometry
        // fixture does not own the camera/demo graph needed by a whole scene.
        fixture.scheduler.execute_movement_category(MR::MovementType_MapObj);
        require(actor.controls == 1 && actor.saw_ground,
                "the derived actor sees this movement's original Binder contact before returning");
        require_vector(actor.mPosition, actor.after_base, "the native scheduler does not integrate again after the original actor method");
        require(std::fabs(actor.mPosition.y - 1.7f) < 0.0001f,
                "the original control/velocity/Binder order produces one resolved displacement");
    }

    void full_plane_array_still_stops_projected_retry() {
        Fixture fixture;
        Fixture::Part floor(fixture, Fixture::triangles());
        Fixture::Part wall(fixture, Fixture::triangles(100),
                           {0, -4, 0, 10, 4, 0, 0, 0, 0, 0, 4, 0});
        const TVec3f start(2, 0.25f, 2), gravity(0, -1, 0);
        Binder binder(nullptr, &start, &gravity, 0.5f, 0, 1);
        const auto end = start + binder.bind(TVec3f(80, 0, 0));
        require(binder.mPlaneNum == 1 && binder.mPlane[0].mParentTriangle.mParts == &floor.parts,
                "the original initial contact fills the single plane slot");
        require_vector(end, TVec3f(2 + 80.0f / 3.0f, 1.7f, 2),
                       "an unstored retry contact stops the original sweep step without adding a reaction");
    }
}

int main() {
    try {
        margin_and_initial_step();
        classification_and_storage();
        filtering_and_capacity();
        rejected_parts_do_not_consume_hit_slots();
        reaction_range_and_moving_parts();
        offsets_and_duplicate_contacts();
        derived_movement_observes_same_frame_binding();
        full_plane_array_still_stops_projected_retry();
        std::puts("PASS original-owner Binder sphere queries: margin, initial step, classification, storage, filtering and capacity");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL original-owner Binder sphere queries: %s\n", error.what());
        return 1;
    }
}
