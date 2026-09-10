#include "Game/Enemy/AnimScaleController.hpp"
#include "Game/Animation/XanimePlayer.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/NPC/NPCActor.hpp"
#include "Game/NPC/NPCActorItem.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/Player/GroupChecker.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/NPCUtil.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/GroupCheckManagerCompat.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "SceneExecutionFixture.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace MR {
    bool getNPCItemData(NPCActorItem*, s32);
    bool checkPlayerSwingTrigger();
}  // namespace MR

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void requireUnavailable(const std::function<void()>& operation, std::string_view message) {
        auto unavailable = false;
        try {
            operation();
        } catch (const std::logic_error&) {
            unavailable = true;
        }
        require(unavailable, message);
    }

    void requireInvalid(const std::function<void()>& operation, std::string_view message) {
        auto invalid = false;
        try {
            operation();
        } catch (const std::invalid_argument&) {
            invalid = true;
        }
        require(invalid, message);
    }

    void requireNear(float actual, float expected, std::string_view message) {
        require(std::abs(actual - expected) < 0.00001F, message);
    }

    class DirectFloatBaseProbe final : public NPCActor {
    public:
        DirectFloatBaseProbe() : NPCActor("direct float-base probe") {
        }

        void calcAndSetBaseMtx() override {
            ++virtualCalls;
            NPCActor::calcAndSetBaseMtx();
        }

        int virtualCalls = 0;
    };

    class NerveProbe final : public Nerve {
    public:
        void execute(Spine* spine) const override {
            last_spine = spine;
            ++executions;
        }

        void executeOnEnd(Spine* spine) const override {
            last_spine = spine;
            ++end_calls;
        }

        mutable Spine* last_spine = nullptr;
        mutable int executions = 0;
        mutable int end_calls = 0;
    };

    void testOriginalSpineTransitions() {
        const auto base = NerveProbe{};
        const auto reaction = NerveProbe{};
        const auto replacement = NerveProbe{};
        auto actor = NPCActor("original NPC spine transitions");
        actor.initNerve(&base);
        auto* const spine = actor.mSpine;
        require(spine != nullptr && spine->mExecutor == &actor &&
                    actor.isNerve(&base) && actor.isEmptyNerve(),
                "NPC initNerve must own a real Spine with the exact actor and initial nerve");
        spine->update();
        require(base.last_spine == spine && base.executions == 1 && actor.getNerveStep() == 1,
                "the original Spine must execute the initial nerve and advance its step");

        actor.pushNerve(&reaction);
        require(actor.mSpine == spine && actor.mCurNerve == &base &&
                    spine->mCurrNerve == &base && spine->mNextNerve == &reaction &&
                    actor.isNerve(&reaction) && actor.getNerveStep() == -1 && base.end_calls == 1,
                "NPC push must save the original nerve and expose the actual pending transition");

        require(actor.popAndPushNerve(&replacement) == &reaction &&
                    actor.mCurNerve == &base && actor.isNerve(&replacement) &&
                    reaction.executions == 0 && reaction.end_calls == 0,
                "popAndPush must replace a pending nerve while preserving the saved base nerve");
        spine->update();
        require(replacement.last_spine == spine && replacement.executions == 1 &&
                    spine->mCurrNerve == &replacement && spine->mNextNerve == nullptr &&
                    actor.getNerveStep() == 1,
                "replacement execution must use the same Spine and commit its pending nerve");

        require(actor.popNerve() == &replacement && actor.isNerve(&base) &&
                    actor.isEmptyNerve() && replacement.end_calls == 1,
                "NPC pop must return the outgoing nerve, restore the saved nerve and empty its slot");
        spine->update();
        require(base.executions == 2 && actor.getNerveStep() == 1,
                "restoring the base nerve must restart its step through the original Spine");

        require(actor.tryPushNullNerve() && actor.mCurNerve == &base,
                "an empty NPC nerve slot must accept the original null nerve");
        const auto* const null_nerve = spine->getCurrentNerve();
        require(null_nerve != nullptr && null_nerve != &base &&
                    !actor.tryPushNullNerve() && spine->getCurrentNerve() == null_nerve &&
                    actor.mCurNerve == &base,
                "a repeated null push must retain both the null nerve identity and saved base");
        spine->update();
        require(actor.isNerve(null_nerve) && actor.getNerveStep() == 1 &&
                    actor.mSpine == spine && base.executions == 2,
                "the original null nerve must execute without replacing the Spine or running the base");
        require(actor.popNerve() == null_nerve && actor.isNerve(&base) && actor.isEmptyNerve(),
                "popping the original null nerve must restore the exact saved base identity");
        spine->update();
        require(base.executions == 3 && actor.mSpine == spine,
                "the same owned Spine must resume the base after the null nerve");
    }

    void testFloatOffsetAndBaseMatrix() {
        const auto* disc_path = std::getenv("SMGPC_REAL_DISC");
        require(disc_path != nullptr && *disc_path != '\0',
                "the NPC model proof requires SMGPC_REAL_DISC with the real game image");
        require(aurora_dvd_open(disc_path), "the NPC model proof must open the real disc");
        struct DiscClose final {
            ~DiscClose() { aurora_dvd_close(); }
        } disc_close;
        DVDInit();
        auto logger = smgpc::logging::create_default_logger();
        auto window = smgpc::render::AuroraWindow({
            .width = 640,
            .height = 456,
            .title = "SMG PC original NPC model and Spine proof",
        });
        auto renderer = smgpc::render::AuroraRenderer(window);
        const auto renderer_context = smgpc::render::ScopedAuroraRendererContext(renderer);
        auto resource_runtime = smgpc::resource::GameResourceRuntime{};
        auto runtime = smgpc::runtime::RuntimeContext(*logger, window, resource_runtime);
        auto scheduler_binding = smgpc::runtime::SceneSchedulerBinding(runtime.scheduler());
        auto domain = smgpc::compat::JkrAllocationDomain::create(runtime.host_heaps(), 16U << 20);
        auto scene = smgpc::test::SceneExecutionFixture(runtime.scheduler(), domain);
        const auto game_allocations = smgpc::compat::JkrAllocationScope(domain);
        auto actor = DirectFloatBaseProbe{};
        actor.initModelManagerWithAnm("Tico", "Tico", false);
        require(actor.mModelManager != nullptr && actor.mModelManager->getJ3DModel() != nullptr &&
                    actor.mModelManager->mModelResourceHolder != nullptr,
                "the NPC base-matrix proof must retain the actual Tico resource and ModelManager");
        auto* const model = actor.mModelManager->getJ3DModel();
        require(actor.mModelManager->mModel == nullptr && actor.mModelManager->mXanimePlayer != nullptr &&
                    actor.mModelManager->mXanimePlayer->mModel == model &&
                    actor.mModelManager->mModelResourceHolder->mModelResTable->getRes("Tico") == model->mModelData,
                "animated Tico must select the Xanime-owned model and retain its authored model resource identity");
        auto transform = TPos3f{};
        transform.identity();
        transform.setTrans(12.0F, -4.0F, 30.0F);
        actor.setBaseMtx(transform);
        require(actor.mPosition.x == 12.0F && actor.mPosition.y == -4.0F && actor.mPosition.z == 30.0F &&
                    model->mBaseTransformMtx[0][3] == 12.0F && model->mBaseTransformMtx[1][3] == -4.0F &&
                    model->mBaseTransformMtx[2][3] == 30.0F,
                "NPC setBaseMtx must update position and the actual J3D model transform");
        requireNear(MR::calcFloatOffset(&actor, 8.0F, 150.0F), 7.5F,
                    "an NPC float offset must decay by exactly 0.5 per frame");
        requireNear(MR::calcFloatOffset(&actor, 0.25F, 150.0F), 0.0F,
                    "an NPC float offset must clamp a negative decay to zero");
        requireNear(MR::calcFloatOffset(
                        &actor, std::numeric_limits<float>::quiet_NaN(), 150.0F),
                    0.0F,
                    "a NaN float offset must take the retail comparison's zero branch");
        requireInvalid([] { static_cast<void>(MR::calcFloatOffset(nullptr, 0.0F, 1.0F)); },
                       "a null NPC must fail the float-offset host contract explicitly");

        actor.mPosition.set(10.0F, 20.0F, 30.0F);
        actor._A0.set(0.5F, 0.25F, -0.75F, 2.0F);
        const auto original = actor.mPosition;
        auto rawY = TVec3f{};
        actor._A0.getYDir(rawY);
        constexpr auto offset = 3.0F;
        const auto floated = original + rawY * offset;
        const auto virtual_calls_before_float = actor.virtualCalls;
        MR::calcAndSetFloatBaseMtx(&actor, offset);
        const auto& matrix = model->mBaseTransformMtx;
        require(actor.virtualCalls == virtual_calls_before_float,
                "float base calculation must directly dispatch NPCActor::calcAndSetBaseMtx");
        requireNear(actor.mPosition.x, original.x,
                    "float base calculation must restore NPC position X");
        requireNear(actor.mPosition.y, original.y,
                    "float base calculation must restore NPC position Y");
        requireNear(actor.mPosition.z, original.z,
                    "float base calculation must restore NPC position Z");
        requireNear(matrix[0][3], floated.x,
                    "float base matrix must capture raw quaternion-Y offset X");
        requireNear(matrix[1][3], floated.y,
                    "float base matrix must capture raw quaternion-Y offset Y");
        requireNear(matrix[2][3], floated.z,
                    "float base matrix must capture raw quaternion-Y offset Z");
        requireInvalid([] { MR::calcAndSetFloatBaseMtx(nullptr, 1.0F); },
                       "a null NPC must fail the float-base host contract explicitly");

        std::cout << "[proof] original Tico ModelManager owns NPC base and float transforms; talk-height cases use actual Mario fixture\n";
    }
}  // namespace

