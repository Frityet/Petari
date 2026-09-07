#include "Game/Scene/Scene.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/StageResourceBinding.hpp"
#include "compat/StageSessionState.hpp"
#include "compat/StageZoneMatrixRegistry.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/StageInitializationService.hpp"
#include <aurora/dvd.h>
#include <aurora/exception.hpp>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
    void require(bool value, const char *message) {
        if (!value)
            aurora::throw_host_exception<std::runtime_error>(message);
    }

    template <class Call>
    void require_logic_error(Call call, const char *message) {
        bool rejected = false;
        try {
            call();
        } catch (const std::logic_error &) {
            rejected = true;
        }
        require(rejected, message);
    }

    class Logger final : public smgpc::logging::ILogger {
        void write(std::FILE *, std::source_location, smgpc::logging::Level,
                   smgpc::logging::Category, std::string_view) override {
        }
    };

    void resources_and_retirement(smgpc::runtime::RuntimeContext &runtime) {
        const auto registrations = smgpc::compat::name_obj_runtime_state_count();
        const auto scheduled = runtime.scheduler().snapshot().size();
        std::weak_ptr<smgpc::compat::JkrAllocationDomain> retired_domain;
        for (int generation = 0; generation < 3; ++generation) {
            {
                auto domain = smgpc::compat::JkrAllocationDomain::create(runtime.host_heaps(), 8U << 20);
                retired_domain = domain;
                Scene scene("Original initialization resource fixture");
                {
                    smgpc::scene::StageInitializationService initialization(
                        runtime, scene,
                        {.scene_name = "Game", .stage_name = "GatewayGalaxy", .scenario_no = 1}, domain);
                    require_logic_error([&] { initialization.place_actors(); },
                                        "construction before archive preload must be rejected");
                    require_logic_error([&] { initialization.finish_actor_placement(); },
                                        "post-placement before construction must be rejected");
                    initialization.initialize_session();
                    require(smgpc::compat::require_active_stage_session().stage_name() == "GatewayGalaxy",
                            "stage metadata must be published before authored resource resolution");
                    initialization.bind_scene_objects();
                    require(smgpc::scene::current_scene_allocation_domain() == domain,
                            "Scene and native bindings must use the same original allocation domain");
                    require(scene.mSceneObjHolder == smgpc::scene::current_scene_obj_holder() && domain,
                            "the actual Scene owns its one bound SceneObjHolder and Game allocation domain");
                    require_logic_error([&] { initialization.bind_scene_objects(); },
                                        "rebinding cannot overwrite the original Scene holder");
                    require_logic_error([&] { initialization.initialize_scenario_resources(); },
                                        "scenario owners require completed stage file resolution");
                    {
                        // These native loading calls will also be reached inside
                        // original GameScene::init. Native retained data must escape
                        // the selected Game heap without changing the caller heap.
                        const smgpc::compat::JkrAllocationScope game(domain);
                        initialization.load_stage_files();
                        initialization.initialize_scenario_resources();
                        require(smgpc::compat::current_jkr_allocation_domain() == domain,
                                "native resource initialization must restore the original caller allocation domain");
                    }
                    auto &resources = smgpc::compat::require_stage_resources();
                    require(resources.start_count() > 0, "the real Gateway archive must retain authored StartInfo rows");
                    JMapIdInfo first_camera;
                    resources.start_camera_id(&first_camera, 0);
                    void *camera_data = nullptr;
                    s32 camera_size = 0;
                    resources.camera_data(&camera_data, &camera_size, first_camera.mZoneID);
                    require(camera_data && camera_size > 0,
                            "the actual start-camera zone must resolve a retained camera table");
                    auto *matrix = smgpc::compat::require_stage_zone_matrices().matrix_for_zone(first_camera.mZoneID);
                    require(matrix != nullptr, "the camera zone must have its actual authored transform");
                    void *retained_camera_data = nullptr;
                    s32 retained_camera_size = 0;
                    resources.camera_data(&retained_camera_data, &retained_camera_size, first_camera.mZoneID);
                    require(camera_data == retained_camera_data && camera_size == retained_camera_size,
                            "repeated camera queries must preserve the same archive resource identity");
                    require(!MR::isExistSceneObj(SceneObj_MarioHolder) && initialization.root() == nullptr,
                            "resource loading must not manufacture actors or a MarioHolder");
                    require_logic_error([&] { initialization.load_stage_files(); },
                                        "duplicate load cannot replace borrowed authored table addresses");
                    require_logic_error([&] { initialization.initialize_scenario_resources(); },
                                        "duplicate scenario initialization cannot replace borrowed owner addresses");
                    require_logic_error([&] { initialization.complete_initialization(); },
                                        "resource loading alone must not claim completed scene initialization");
                    require(smgpc::compat::require_active_stage_session().execution_phase() ==
                                smgpc::compat::StageSessionState::ExecutionPhase::Initialization,
                            "a resource-only stage remains in its actual initialization phase");
                }
            }
            require(retired_domain.expired() && !smgpc::scene::current_scene_obj_holder() &&
                        !smgpc::compat::try_active_stage_session(),
                    "partial initialization retirement must release bindings and the actual Game heap");
            require(smgpc::compat::name_obj_runtime_state_count() == registrations &&
                        runtime.scheduler().snapshot().size() == scheduled,
                    "resource retirement must remove Demo/SceneObj registrations before the next generation");
            require_logic_error([] { (void)smgpc::compat::require_stage_resources(); },
                                "retired stage resources must be unavailable");
        }
    }
}  // namespace

int main() {
    try {
        const auto *disc = std::getenv("SMGPC_REAL_DISC");
        require(disc, "SMGPC_REAL_DISC must name the real disc");
        smgpc::render::AuroraWindow window({.width = 640, .height = 456, .title = "Stage initialization resources"});
        smgpc::render::AuroraRenderer renderer(window);
        require(aurora_dvd_open(disc), "cannot open requested real disc");
        struct Disc {
            ~Disc() {
                aurora_dvd_close();
            }
        } close_disc;
        DVDInit();
        smgpc::resource::GameResourceRuntime process({96U << 20, 32U << 20, 4U << 20});
        Logger logger;
        smgpc::runtime::RuntimeContext runtime(logger, window, process);
        smgpc::runtime::SceneSchedulerBinding scheduler_binding(runtime.scheduler());
        (void)renderer.begin_frame();
        resources_and_retirement(runtime);
        renderer.end_frame();
        std::cout << "Real-disc stage resource boundaries and three partial-initialization retirement generations passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
