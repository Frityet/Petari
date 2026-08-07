#include "Game/Util/DemoUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "compat/DemoUtilCompat.hpp"

#include <optional>
#include <string_view>

namespace {

    [[nodiscard]] std::optional<std::string_view> optional_name(const char *name) {
        return name != nullptr ? std::optional<std::string_view>(name) : std::nullopt;
    }

}  // namespace

namespace smgpc::compat {

    void release_demo_runtime_state(const LiveActor *actor) {
        release_active_demo_for_owner(actor);
        release_actor_from_all_demo_scenes(actor);
    }

    bool has_registered_demo_cast(const LiveActor *actor) {
        return has_any_demo_scene_cast(actor);
    }

    std::size_t registered_demo_membership_count(const LiveActor *actor) {
        return demo_scene_membership_count(actor);
    }

    std::size_t registered_demo_action_count(const LiveActor *actor) {
        return demo_scene_action_count(actor);
    }

}  // namespace smgpc::compat

namespace MR {

    bool tryRegisterDemoCast(LiveActor *pActor, const JMapInfoIter &rIter) {
        auto *runtime = smgpc::compat::active_demo_scene_runtime();
        return runtime != nullptr && runtime->try_register_cast(pActor, rIter);
    }

    bool tryRegisterDemoCast(LiveActor *pActor, const char *pName,
                             const JMapInfoIter &rIter) {
        auto *runtime = smgpc::compat::active_demo_scene_runtime();
        return runtime != nullptr && pName != nullptr &&
               runtime->try_register_cast(pActor, pName, rIter);
    }

    void registerDemoCast(LiveActor *pActor, const char *pName,
                          const JMapInfoIter &rIter) {
        (void)tryRegisterDemoCast(pActor, pName, rIter);
    }

    bool tryRegisterDemoActionFunctor(const LiveActor *pActor,
                                      const MR::FunctorBase &rFunctor,
                                      const char *pActionName) {
        auto *runtime = smgpc::compat::active_demo_scene_runtime();
        return runtime != nullptr && runtime->try_register_action_functor(
                                         pActor, rFunctor, optional_name(pActionName));
    }

    bool tryRegisterDemoActionFunctorDirect(const LiveActor *pActor,
                                            const MR::FunctorBase &rFunctor,
                                            const char *pDemoName,
                                            const char *pActionName) {
        auto *runtime = smgpc::compat::active_demo_scene_runtime();
        return runtime != nullptr && pDemoName != nullptr &&
               runtime->try_register_action_functor(
                   pActor, pDemoName, rFunctor, optional_name(pActionName));
    }

    void registerDemoActionFunctorDirect(const LiveActor *pActor,
                                         const MR::FunctorBase &rFunctor,
                                         const char *pDemoName,
                                         const char *pActionName) {
        (void)tryRegisterDemoActionFunctorDirect(pActor, rFunctor, pDemoName, pActionName);
    }

    bool tryRegisterDemoActionNerve(const LiveActor *pActor, const Nerve *pNerve,
                                    const char *pActionName) {
        auto *runtime = smgpc::compat::active_demo_scene_runtime();
        return runtime != nullptr && runtime->try_register_action_nerve(
                                         pActor, pNerve, optional_name(pActionName));
    }

    void registerDemoActionNerve(const LiveActor *pActor, const Nerve *pNerve,
                                 const char *pActionName) {
        (void)tryRegisterDemoActionNerve(pActor, pNerve, pActionName);
    }

    bool isDemoCast(const LiveActor *pActor, const char *pDemoName) {
        auto *runtime = smgpc::compat::active_demo_scene_runtime();
        if (runtime == nullptr) {
            return false;
        }
        return pDemoName != nullptr ? runtime->has_cast(pActor, pDemoName) :
                                      runtime->has_cast(pActor);
    }

    void startTimeKeepDemoMarioPuppetable(NameObj *pObj, const char *pDemoName,
                                          const char *pPartName) {
        auto *actor = dynamic_cast<LiveActor *>(pObj);
        static_cast<void>(pPartName);
        (void)MR::tryStartDemoMarioPuppetable(actor, pDemoName);
    }

    void timeKeepDemoFadeOut() {
        MR::closeWipeFade(60);
    }

    void timeKeepDemoFadeIn() {
        MR::openWipeFade(60);
    }

}  // namespace MR
