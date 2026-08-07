#include "Game/Util/DemoUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/DemoSceneRuntime.hpp"

#include <optional>
#include <stdexcept>
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
        auto &runtime = smgpc::compat::require_active_demo_scene_runtime(
            "Time-keep demo start");
        if (starter == nullptr || demo_name == nullptr) {
            throw std::invalid_argument(
                "A time-keep demo requires a real starter and demo name.");
        }
        if (require_inactive && runtime.is_active()) {
            return false;
        }

        const auto result = runtime.start_demo(
            starter, demo_name, optional_name(part_name),
            puppetable ? smgpc::compat::DemoPlayerMode::MarioPuppetable
                       : smgpc::compat::DemoPlayerMode::Normal);
        if (!result.has_value() ||
            *result != smgpc::compat::DemoSheetStartResult::Started) {
            return false;
        }
        return true;
    }

    [[nodiscard]] bool start_registered_scene_time_keep_demo(
        LiveActor *starter, const char *part_name, bool puppetable) {
        auto &runtime = smgpc::compat::require_active_demo_scene_runtime(
            "Registered time-keep demo start");
        if (starter == nullptr || runtime.is_active()) {
            return false;
        }

        // DemoExecutor::tryStartProperDemoSystem* selects the puppetable path
        // when its Player keeper parsed at least one row. The explicit Mario
        // overload forces the same path even for an empty Player table.
        puppetable = puppetable ||
                     runtime.registered_demo_has_player_rows(starter);

        const auto result = runtime.start_demo_registered(
            starter, optional_name(part_name),
            puppetable ? smgpc::compat::DemoPlayerMode::MarioPuppetable
                       : smgpc::compat::DemoPlayerMode::Normal);
        if (!result.has_value() ||
            *result != smgpc::compat::DemoSheetStartResult::Started) {
            return false;
        }
        return true;
    }

}  // namespace

namespace smgpc::compat {

    void release_demo_runtime_state(const LiveActor *actor) {
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
        return smgpc::compat::require_active_demo_scene_runtime(
                   "Demo-cast registration")
            .try_register_cast(pActor, rIter);
    }

    bool tryRegisterDemoCast(LiveActor *pActor, const char *pName,
                             const JMapInfoIter &rIter) {
        if (pName == nullptr) {
            throw std::invalid_argument(
                "Named demo-cast registration requires a real demo name.");
        }
        return smgpc::compat::require_active_demo_scene_runtime(
                   "Named demo-cast registration")
            .try_register_cast(pActor, pName, rIter);
    }

    void registerDemoCast(LiveActor *pActor, const char *pName,
                          const JMapInfoIter &rIter) {
        if (!tryRegisterDemoCast(pActor, pName, rIter)) {
            throw std::logic_error(
                "Required demo-cast registration has no matching real group.");
        }
    }

    bool tryRegisterDemoActionFunctor(const LiveActor *pActor,
                                      const MR::FunctorBase &rFunctor,
                                      const char *pActionName) {
        return smgpc::compat::require_active_demo_scene_runtime(
                   "Demo functor registration")
            .try_register_action_functor(pActor, rFunctor,
                                         optional_name(pActionName));
    }

    bool tryRegisterDemoActionFunctorDirect(const LiveActor *pActor,
                                            const MR::FunctorBase &rFunctor,
                                            const char *pDemoName,
                                            const char *pActionName) {
        if (pDemoName == nullptr) {
            throw std::invalid_argument(
                "Direct demo functor registration requires a real demo name.");
        }
        return smgpc::compat::require_active_demo_scene_runtime(
                   "Direct demo functor registration")
            .try_register_action_functor(
                pActor, pDemoName, rFunctor, optional_name(pActionName));
    }

    void registerDemoActionFunctorDirect(const LiveActor *pActor,
                                         const MR::FunctorBase &rFunctor,
                                         const char *pDemoName,
                                         const char *pActionName) {
        if (!tryRegisterDemoActionFunctorDirect(pActor, rFunctor, pDemoName,
                                                pActionName)) {
            throw std::logic_error(
                "Required direct demo functor registration has no matching real Action row.");
        }
    }

    bool tryRegisterDemoActionNerve(const LiveActor *pActor, const Nerve *pNerve,
                                    const char *pActionName) {
        return smgpc::compat::require_active_demo_scene_runtime(
                   "Demo nerve registration")
            .try_register_action_nerve(pActor, pNerve,
                                       optional_name(pActionName));
    }

    void registerDemoActionNerve(const LiveActor *pActor, const Nerve *pNerve,
                                 const char *pActionName) {
        if (!tryRegisterDemoActionNerve(pActor, pNerve, pActionName)) {
            throw std::logic_error(
                "Required demo nerve registration has no matching real Action row.");
        }
    }

