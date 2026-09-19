#include "Game/AreaObj/CollisionArea.hpp"
#include "Game/Map/CollisionParts.hpp"
#include "Game/Map/CollisionCategorizedKeeper.hpp"
#include "Game/Map/KCollision.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioHolder.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/GameDataSession.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/MarioCameraTarget.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "resource/KCollisionResource.hpp"
#include "resource/TextEncoding.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/GatewayDemoScene.hpp"
#include "scene/NameObjChildOwner.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/SceneInitializationState.hpp"
#include "scene/nameobj/NameObjFactory.hpp"
#include "render/RendererService.hpp"
#include "Logger.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
    void require(bool value, const char* message) {
        if (!value) throw std::runtime_error(message);
    }

    void near(float value, float expected, const char* message) {
        require(std::isfinite(value) && std::abs(value - expected) < 0.05F, message);
    }

    class MarioOwner {
    public:
        explicit MarioOwner(smgpc::runtime::RuntimeContext& runtime) : _runtime(runtime) {
            const smgpc::scene::SceneInitializationScope phase(SceneInitializeState_PlacementPlayer);
            const auto domain = smgpc::scene::current_scene_allocation_domain();
            _actor = static_cast<MarioActor*>(_objects.capture_construction_children([&] {
                const smgpc::compat::JkrAllocationScope game(domain);
                return createNameObj<MarioActor>("MarioActor");
            }));
            _runtime.player_system().attach_actor(*_actor, {
                .read_element_mode = +[](const LiveActor& actor) -> s32 { return static_cast<const MarioActor&>(actor).mPlayerMode; },
                .read_base_matrix = +[](const LiveActor& actor) {
                    return smgpc::compat::mario_camera_base_matrix(static_cast<const MarioActor&>(actor));
                },
                .read_up_vector = +[](const LiveActor& actor, TVec3f* out) { static_cast<const MarioActor&>(actor).getUpVec(out); },
                .read_front_vector = +[](const LiveActor& actor, TVec3f* out) { static_cast<const MarioActor&>(actor).getFrontVec(out); },
                .read_side_vector = +[](const LiveActor& actor, TVec3f* out) { static_cast<const MarioActor&>(actor).getSideVec(out); },
            });
        }
        ~MarioOwner() {
            if (_runtime.player_system().attached_actor() == _actor) _runtime.player_system().detach_actor(_actor);
            if (auto* holder = MR::getMarioHolder(); holder && holder->getMarioActor() == _actor) holder->setMarioActor(nullptr);
            _objects.clear();
        }
        void init(const JMapInfoIter& iter) {
            const smgpc::scene::SceneInitializationScope phase(SceneInitializeState_PlacementPlayer);
            const auto domain = smgpc::scene::current_scene_allocation_domain();
            _objects.capture_construction_children([&] {
                const smgpc::compat::JkrAllocationScope game(domain);
                _actor->init(iter);
            });
        }
        MarioActor& actor() { return *_actor; }
    private:
        smgpc::runtime::RuntimeContext& _runtime;
        smgpc::scene::NameObjChildOwner _objects;
        MarioActor* _actor = nullptr;
    };

    const KCLFile* exercise_polygon(smgpc::scene::StageCollisionService& collision) {
        const auto domain = smgpc::scene::current_scene_allocation_domain();
        smgpc::scene::NameObjChildOwner objects;
        TPos3f matrix;
        matrix.identity();
        matrix.setTrans(TVec3f(50000.0F, 50000.0F, 50000.0F));
        auto* polygon = static_cast<AreaPolygon*>(objects.capture_construction_children([&] {
            const smgpc::compat::JkrAllocationScope game(domain);
            auto* result = new AreaPolygon;
            result->setMtx(matrix, TVec3f(200.0F, 400.0F, 600.0F), 0.0F);
            result->init(JMapInfoIter());
            return result;
        }));
        auto* parts = polygon->mParts;
        const auto* file = polygon->mKCLFile;
        require(parts && parts->_CC && parts->mServer->mFile == file && parts->mServer->getTriangleNum() == 2,
                "Original AreaPolygon creates and validates its actual two-prism collision part");
        const auto initial0 = collision.surface(parts, 0);
        const auto initial1 = collision.surface(parts, 1);
        require(initial0 && initial1, "Original generated prisms publish actual surfaces");
        const auto id0 = initial0->triangle_index;
        const auto id1 = initial1->triangle_index;
        const auto zone_count = parts->mZone->mNumParts;
        const std::array<TVec3f, 6> normals{TVec3f(1, 0, 0), TVec3f(-1, 0, 0), TVec3f(0, 1, 0),
                                          TVec3f(0, -1, 0), TVec3f(0, 0, 1), TVec3f(0, 0, -1)};
        const std::array<float, 6> distances{110, 110, 210, 210, 310, 310};
        for (s32 face = 0; face < 6; ++face) {
            const smgpc::compat::JkrAllocationScope game(domain);
            polygon->setSurfaceAndSync(face);
            const auto first = collision.surface(parts, 0);
            const auto second = collision.surface(parts, 1);
            require(first && second && first->triangle_index == id0 && second->triangle_index == id1,
                    "Face changes retain original part, local prisms and stable native identities");
            near(first->normals[0].x, normals[face].x, "Updated face normal X");
            near(first->normals[0].y, normals[face].y, "Updated face normal Y");
            near(first->normals[0].z, normals[face].z, "Updated face normal Z");
            const auto center = TVec3f(50000, 50000, 50000);
            const auto tangent = face < 2 ? TVec3f(0, 17, 31) : (face < 4 ? TVec3f(17, 0, 31) : TVec3f(17, 31, 0));
            const auto start = center + tangent + normals[face] * 1000.0F;
            const auto offset = normals[face] * -2000.0F;
            const auto hits = collision.line_hits(start, offset);
            const auto hit = std::find_if(hits.begin(), hits.end(), [&](const auto& value) {
                return value.triangle_index == id0 || value.triangle_index == id1;
            });
            require(hit != hits.end(), "Original octree arrow traversal reaches each regenerated face");
            near(hit->fraction, (1000.0F - distances[face]) / 2000.0F, "Regenerated face arrow fraction");
            smgpc::scene::StageCollisionHit nearest;
            require(collision.line_cast(start, offset, &nearest) &&
                    (nearest.triangle_index == id0 || nearest.triangle_index == id1),
                    "Refitted native broad phase reaches the same regenerated geometry");
            near(nearest.fraction, hit->fraction, "Native and original regenerated line query agreement");
        }
        polygon->invalidate();
        require(!parts->_CC && !collision.surface(id0) && parts->mZone->mNumParts + 1 == zone_count,
                "Original invalidation removes the generated part from both zone and native queries");
        polygon->validate();
        require(parts->_CC && collision.surface(id0) && parts->mZone->mNumParts == zone_count,
                "Original validation restores the same generated part and surfaces");
        objects.clear();
        require(!collision.surface(id0) && !collision.surface(parts, 0),
                "Original actor retirement invalidates cached generated surfaces before its storage dies");
        return file;
    }
}

