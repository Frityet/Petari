#include "Game/Enemy/AnimScaleController.hpp"
#include "Game/Animation/XanimePlayer.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/NPC/NPCActor.hpp"
#include "Game/NPC/NPCActorItem.hpp"
#include "Game/NPC/NPCDirector.hpp"
#include "Game/NPC/NPCParameter.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/LiveActor/RailRider.hpp"
#include "Game/NPC/TalkMessageCtrl.hpp"
#include "Game/NPC/TalkNodeCtrl.hpp"
#include "Game/Player/GroupChecker.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/NPCUtil.hpp"
#include "Game/Util/JointController.hpp"
#include "Game/Util/RailUtil.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/GroupCheckManagerCompat.hpp"
#include "runtime/RuntimeContext.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/JMapResource.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "SceneExecutionFixture.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <cmath>
#include <array>
#include <bit>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace MR {
    bool getNPCItemData(NPCActorItem*, s32);
    bool checkPlayerSwingTrigger();
}  // namespace MR

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            const auto host_allocations = smgpc::compat::JkrHostAllocationScope{};
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

    void write_be32(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint32_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 24U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 16U);
        bytes[offset + 2U] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 3U] = static_cast<std::uint8_t>(value);
    }

    void write_be16(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint16_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value);
    }

    void write_be_float(std::vector<std::uint8_t> &bytes, std::size_t offset, float value) {
        write_be32(bytes, offset, std::bit_cast<std::uint32_t>(value));
    }

    void write_bcsv_field(std::vector<std::uint8_t> &bytes, std::size_t index, std::string_view name, std::uint16_t offset,
                          smgpc::resource::BcsvFieldType type) {
        const auto field_offset = 0x10U + index * 0x0cU;
        write_be32(bytes, field_offset, smgpc::resource::jmap_hash(name));
        write_be32(bytes, field_offset + 0x04U, 0xffffffffU);
        write_be16(bytes, field_offset + 0x08U, offset);
        bytes[field_offset + 0x0aU] = 0U;
        bytes[field_offset + 0x0bU] = static_cast<std::uint8_t>(type);
    }

    JMapInfo make_fieldless_jmap(std::uint32_t entry_count) {
        auto bytes = std::vector<std::uint8_t>(0x10U, 0U);
        write_be32(bytes, 0x00U, entry_count);
        write_be32(bytes, 0x08U, 0x10U);
        return JMapInfo::from_bcsv(bytes);
    }

    std::vector<std::uint8_t> make_npc_item_table(bool sparse) {
        // Deliberately shuffle the columns: NPCParameterReader must resolve
        // the authored field hashes, independently of their physical order.
        constexpr auto names = std::array<std::string_view, 4>{
            "mGoods1", "mGoodsJoint0", "mGoods0", "mGoodsJoint1"};
        constexpr auto values = std::array<std::array<std::string_view, 4>, 2>{
            std::array<std::string_view, 4>{"Lantern", "HandL", "AuthoredNpcItemNameLongEnoughToRequireRetainedStorage", "HandR"},
            std::array<std::string_view, 4>{"Book", "Head", "Glasses", "Spine"}};
        const auto fields = sparse ? 1U : 4U;
        const auto rows = sparse ? 1U : 2U;
        const auto data_offset = 0x10U + fields * 0x0cU;
        const auto entry_size = fields * 4U;
        const auto string_offset = data_offset + rows * entry_size;
        auto bytes = std::vector<std::uint8_t>(string_offset, 0U);
        write_be32(bytes, 0x00U, rows);
        write_be32(bytes, 0x04U, fields);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);
        for (auto field = 0U; field < fields; ++field) {
            write_bcsv_field(bytes, field, names[field], field * 4U,
                             smgpc::resource::BcsvFieldType::StringOffset);
        }
        for (auto row = 0U; row < rows; ++row) {
            for (auto field = 0U; field < fields; ++field) {
                write_be32(bytes, data_offset + row * entry_size + field * 4U,
                           static_cast<std::uint32_t>(bytes.size() - string_offset));
                const auto value = values[row][field];
                bytes.insert(bytes.end(), value.begin(), value.end());
                bytes.push_back(0U);
            }
        }
        return bytes;
    }

    void testOriginalNPCItemData() {
        auto* director = dynamic_cast<NPCDirector*>(MR::createSceneObj(SceneObj_NPCDirector));
        require(director != nullptr && director->mDataResourceHolder != nullptr &&
                    director->mItemParameterReader != nullptr,
                "NPC item lookup requires the actual scene-owned director, NPCData archive and original reader");
        auto table = smgpc::resource::JMapResource(make_npc_item_table(false));
        auto sparse = smgpc::resource::JMapResource(make_npc_item_table(true));
        auto resources = ResTable{};
        resources.newFileInfoTable(3);
        resources.add("TestNpcItem.bcsv", const_cast<void*>(table.data()), false);
        resources.add("SparseNpcItem.bcsv", const_cast<void*>(sparse.data()), false);
        resources.add("NullNpcItem.bcsv", nullptr, false);
        struct RestoreResources {
            ResourceHolder& holder;
            ResTable* original;
            ResTable& fixture;
            NPCItemParameterReader& reader;
            NPCActorItem original_item;
            ~RestoreResources() {
                holder.mFileInfoTable = original;
                reader.copy(&original_item);
                for (u32 index = 0; index < fixture.mCount; ++index) {
                    delete[] fixture.mFileInfoTable[index].mName;
                }
                delete[] fixture.mFileInfoTable;
            }
        } restore{*director->mDataResourceHolder, director->mDataResourceHolder->mFileInfoTable,
                  resources, *director->mItemParameterReader, director->mItemParameterReader->mItem};
        director->mDataResourceHolder->mFileInfoTable = &resources;

        auto item = NPCActorItem("TestNpc");
        require(MR::getNPCItemData(&item, 0) && std::string_view(item.mActor) == "TestNpc" &&
                    std::string_view(item.mGoods0) == "AuthoredNpcItemNameLongEnoughToRequireRetainedStorage" &&
                    std::string_view(item.mGoods1) == "Lantern" &&
                    std::string_view(item.mGoodsJoint0) == "HandL" &&
                    std::string_view(item.mGoodsJoint1) == "HandR",
                "original getter must resolve the actor table and all four named item fields");
        const auto* retained_name = item.mGoods0;
        require(MR::getNPCItemData(&item, 1) && std::string_view(item.mGoods0) == "Glasses" &&
                    std::string_view(item.mGoods1) == "Book" &&
                    std::string_view(item.mGoodsJoint0) == "Head" &&
                    std::string_view(item.mGoodsJoint1) == "Spine" &&
                    std::string_view(retained_name) == "AuthoredNpcItemNameLongEnoughToRequireRetainedStorage",
                "row selection must use the original reader while prior borrowed strings outlive its local JMapInfo");
        for (const auto row : {-1, 2}) {
            const auto old_goods = item.mGoods0;
            require(MR::getNPCItemData(&item, row) && item.mGoods0 == old_goods,
                    "an existing NPC table returns true and preserves input for an out-of-range row");
        }
        item.mActor = "SparseNpc";
        const auto old_goods = item.mGoods0;
        const auto old_joint = item.mGoodsJoint0;
        require(MR::getNPCItemData(&item, 0) && item.mGoods0 == old_goods &&
                    item.mGoodsJoint0 == old_joint && std::string_view(item.mGoods1) == "Lantern",
                "missing named columns must preserve caller values rather than retain another actor's reader state");
        item.mActor = "MissingNpc";
        require(!MR::getNPCItemData(&item, 0) && item.mGoods0 == old_goods,
                "a missing actor item table must return false without changing the item");
        item.mActor = "NullNpc";
        require(MR::getNPCItemData(&item, 0) && item.mGoods0 == old_goods,
                "a present null table must preserve retail attach-false behavior and leave copied defaults intact");
        std::cout << "[proof] original NPC item table lookup, shuffled fields, rows, sparse/missing/null resources and retained strings\n";
    }

    JMapInfo make_open_rail_path_info() {
        constexpr auto data_offset = 0x1cU;
        constexpr auto entry_size = 8U;
        auto bytes = std::vector<std::uint8_t>(data_offset + entry_size, 0U);
        write_be32(bytes, 0x00U, 1U);
        write_be32(bytes, 0x04U, 1U);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);
        write_bcsv_field(bytes, 0U, "closed", 0U, smgpc::resource::BcsvFieldType::InlineString);
        bytes[data_offset + 0U] = 'O';
        bytes[data_offset + 1U] = 'P';
        bytes[data_offset + 2U] = 'E';
        bytes[data_offset + 3U] = 'N';
        return JMapInfo::from_bcsv(bytes);
    }

    JMapInfo make_linear_rail_point_info(std::uint32_t entry_count = 3U) {
        constexpr auto field_count = 10U;
        constexpr auto entry_size = field_count * 4U;
        constexpr auto data_offset = 0x10U + field_count * 0x0cU;
        constexpr auto field_names = std::array<std::string_view, field_count>{
            "pnt0_x", "pnt0_y", "pnt0_z", "pnt1_x", "pnt1_y", "pnt1_z", "pnt2_x", "pnt2_y", "pnt2_z", "id",
        };

        auto bytes = std::vector<std::uint8_t>(data_offset + entry_count * entry_size, 0U);
        write_be32(bytes, 0x00U, entry_count);
        write_be32(bytes, 0x04U, field_count);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);
        for (auto field = 0U; field < field_count; ++field) {
            const auto type = field + 1U == field_count ? smgpc::resource::BcsvFieldType::Int32 : smgpc::resource::BcsvFieldType::Float;
            write_bcsv_field(bytes, field, field_names[field], static_cast<std::uint16_t>(field * 4U), type);
        }

        for (auto entry = 0U; entry < entry_count; ++entry) {
            const auto entry_offset = data_offset + entry * entry_size;
            const auto x = static_cast<float>(entry * 10U);
            write_be_float(bytes, entry_offset + 0U * 4U, x);
            write_be_float(bytes, entry_offset + 3U * 4U, x);
            write_be_float(bytes, entry_offset + 6U * 4U, x);
            write_be32(bytes, entry_offset + 9U * 4U, entry);
        }
        return JMapInfo::from_bcsv(bytes);
    }

    std::array<const char*, 4> npcMotionNames(const NPCActor& actor) {
        const auto* table = actor.mModelManager->mModelResourceHolder->mMotionResTable;
        require(table != nullptr && table->mCount >= 4,
                "action decisions require four distinct authored Tico BCK resources");
        std::array<const char*, 4> names{};
        for (std::size_t index = 0; index < names.size(); ++index) {
            names[index] = table->getResName(static_cast<u32>(index));
            require(names[index] != nullptr && MR::isExistBck(&actor, names[index]),
                    "every selected action must be backed by the real Tico animation archive");
        }
        return names;
    }

    void clearReactionEdges(NPCActor& actor) {
        actor._DD = actor._DE = actor._DF = actor._E0 = 0;
        actor._E2 = actor._E3 = actor._E4 = actor._E5 = 0;
    }

    void testOriginalReactionActions(NPCActor& actor) {
        const auto names = npcMotionNames(actor);
        const auto base = NerveProbe{};
        const auto reaction = NerveProbe{};
        auto scale = AnimScaleController(nullptr);
        auto delegator = std::unique_ptr<JointControlDelegator<NPCActor>>(
            MR::createJointDelegatorWithNullChildFunc(&actor, &NPCActor::calcJointScale, "Body"));
        struct RestoreLinks {
            NPCActor& actor;
            ~RestoreLinks() {
                actor.mScaleController = nullptr;
                actor.mDelegator = nullptr;
                actor.setNerve(actor.mWaitNerve);
                actor.mCurNerve = nullptr;
            }
        } restore{actor};
        actor.initNerve(&base);
        actor.mSpine->update();
        actor._128 = 1;
        actor._134 = names[0];
        actor._13C = names[1];
        actor._130 = names[2];
        actor._138 = names[3];
        clearReactionEdges(actor);
        MR::startAction(&actor, names[3]);

        actor._E2 = actor._E3 = actor._E4 = actor._E5 = 1;
        require(MR::tryStartReaction(&actor) && MR::isActionStart(&actor, names[0]),
                "new trample must win over hit, spin and pointing using the actual animation player");
        actor._E2 = 0;
        require(MR::tryStartReaction(&actor) && MR::isActionStart(&actor, names[1]),
                "new hit must win over spin and pointing");
        actor._E5 = 0;
        require(MR::tryStartReaction(&actor) && MR::isActionStart(&actor, names[2]),
                "new spin must win over pointing");
        actor._E3 = 0;
        require(!MR::tryStartReaction(&actor) && MR::isActionStart(&actor, names[2]),
                "pointing must not interrupt an active spin action");

        // Use a valid different animation while excluding the three higher-priority names.
        actor._134 = actor._13C = actor._130 = nullptr;
        require(MR::tryStartReaction(&actor) && MR::isActionStart(&actor, names[3]),
                "a fresh pointing edge must start its real action when no higher-priority action is active");
        actor._DF = 1;
        auto* control = MR::getBckCtrl(&actor);
        require(control == &actor.mModelManager->mXanimePlayer->_24[actor.mModelManager->mXanimePlayer->_54],
                "completion predicates must inspect the actual active Xanime frame controller");
        control->setAttribute(0);
        control->mState = 0;
        require(!MR::tryStartReaction(&actor), "ongoing pointing must not report a fresh edge");
        control->mState = 1;
        require(!MR::tryStartReaction(&actor) && MR::isActionStart(&actor, names[3]),
                "restarting completed pointing retains the original false new-reaction result");

        clearReactionEdges(actor);
        actor._128 = 0;
        actor._E2 = 1;
        require(!MR::tryStartReaction(&actor),
                "disabled animation reactions need both actual scale objects before accepting an edge");
        actor.mScaleController = &scale;
        require(!MR::tryStartReaction(&actor), "a scale controller alone must not fabricate a joint delegator");
        actor.mDelegator = delegator.get();
        require(MR::tryStartReaction(&actor) && MR::isActionStart(&actor, names[3]),
                "the actual scale-controller/delegator pair accepts the edge without changing animation");

        require(MR::tryStartReactionAndPushNerve(&actor, &reaction) && actor.mCurNerve == &base &&
                    actor.isNerve(&reaction) && base.end_calls == 1,
                "a real reaction must push the exact nerve while preserving the base Spine state");
        actor.mSpine->update();
        require(reaction.executions == 1 && actor.getNerveStep() == 1,
                "the original Spine executes the pushed reaction");
        require(!MR::tryStartReactionAndPopNerve(&actor) && actor.isNerve(&reaction) &&
                    actor.mCurNerve == &base && actor.getNerveStep() == -1,
                "another edge must restart the reaction through pop-and-push without leaving it");
        actor.mSpine->update();
        require(reaction.executions == 2 && actor.getNerveStep() == 1,
                "restarted reaction executes at step zero again");
        clearReactionEdges(actor);
        scale._C.set(1.5F, 1.0F, 1.0F);
        require(!MR::tryStartReactionAndPopNerve(&actor) && actor.isNerve(&reaction),
                "actual scale deformation must keep the reaction nerve active");
        scale.resetScale();
        control = MR::getBckCtrl(&actor);
        control->setAttribute(0);
        control->mState = 2;
        require(!MR::isActionLoopedOrStopped(&actor) && !MR::tryStartReactionAndPopNerve(&actor),
                "one-time mode must ignore the loop bit and wait for stop");
        control->setAttribute(2);
        control->mState = 1;
        require(!MR::isActionLoopedOrStopped(&actor) && !MR::tryStartReactionAndPopNerve(&actor),
                "loop mode must ignore the stop bit and wait for loop completion");
        control->mState = 2;
        require(MR::isActionLoopedOrStopped(&actor) && MR::tryStartReactionAndPopNerve(&actor) &&
                    actor.isNerve(&base) && actor.isEmptyNerve(),
                "loop completion must restore the exact saved base nerve");
        actor.mSpine->update();
        require(base.executions == 2, "the restored base resumes through the same original Spine");
        std::cout << "[proof] original NPC reaction priority, actual BCK state, scale ownership and push/pop outcomes\n";
    }

    void testOriginalMoveAndTalkActions(NPCActor& actor) {
        const auto names = npcMotionNames(actor);
        requireUnavailable([&] { (void)MR::tryStartTurnAction(&actor); },
                           "turn-to-player must reject the absent real Mario owner instead of inventing a player");
        auto talk = TalkMessageCtrl(&actor, TVec3f{}, nullptr);
        struct RestoreTalk {
            NPCActor& actor;
            ~RestoreTalk() { actor.mMsgCtrl = nullptr; }
        } restore{actor};
        actor.mMsgCtrl = &talk;
        require(talk.mNodeCtrl != nullptr, "talk action selection needs its actual registered TalkNodeCtrl");
        talk._18 = 3;
        talk.mNodeCtrl->mMessageInfo.mTalkType = 0;
        actor.mParam._1 = 0;
        actor.mParam._1C = names[0];
        actor.mParam._20 = names[1];
        MR::startAction(&actor, names[2]);
        require(MR::tryStartTalkAction(&actor) && MR::isActionStart(&actor, names[0]) &&
                    !MR::tryStartTalkAction(&actor),
                "active non-turning talk starts its authored action once");
        actor.mParam._1C = "";
        require(!MR::tryStartTalkAction(&actor) && MR::isActionStart(&actor, names[0]),
                "an empty talk action must leave the actual animation unchanged");
        actor.mParam._1C = names[1];
        require(!MR::isExistRail(&actor) && MR::tryStartMoveTalkAction(&actor) &&
                    MR::isActionStart(&actor, names[1]),
                "without a rail, move-talk must delegate to the original stationary talk decision");

        auto placement = make_fieldless_jmap(1);
        placement.setRailInfo(0, make_open_rail_path_info(), make_linear_rail_point_info(), 0);
        actor.initRailRider(JMapInfoIter(&placement, 0));
        require(actor.mRailRider != nullptr && actor.mRailRider->mBezierRail != nullptr,
                "movement actions require the actual original RailRider and BezierRail");
        actor.mPosition.zero();
        actor.mGravity.set(0.0F, -1.0F, 0.0F);
        actor._A0.set(0.0F, 0.0F, 0.0F, 1.0F);
        actor._10C = 2.0F;
        actor._110 = 0.5F;
        actor._114 = 1.0F;
        actor._124 = 0;
        MR::setRailCoordSpeed(&actor, 0.0F);
        MR::startMoveAction(&actor);
        requireNear(MR::getRailCoordSpeed(&actor), 0.5F, "original rail speed approaches its target by the configured rate");
        requireNear(MR::getRailCoord(&actor), 0.5F, "rail advancement must use the adjusted speed in the same call");
        requireNear(actor.mPosition.x, 0.5F, "the original pose helper follows the advanced rail position");
        auto front = TVec3f{};
        actor._A0.getZDir(front);
        require(front.epsilonEquals(TVec3f{1.0F, 0.0F, 0.0F}, 0.0001F),
                "rail pose must orient the actual NPC quaternion along the path");
        MR::setRailCoord(&actor, 19.75F);
        MR::setRailCoordSpeed(&actor, 2.0F);
        MR::startMoveAction(&actor);
        requireNear(MR::getRailCoord(&actor), 20.0F, "open-rail motion must reach its original end coordinate");
        require(!MR::isRailGoingToEnd(&actor), "reaching the original goal reverses the real RailRider direction");

        actor._11C = names[2];
        actor._120 = names[3];
        actor._118 = 0.375F;
        const auto before_long_talk = MR::getRailCoord(&actor);
        actor.mParam._1C = names[0];
        require(MR::tryStartMoveTalkAction(&actor) && MR::isActionStart(&actor, names[0]),
                "long talk selects the stationary talk action even when a rail exists");
        requireNear(MR::getRailCoord(&actor), before_long_talk, "long talk must not advance the rail");
        requireNear(MR::getBckCtrl(&actor)->getRate(), 1.0F, "stationary talk restores the normal animation rate");
        talk.mNodeCtrl->mMessageInfo.mTalkType = 1;
        require(MR::tryStartMoveTalkAction(&actor) && MR::isActionStart(&actor, names[3]),
                "short talk must retain movement and select its move-talk action");
        require(MR::getRailCoord(&actor) < before_long_talk, "short talk continues along the reversed rail");
        requireNear(MR::getBckCtrl(&actor)->getRate(), 0.375F, "short moving talk applies the configured animation rate");
        MR::setBckRate(&actor, 1.0F);
        require(!MR::tryStartMoveTalkAction(&actor), "retaining the same moving-talk action is not a new start");
        requireNear(MR::getBckCtrl(&actor)->getRate(), 0.375F, "moving-talk rate is reapplied even when its action did not change");
        talk._18 = 0;
        require(MR::tryStartMoveTalkAction(&actor) && MR::isActionStart(&actor, names[2]),
                "ending talk restores the original rail move action");
        requireNear(MR::getBckCtrl(&actor)->getRate(), 1.0F, "ordinary moving action restores the normal animation rate");
        std::cout << "[proof] original NPC stationary/long/short talk decisions and real rail speed, pose and reversal\n";
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
        testOriginalNPCItemData();
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
        testOriginalReactionActions(actor);
        testOriginalMoveAndTalkActions(actor);
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
