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

    [[nodiscard]] bool start_scene_time_keep_demo(NameObj *starter,
                                                  const char *demo_name,
                                                  const char *part_name,
                                                  bool puppetable,
                                                  bool require_inactive) {
        if (demo_name == nullptr || (require_inactive && MR::isDemoActive())) {
            return false;
        }

        auto *runtime = smgpc::compat::active_demo_scene_runtime();
        if (runtime == nullptr) {
            return false;
        }

        const auto result = runtime->start_demo(starter, demo_name,
                                                optional_name(part_name));
        if (!result.has_value() ||
            *result != smgpc::compat::DemoSheetStartResult::Started) {
            return false;
        }

        smgpc::compat::activate_demo_state(starter, demo_name, puppetable);
        return true;
    }

    [[nodiscard]] bool start_registered_scene_time_keep_demo(
        LiveActor *starter, const char *part_name, bool puppetable) {
        if (starter == nullptr || MR::isDemoActive()) {
            return false;
        }

        auto *runtime = smgpc::compat::active_demo_scene_runtime();
        if (runtime == nullptr) {
            return false;
        }

        // DemoExecutor::tryStartProperDemoSystem* selects the puppetable path
        // when its Player keeper parsed at least one row. The explicit Mario
        // overload forces the same path even for an empty Player table.
        puppetable = puppetable ||
                     runtime->registered_demo_has_player_rows(starter);

        const auto result = runtime->start_demo_registered(
            starter, optional_name(part_name));
        if (!result.has_value() ||
            *result != smgpc::compat::DemoSheetStartResult::Started) {
            return false;
        }

        smgpc::compat::activate_demo_state(starter, runtime->active_demo_name(),
                                           puppetable);
        return true;
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

    bool tryStartDemoRegistered(LiveActor *pActor, const char *pPartName) {
        return start_registered_scene_time_keep_demo(pActor, pPartName, false);
    }

    bool tryStartDemoRegisteredMarioPuppetable(LiveActor *pActor,
                                               const char *pPartName) {
        return start_registered_scene_time_keep_demo(pActor, pPartName, true);
    }

    bool tryStartTimeKeepDemo(NameObj *pObj, const char *pDemoName,
                              const char *pPartName) {
        return start_scene_time_keep_demo(pObj, pDemoName, pPartName, false, true);
    }

    bool tryStartTimeKeepDemoMarioPuppetable(NameObj *pObj,
                                             const char *pDemoName,
                                             const char *pPartName) {
        return start_scene_time_keep_demo(pObj, pDemoName, pPartName, true, true);
    }

    bool tryStartTimeKeepDemoMarioPuppetable(LiveActor *pActor,
                                             const char *pDemoName,
                                             const char *pPartName) {
        return tryStartTimeKeepDemoMarioPuppetable(
            static_cast<NameObj *>(pActor), pDemoName, pPartName);
    }

    void startTimeKeepDemo(NameObj *pObj, const char *pDemoName,
                           const char *pPartName) {
        if (smgpc::compat::active_demo_scene_runtime() == nullptr) {
            smgpc::compat::activate_demo_state(
                pObj, pDemoName != nullptr ? pDemoName : "", false);
            return;
        }
        (void)start_scene_time_keep_demo(pObj, pDemoName, pPartName, false,
                                         false);
    }

    void startTimeKeepDemoMarioPuppetable(NameObj *pObj, const char *pDemoName,
                                          const char *pPartName) {
        if (smgpc::compat::active_demo_scene_runtime() == nullptr) {
            smgpc::compat::activate_demo_state(
                pObj, pDemoName != nullptr ? pDemoName : "", true);
            return;
        }
        (void)start_scene_time_keep_demo(pObj, pDemoName, pPartName, true,
                                         false);
    }

    bool isDemoExist(const char *pDemoName) {
        const auto *runtime = smgpc::compat::active_demo_scene_runtime();
        return runtime != nullptr && pDemoName != nullptr &&
               runtime->find_definition(pDemoName).has_value();
    }

    bool isTimeKeepDemoActive() {
        const auto *runtime = smgpc::compat::active_demo_scene_runtime();
        return runtime != nullptr && runtime->is_time_keep_active();
    }

    bool isDemoActiveRegistered(const LiveActor *pActor) {
        const auto *runtime = smgpc::compat::active_demo_scene_runtime();
        return runtime != nullptr && runtime->is_active_registered(pActor);
    }

    bool isDemoPartExist(const LiveActor *pActor, const char *pPartName) {
        const auto *runtime = smgpc::compat::active_demo_scene_runtime();
        return runtime != nullptr && pPartName != nullptr &&
               runtime->part_exists(pActor, pPartName);
    }

    bool isDemoLastStep() {
        const auto *runtime = smgpc::compat::active_demo_scene_runtime();
        return runtime != nullptr && runtime->is_demo_last_step();
    }

    bool isDemoPartActive(const char *pPartName) {
        const auto *runtime = smgpc::compat::active_demo_scene_runtime();
        return runtime != nullptr && pPartName != nullptr &&
               runtime->is_part_active(pPartName);
    }

    bool isDemoPartStep(const char *pPartName, s32 step) {
        return isDemoPartActive(pPartName) && getDemoPartStep(pPartName) == step;
    }

    bool isDemoPartFirstStep(const char *pPartName) {
        return isDemoPartStep(pPartName, 0);
    }

    bool isDemoPartLastStep(const char *pPartName) {
        return isDemoPartActive(pPartName) &&
               getDemoPartStep(pPartName) == getDemoPartTotalStep(pPartName) - 1;
    }

    bool isDemoPartLessEqualStep(const char *pPartName, s32 step) {
        return isDemoPartActive(pPartName) && getDemoPartStep(pPartName) <= step;
    }

    bool isDemoPartGreaterStep(const char *pPartName, s32 step) {
        return isDemoPartActive(pPartName) && getDemoPartStep(pPartName) > step;
    }

    s32 getDemoPartTotalStep(const char *pPartName) {
        const auto *runtime = smgpc::compat::active_demo_scene_runtime();
        if (runtime == nullptr || pPartName == nullptr) {
            return 0;
        }
        return runtime->part_total_step(pPartName).value_or(0);
    }

    s32 getDemoPartStep(const char *pPartName) {
        const auto *runtime = smgpc::compat::active_demo_scene_runtime();
        if (runtime == nullptr || pPartName == nullptr) {
            return -1;
        }
        return runtime->part_step(pPartName).value_or(-1);
    }

    f32 calcDemoPartStepRate(const char *pPartName) {
        const auto total_step = getDemoPartTotalStep(pPartName);
        return total_step != 0 ? static_cast<f32>(getDemoPartStep(pPartName)) / total_step : 0.0f;
    }

    void pauseTimeKeepDemo(LiveActor *pActor) {
        if (auto *runtime = smgpc::compat::active_demo_scene_runtime()) {
            runtime->pause_time_keep(pActor);
        }
    }

    void resumeTimeKeepDemo(LiveActor *pActor) {
        if (auto *runtime = smgpc::compat::active_demo_scene_runtime()) {
            runtime->resume_time_keep(pActor);
        }
    }

    const char *getCurrentDemoPartNameMain(const char *pDemoName) {
        const auto *runtime = smgpc::compat::active_demo_scene_runtime();
        if (runtime == nullptr || pDemoName == nullptr) {
            return nullptr;
        }
        const auto part_name = runtime->current_main_part_name(pDemoName);
        return part_name.has_value() ? part_name->data() : nullptr;
    }

    bool isDemoPartTalk(const char *pPartName) {
        return pPartName != nullptr &&
               std::string_view(pPartName).find("会話") != std::string_view::npos;
    }

    void timeKeepDemoFadeOut() {
        MR::closeWipeFade(60);
    }

    void timeKeepDemoFadeIn() {
        MR::openWipeFade(60);
    }

}  // namespace MR
