#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "Game/NPC/DemoRabbit.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "JSystem/J3DGraphAnimator/J3DModel.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <unistd.h>
#include <vector>

namespace {
    void require(bool condition, const char* message) {
        if (!condition) {
            std::fprintf(stderr, "[npc-orientation] assertion failed: %s\n", message);
            std::fflush(stderr);
            aurora::throw_host_exception<std::runtime_error>(message);
        }
    }

    void check_comparison(const TVec3f& a, const TVec3f& b, bool equal) {
        require((a == b) == equal && (b == a) == equal &&
                    (a != b) == !equal && (b != a) == !equal,
                "const vector equality/inequality compare all components using the original epsilon");
        auto mutable_a = a;
        auto mutable_b = b;
        require((mutable_a == mutable_b) == equal && (mutable_a != mutable_b) == !equal &&
                    (mutable_a == b) == equal && (a == mutable_b) == equal,
                "mutable and mixed-const comparisons retain value semantics");
    }

    void verify_vector_comparison() {
        const TVec3f zero(0, 0, 0);
        const TVec3f separate_zero(0, 0, 0);
        check_comparison(zero, separate_zero, true);
        const auto& alias = zero;
        check_comparison(zero, alias, true);
        check_comparison(TVec3f(17, -29, 63), TVec3f(17, -29, 63), true);
        constexpr float epsilon = JGeometry::TUtil<float>::epsilon();
        const float outside = std::nextafter(epsilon, std::numeric_limits<float>::infinity());
        for (int axis = 0; axis < 3; ++axis) {
            for (float sign : {-1.0F, 1.0F}) {
                TVec3f edge(0, 0, 0), beyond(0, 0, 0);
                const std::array<float*, 3> edge_components{&edge.x, &edge.y, &edge.z};
                const std::array<float*, 3> beyond_components{&beyond.x, &beyond.y, &beyond.z};
                *edge_components[axis] = sign * epsilon;
                *beyond_components[axis] = sign * outside;
                check_comparison(zero, edge, true);
                check_comparison(zero, beyond, false);
            }
        }
        check_comparison(zero, TVec3f(epsilon, -epsilon, epsilon), true);
        check_comparison(zero, TVec3f(-0.0F, 0.0F, -0.0F), true);
        for (int axis = 0; axis < 3; ++axis) {
            TVec3f unordered(0, 0, 0);
            const std::array<float*, 3> components{&unordered.x, &unordered.y, &unordered.z};
            *components[axis] = std::numeric_limits<float>::quiet_NaN();
            check_comparison(zero, unordered, false);
            const auto& unordered_alias = unordered;
            check_comparison(unordered, unordered_alias, false);
        }
        const TVec3f infinite(std::numeric_limits<float>::infinity(), 0, 0);
        check_comparison(infinite, infinite, false);  // inf - inf is unordered in the donor.
        std::fprintf(stderr, "[npc-orientation] PASS original vector value equality, const/alias and all component boundaries\n");
    }

#ifndef NDEBUG
    template<class F>
    struct Restore final {
        F action;
        ~Restore() { action(); }
    };

    void require_vector(const TVec3f& actual, const TVec3f& expected, const char* message) {
        require(std::fabs(actual.x - expected.x) < 0.00003F &&
                    std::fabs(actual.y - expected.y) < 0.00003F &&
                    std::fabs(actual.z - expected.z) < 0.00003F, message);
    }

    void verify_original_npc_cache(DemoRabbit& actor) {
        auto* model = MR::getJ3DModel(&actor);
        require(model, "actual guide NPC has its original model owner");
        const auto original_rotation = actor.mRotation;
        const auto original_cache = actor._CC;
        const auto original_quaternion = actor._A0;
        Mtx original_matrix;
        PSMTXCopy(model->getBaseTRMtx(), original_matrix);
        const auto restore = Restore{[&] {
            actor.mRotation = original_rotation;
            actor._CC = original_cache;
            actor._A0 = original_quaternion;
            PSMTXCopy(original_matrix, model->getBaseTRMtx());
        }};

        const auto check_basis = [&](const TVec3f& up, const TVec3f& front) {
            const auto& matrix = model->getBaseTRMtx();
            require_vector(TVec3f(matrix[0][1], matrix[1][1], matrix[2][1]), up,
                           "original NPC callback retains the control quaternion's up axis");
            require_vector(TVec3f(matrix[0][2], matrix[1][2], matrix[2][2]), front,
                           "original NPC callback retains the control quaternion's facing axis");
            require_vector(TVec3f(matrix[0][3], matrix[1][3], matrix[2][3]), actor.mPosition,
                           "original NPC callback keeps the actual actor translation");
        };

        // Inject only local pose inputs, then call the actual untouched callback.
        // A 90-degree world-Y rotation differs from the unchanged zero Euler cache.
        const auto half_sqrt_two = std::sqrt(0.5F);
        actor.mRotation.set(0, 0, 0);
        actor._CC.set(0, 0, 0);
        actor._A0.set(0.0F, half_sqrt_two, 0.0F, half_sqrt_two);
        actor.NPCActor::calcAndSetBaseMtx();
        check_basis(TVec3f(0, 1, 0), TVec3f(1, 0, 0));

        // A genuine Euler edit must still invalidate the cache and produce Rx(90).
        actor.mRotation.set(90, 0, 0);
        actor.NPCActor::calcAndSetBaseMtx();
        check_basis(TVec3f(0, 0, 1), TVec3f(0, -1, 0));
        require_vector(actor._CC, actor.mRotation, "original NPC callback caches the new Euler value");

        // Subsequent control can tilt the up axis without changing that Euler value.
        actor._A0.set(half_sqrt_two, 0.0F, 0.0F, half_sqrt_two);
        actor._A0.mult(TQuat4f(0.0F, half_sqrt_two, 0.0F, half_sqrt_two));
        actor.NPCActor::calcAndSetBaseMtx();
        check_basis(TVec3f(1, 0, 0), TVec3f(0, -1, 0));
    }

