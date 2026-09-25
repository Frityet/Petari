#include "scene/SceneExecutionService.hpp"

#include "Game/Map/Air.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/SceneScheduler.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include "Game/NPC/TalkDirector.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"

namespace smgpc::scene {

    SceneExecutionService::SceneExecutionService(smgpc::runtime::RuntimeContext &runtime) : _runtime(runtime) {
    }

    SceneExecutionService::~SceneExecutionService() = default;

    void SceneExecutionService::execute_movement() {
        auto &scheduler = _runtime.scheduler();
        const smgpc::compat::JkrAllocationScope game(scheduler.allocation_domain());
        const J3DSys::ContextScope commands;
        scheduler.begin_frame();
        SceneFunction::movementStopSceneController();
        SceneFunction::executeMovementList();
    }

    void SceneExecutionService::execute_calc_anim_and_view() {
        const smgpc::compat::JkrAllocationScope game(_runtime.scheduler().allocation_domain());
        const J3DSys::ContextScope commands;
        SceneFunction::executeCalcAnimList();
        CategoryList::execute(MR::CalcAnimType_AnimParticleIgnorePause);
        SceneFunction::executeCalcViewAndEntryList();
    }

    void SceneExecutionService::draw_3d_normal(const smgpc::camera::CameraPose &camera_pose) {
        auto &scheduler = _runtime.scheduler();
        // SceneExecutor loads player light and runs DrawType_Player after the
        // normal opaque lists but before the normal translucent lists.
        scheduler.execute_draw_buffer_list_normal(
            camera_pose, MR::isExistPriorDrawAir(), MR::DrawType_Player, MR::LightType_Player);
        scheduler.execute_draw_type(MR::DrawType_EffectDraw3D);
        scheduler.execute_draw_type(MR::DrawType_EffectDrawForBloomEffect);
        scheduler.execute_draw_type(MR::DrawType_CenterScreenBlur);
        scheduler.execute_draw_type(MR::DrawType_CaptureScreenIndirect);
        scheduler.execute_draw_after_indirect(camera_pose);
        scheduler.execute_draw_type(MR::DrawType_0x33);
        MR::setStarPointerDrawSyncToken();
        MR::setTalkDirectorDrawSyncToken();
    }

    void SceneExecutionService::draw_2d_normal() {
        _runtime.scheduler().execute_draw_list_2d_normal();
    }

}  // namespace smgpc::scene