    bool isDemoCast(const LiveActor *pActor, const char *pDemoName) {
        auto &runtime = smgpc::compat::require_active_demo_scene_runtime(
            "Demo-cast query");
        return pDemoName != nullptr ? runtime.has_cast(pActor, pDemoName) :
                                      runtime.has_cast(pActor);
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
        if (!start_scene_time_keep_demo(pObj, pDemoName, pPartName, false,
                                        false)) {
            throw std::logic_error("Requested time-keep demo is unavailable.");
        }
    }

    void startTimeKeepDemoMarioPuppetable(NameObj *pObj, const char *pDemoName,
                                          const char *pPartName) {
        if (!start_scene_time_keep_demo(pObj, pDemoName, pPartName, true,
                                        false)) {
            throw std::logic_error("Requested puppetable time-keep demo is unavailable.");
        }
    }

    bool isDemoExist(const char *pDemoName) {
        if (pDemoName == nullptr) {
            throw std::invalid_argument(
                "A demo-existence query requires a real demo name.");
        }
        return smgpc::compat::require_active_demo_scene_runtime(
                   "Demo-existence query")
            .find_definition(pDemoName)
            .has_value();
    }

    bool isTimeKeepDemoActive() {
        return smgpc::compat::require_active_demo_scene_runtime(
                   "Time-keep demo active query")
            .is_time_keep_active();
    }

    bool isDemoActiveRegistered(const LiveActor *pActor) {
        return smgpc::compat::require_active_demo_scene_runtime(
                   "Registered demo active query")
            .is_active_registered(pActor);
    }

    bool isDemoPartExist(const LiveActor *pActor, const char *pPartName) {
        if (pPartName == nullptr) {
            throw std::invalid_argument(
                "A demo-part existence query requires a real part name.");
        }
        return smgpc::compat::require_active_demo_scene_runtime(
                   "Demo-part existence query")
            .part_exists(pActor, pPartName);
    }

    bool isDemoLastStep() {
        return smgpc::compat::require_active_demo_scene_runtime(
                   "Demo last-step query")
            .is_demo_last_step();
    }

    bool isDemoPartActive(const char *pPartName) {
        if (pPartName == nullptr) {
            throw std::invalid_argument(
                "A demo-part active query requires a real part name.");
        }
        return smgpc::compat::require_active_demo_scene_runtime(
                   "Demo-part active query")
            .is_part_active(pPartName);
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
        if (pPartName == nullptr) {
            throw std::invalid_argument(
                "A demo-part total-step query requires a real part name.");
        }
        const auto step = smgpc::compat::require_active_demo_scene_runtime(
                              "Demo-part total-step query")
                              .part_total_step(pPartName);
        if (!step.has_value()) {
            throw std::logic_error(
                "The requested demo part has no real active Time/SubPart row.");
        }
        return *step;
    }

    s32 getDemoPartStep(const char *pPartName) {
        if (pPartName == nullptr) {
            throw std::invalid_argument(
                "A demo-part step query requires a real part name.");
        }
        const auto step = smgpc::compat::require_active_demo_scene_runtime(
                              "Demo-part step query")
                              .part_step(pPartName);
        if (!step.has_value()) {
            throw std::logic_error(
                "The requested demo part has no real active Time/SubPart row.");
        }
        return *step;
    }

    f32 calcDemoPartStepRate(const char *pPartName) {
        const auto total_step = getDemoPartTotalStep(pPartName);
        return static_cast<f32>(getDemoPartStep(pPartName)) / total_step;
    }

    void pauseTimeKeepDemo(LiveActor *pActor) {
        smgpc::compat::require_active_demo_scene_runtime(
            "Time-keep demo pause")
            .pause_time_keep(pActor);
    }

    void resumeTimeKeepDemo(LiveActor *pActor) {
        smgpc::compat::require_active_demo_scene_runtime(
            "Time-keep demo resume")
            .resume_time_keep(pActor);
    }

    const char *getCurrentDemoPartNameMain(const char *pDemoName) {
        if (pDemoName == nullptr) {
            throw std::invalid_argument(
                "A current demo-part query requires a real demo name.");
        }
        const auto part_name =
            smgpc::compat::require_active_demo_scene_runtime(
                "Current demo-part query")
                .current_main_part_name(pDemoName);
        return part_name.has_value() ? part_name->data() : nullptr;
    }

    bool isDemoPartTalk(const char *pPartName) {
        if (pPartName == nullptr) {
            throw std::invalid_argument(
                "A demo-part talk query requires a real part name.");
        }
        return std::string_view(pPartName).find("会話") !=
               std::string_view::npos;
    }

    void timeKeepDemoFadeOut() {
        MR::closeWipeFade(60);
    }

    void timeKeepDemoFadeIn() {
        MR::openWipeFade(60);
    }

}  // namespace MR