    struct Probe {
        bool exercised = false;
        std::uint64_t normal_frames = 0;
        std::vector<DemoRabbit*> actors;

        void after_frame(GameSystem& system, std::uint64_t frame) {
            auto* controller = system.mSceneController;
            if (!controller || controller->mSceneInitializeState != SceneInitializeState_End ||
                controller->getCurrentSceneForExecute() != controller->mScene ||
                !dynamic_cast<GameScene*>(controller->mScene)) return;
            if (exercised) {
                ++normal_frames;
                return;
            }
            const aurora::allocation::HostAllocationScope host;
            // This observation survives scene teardown; keep its vector storage
            // in the host allocation domain before entering the guest scope.
            for (auto* object : smgpc::compat::snapshot_name_obj_runtime_objects())
                if (auto* actor = dynamic_cast<DemoRabbit*>(object)) actors.push_back(actor);
            require(!actors.empty(), "ordinary authored placement constructed at least one real DemoRabbit");
            const auto domain = smgpc::scene::current_scene_allocation_domain();
            require(domain != nullptr, "the ordinary original scene owns all NPC allocations");
            const smgpc::compat::JkrAllocationScope allocation(domain);
            const J3DSys::CommandScope commands;
            for (auto* actor : actors) verify_original_npc_cache(*actor);
            exercised = true;
            std::fprintf(stderr, "[npc-orientation] PASS original NPC cache/control pose, Euler invalidation and model basis; actors=%zu frame=%llu\n",
                         actors.size(), static_cast<unsigned long long>(frame));
        }
    };

    void verify_process() {
        const char* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc && *disc, "SMGPC_REAL_DISC must name the real disc image");
        const auto save = std::filesystem::temp_directory_path() /
                          ("petari-original-npc-orientation-" + std::to_string(getpid()));
        require(!std::filesystem::exists(save), "diagnostic starts with a fresh console directory");
        setenv("SMGPC_SAVE_DIR", save.c_str(), 1);
        for (const auto* name : {"SMGPC_NAND_DIR", "SMGPC_DEBUG_WPAD_BUTTON_SCRIPT", "SMGPC_DEBUG_WPAD_POINTER_SCRIPT",
                                "SMGPC_DEBUG_WPAD_STICK_SCRIPT", "SMGPC_DEBUG_WPAD_INPUT_FILE", "SMGPC_STRICT_PLACEMENT"})
            unsetenv(name);
        const smgpc::app::BootstrapConfiguration configuration{
            .window_width = 640, .window_height = 456, .window_title = "Original NPC orientation regression",
            .arguments = {"original-npc-orientation-test", "--stage", "HeavensDoorGalaxy", "--scenario", "1", "--max-frames", "120"},
            .disc_image = disc,
        };
        auto logger = smgpc::logging::create_default_logger();
        smgpc::app::ensure_disc_image_open(configuration, *logger);
        struct DiscLifetime { ~DiscLifetime() { smgpc::app::close_disc_image(); } } disc_lifetime;
        Probe probe;
        const smgpc::app::OriginalGameDebugObserver observer{
            .context = &probe,
            .after_frame = +[](void* context, GameSystem& system, std::uint64_t frame) {
                static_cast<Probe*>(context)->after_frame(system, frame);
            },
        };
        require(smgpc::app::run_original_game(configuration, *logger, observer) == 0 &&
                    probe.exercised && probe.normal_frames >= 30,
                "original process completes the owner checks and subsequent ordinary frames");
        for (auto* actor : probe.actors)
            require(!smgpc::compat::has_actor_runtime_state(actor), "normal scene teardown retires each observed NPC");
        std::fprintf(stderr, "[npc-orientation] PASS original 120-frame process and actual NPC retirement\n");
    }
#endif
}

int main(int argc, char** argv) {
    try {
        require(argc <= 2, "expected optional --vectors-only or --owner-only");
        const std::string_view mode = argc == 2 ? argv[1] : "";
        require(mode.empty() || mode == "--vectors-only" || mode == "--owner-only", "unknown diagnostic mode");
        if (mode != "--owner-only") verify_vector_comparison();
        if (mode != "--vectors-only") {
#ifndef NDEBUG
            verify_process();
#else
            require(false, "original-process owner diagnostic requires a debug build");
#endif
        }
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL original NPC orientation: %s\n", error.what());
        return 1;
    }
}
