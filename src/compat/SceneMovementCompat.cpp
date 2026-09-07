#include <aurora/exception.hpp>
#include "Game/NameObj/NameObjExecuteHolder.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/SceneScheduler.hpp"

#include <stdexcept>

namespace {
    smgpc::runtime::SceneScheduler &require_scheduler() {
        if (auto *scheduler = smgpc::runtime::try_active_scene_scheduler(); scheduler != nullptr) {
            return *scheduler;
        }
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
            return runtime->scheduler();
        }
        aurora::throw_host_exception<std::logic_error>("Scene movement category requests require an active scene scheduler.");
    }
}  // namespace

void CategoryList::requestMovementOn(MR::MovementType type) {
    MR::requestMovementOnWithCategory(type);
}

void CategoryList::requestMovementOff(MR::MovementType type) {
    MR::requestMovementOffWithCategory(type);
}

void CategoryList::execute(MR::DrawType type) {
    // The original NameObjListExecutor draw-category dispatch is owned by
    // the active native scheduler, including nested immediate captures.
    require_scheduler().execute_draw_type(type);
}

void CategoryList::execute(MR::MovementType type) {
    require_scheduler().execute_movement_category(type);
}

void CategoryList::execute(MR::CalcAnimType type) {
    require_scheduler().execute_calc_anim_category(type);
}

void CategoryList::entryDrawBuffer2D() {
    require_scheduler().entry_draw_buffer(MR::CameraType_2D);
}

void CategoryList::entryDrawBuffer3D() {
    require_scheduler().entry_draw_buffer(MR::CameraType_3D);
}

void CategoryList::entryDrawBufferMirror() {
    require_scheduler().entry_draw_buffer(MR::CameraType_Mirror);
}

void CategoryList::drawOpa(MR::DrawBufferType type) {
    require_scheduler().execute_draw_buffer_opa(type);
}

void CategoryList::drawXlu(MR::DrawBufferType type) {
    require_scheduler().execute_draw_buffer_xlu(type);
}
