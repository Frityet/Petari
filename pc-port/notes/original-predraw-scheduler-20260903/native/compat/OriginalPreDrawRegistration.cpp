#include "Game/Util/ObjUtil.hpp"
#include "runtime/SceneScheduler.hpp"
#include "runtime/RuntimeContext.hpp"
#include <stdexcept>

namespace MR {
void registerPreDrawFunction(const MR::FunctorBase& functor, int category) {
    if (auto* scheduler = smgpc::runtime::try_active_scene_scheduler()) {
        scheduler->register_pre_draw_function(functor, category);
        return;
    }
    if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
        runtime->scheduler().register_pre_draw_function(functor, category);
        return;
    }
    throw std::logic_error("Pre-draw registration requires an active scene scheduler");
}
}