int main() {
    auto passed = 0;

    auto caps = NPCActorCaps("TestNpc");
    require(!caps.mModel && !caps.mMakeActor && !caps.mLodCtrl && !caps.mShadow,
            "retail NPC caps must begin disabled instead of silently enabling host substitutes");
    caps.setDefault();
    require(caps.mModel && caps.mMakeActor && caps.mLodCtrl && caps.mShadow && caps.mMessage,
            "retail NPC default caps must retain the decompiled defaults");
    ++passed;

    auto actor = NPCActor("npc-reaction-test");
    require(actor.receiveMsgPlayerAttack(ACTMES_PLAYER_TRAMPLE, nullptr, nullptr),
            "the exact NPCActor source must accept the first trample reaction");
    require(!actor.receiveMsgPlayerAttack(ACTMES_PLAYER_TRAMPLE, nullptr, nullptr),
            "the exact NPCActor source must retain its trample cooldown");
    actor.updateReaction();
    require(actor._D8 && !actor._E2, "the exact NPCActor reaction edge must be consumed once");
    actor.updateReaction();
    require(!actor._D8, "the exact NPCActor reaction edge must clear on the next update");
    ++passed;

    const auto manager_baseline = smgpc::compat::group_check_manager_runtime_state_count();
    const auto checker_baseline = smgpc::compat::group_checker_runtime_state_count();
    const auto membership_baseline = smgpc::compat::attribute_group_membership_count();
    {
        auto holder = SceneObjHolder{};
        auto binding = smgpc::scene::SceneObjHolderBinding(holder);

        requireUnavailable([&] { MR::addToAttributeGroupSearchTurtle(&actor); },
                           "attribute membership must not fabricate a missing scene-owned manager");
        require(holder.getObj(SceneObj_GroupCheckManager) == nullptr,
                "a failed membership request must leave the required pre-placement SceneObj absent");
        require(MR::createSceneObj(SceneObj_GroupCheckManager) != nullptr,
                "the focused scene fixture must explicitly create its GroupCheckManager");

        MR::addToAttributeGroupSearchTurtle(&actor);
        MR::addToAttributeGroupSearchTurtle(&actor);
        require(MR::isExistInAttributeGroupSearchTurtle(&actor),
                "SearchTurtle membership must be queryable through the scene-owned GroupCheckManager");
        require(!MR::isExistInAttributeGroupReflectSpinningBox(&actor),
                "SearchTurtle membership must not leak into the ReflectSpinningBox group");
        require(smgpc::compat::attribute_group_membership_count() == membership_baseline + 1U,
                "attribute group insertion must be idempotent for the same NameObj name");

        auto same_name = NPCActor("npc-reaction-test");
        require(MR::isExistInAttributeGroupSearchTurtle(&same_name),
                "GroupChecker lookup must use NameObj names like the retail HashSortTable");

        auto second = NPCActor("npc-second-search-target");
        MR::addToAttributeGroupSearchTurtle(&second);
        require(MR::isExistInAttributeGroupSearchTurtle(&second),
                "multiple actors must coexist in the same SearchTurtle group");

        auto reflected = NPCActor("npc-reflected-target");
        MR::addToAttributeGroupReflectSpinningBox(&reflected);
        require(MR::isExistInAttributeGroupReflectSpinningBox(&reflected) &&
                    !MR::isExistInAttributeGroupSearchTurtle(&reflected),
                "the two retail attribute groups must retain independent membership");

        const auto before_transient = smgpc::compat::attribute_group_membership_count();
        {
            auto transient = NPCActor("npc-transient-search-target");
            MR::addToAttributeGroupSearchTurtle(&transient);
            require(smgpc::compat::attribute_group_membership_count() == before_transient + 1U,
                    "a newly inserted NameObj name must contribute one group membership");
        }
        auto same_transient_name = NPCActor("npc-transient-search-target");
        require(smgpc::compat::attribute_group_membership_count() == before_transient + 1U &&
                    MR::isExistInAttributeGroupSearchTurtle(&same_transient_name),
                "name membership must live with the scene-owned checker, not an individual actor identity");
        require(smgpc::compat::group_check_manager_runtime_state_count() == manager_baseline + 1U &&
                    smgpc::compat::group_checker_runtime_state_count() == checker_baseline + 2U,
                "one scene manager must own exactly the two retail group checkers");
    }
    require(smgpc::compat::group_check_manager_runtime_state_count() == manager_baseline &&
                smgpc::compat::group_checker_runtime_state_count() == checker_baseline &&
                smgpc::compat::attribute_group_membership_count() == membership_baseline,
            "scene teardown must release both group checkers and every remaining membership");
    requireInvalid([] { MR::addToAttributeGroupSearchTurtle(nullptr); },
                   "null attribute-group insertion must remain an explicit contract error");
    ++passed;

    auto item = NPCActorItem("TestNpc");
    requireUnavailable([&] { (void)MR::getNPCItemData(&item, 0); },
                       "missing NPC item-table data must be explicitly unavailable");
    requireUnavailable([] { (void)MR::checkPlayerSwingTrigger(); },
                       "missing real MarioActor swing state must be explicitly unavailable");
    {
        auto parameter = AnimScaleParam{};
        auto controller = AnimScaleController(&parameter);
        controller.update();
        require(controller._C.y == 1.0F, "upstream scale controller must remain at rest until triggered");
    }
    ++passed;

    testOriginalSpineTransitions();
    ++passed;

    testFloatOffsetAndBaseMatrix();
    ++passed;

    std::cout << "NPCActor real-or-absent tests passed: " << passed << "/6\n";
    return 0;
}