int main() {
    try {
        const auto* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc && aurora_dvd_open(disc), "SMGPC_REAL_DISC must open the actual game archive");
        struct Disc { ~Disc() { aurora_dvd_close(); } } disc_guard;
        DVDInit();
        auto logger = smgpc::logging::create_default_logger();
        smgpc::render::AuroraWindow window({.width = 640, .height = 456, .title = "Original generated collision proof"});
        smgpc::render::AuroraRenderer renderer(window);
        smgpc::resource::GameResourceRuntime resources;
        smgpc::runtime::RuntimeContext runtime(*logger, window, resources);
        runtime.initialize_scenario_catalog(resources);
        runtime.initialize_particle_resources(resources);
        runtime.set_current_stage_name("HeavensDoorGalaxy");
        const smgpc::render::ScopedAuroraRendererContext renderer_context(renderer);
        smgpc::compat::GameDataSession game_data(1U, resources, runtime.retain_scenario_catalog());
        game_data.holder().followStoryEventByName(smgpc::resource::encode_cp932("ピーチ城浮上後").c_str());
        game_data.store_scene_start();
        for (int generation = 0; generation < 2; ++generation) {
            const KCLFile* retired = nullptr;
            {
                smgpc::scene::GatewayDemoScene scene(runtime.dvd());
                MarioOwner mario(runtime);
                (void)renderer.begin_frame();
                mario.init(scene.player_start_iter());
                auto placements = scene.finalize_placements(mario.actor());
                require(MR::getMarioHolder()->getMarioActor() == &mario.actor(),
                        "AreaPolygon player-mode queries use the actual initialized original Mario");
                retired = exercise_polygon(scene.collision());
                renderer.end_frame();
            }
            require(!smgpc::resource::is_native_kcollision_file(retired),
                    "Scene retirement releases the final generated typed-resource identity");
        }
        std::cout << "PASS original AreaPolygon six faces, generated geometry refresh, original/native queries, zone membership and two scene retirements\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
