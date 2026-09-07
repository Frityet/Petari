#include "compat/MetrowerksStdCompat.hpp"

#include "Game/Animation/XanimeCore.hpp"
#include "Game/Animation/XanimePlayer.hpp"
#include "Game/Animation/XanimeResource.hpp"
#include "Game/Util/HashUtil.hpp"
#include "JSystem/J3DGraphAnimator/J3DModel.hpp"

#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) throw std::runtime_error(std::string(message));
    }

    void near(float actual, float expected, std::string_view message) {
        if (!std::isfinite(actual) || std::fabs(actual - expected) > 0.0002F) {
            throw std::runtime_error(std::string(message) + "; actual=" + std::to_string(actual) +
                                     "; expected=" + std::to_string(expected));
        }
    }

    struct Globals {
        J3DSys system = j3dSys;
        Mtx matrix;
        Vec scale = J3DSys::mCurrentS;
        Vec parent_scale = J3DSys::mParentS;
        J3DJoint* joint = J3DMtxCalc::getJoint();
        J3DMtxBuffer* buffer = J3DMtxCalc::getMtxBuffer();
        J3DMtxCalc* calculator = J3DJoint::mCurrentMtxCalc;
        Globals() { std::memcpy(matrix, J3DSys::mCurrentMtx, sizeof(Mtx)); }
        ~Globals() {
            j3dSys = system;
            std::memcpy(J3DSys::mCurrentMtx, matrix, sizeof(Mtx));
            J3DSys::mCurrentS = scale;
            J3DSys::mParentS = parent_scale;
            J3DMtxCalc::setJoint(joint);
            J3DMtxCalc::setMtxBuffer(buffer);
            J3DJoint::mCurrentMtxCalc = calculator;
        }
    };

    // Real joint-only model data and an actual J3DModel constructor. This does
    // not represent a partially decoded material/shape archive as a full BMD.
    struct Model {
        J3DModelData data;
        std::array<J3DJoint, 2> joints;
        std::array<J3DJoint*, 2> pointers{&joints[0], &joints[1]};
        std::array<u8, 2> draw_flags{};
        std::array<u16, 2> draw_indices{0, 1};
        J3DMtxCalcNoAnm<J3DMtxCalcCalcTransformBasic, J3DMtxCalcJ3DSysInitBasic> basic;
        std::unique_ptr<J3DModel> object;

        Model() {
            joints[0].mJntNo = 0;
            joints[0].mChild = &joints[1];
            joints[0].mTransformInfo.mTranslate = {7, 8, 9};
            joints[1].mJntNo = 1;
            joints[1].mTransformInfo.mTranslate = {0, 3, 0};
            auto& tree = data.getJointTree();
            tree.mJointNum = 2;
            tree.mJointNodePointer = pointers.data();
            tree.mRootNode = &joints[0];
            tree.mBasicMtxCalc = &basic;
            tree.mDrawMtxData.mEntryNum = 2;
            tree.mDrawMtxData.mDrawFullWgtMtxNum = 2;
            tree.mDrawMtxData.mDrawMtxFlag = draw_flags.data();
            tree.mDrawMtxData.mDrawMtxIndex = draw_indices.data();
            object = std::make_unique<J3DModel>(&data, 0, 1);
        }

        ~Model() {
            // The original destructor does not own arena allocations.
            auto* buffer = object->getMtxBuffer();
            for (auto bank = 0; bank < 2; ++bank) {
                ::operator delete[](buffer->mpDrawMtxArr[bank][0], 0x20);
                ::operator delete[](buffer->mpNrmMtxArr[bank][0], 0x20);
                delete[] buffer->mpDrawMtxArr[bank];
                delete[] buffer->mpNrmMtxArr[bank];
            }
            delete[] buffer->mpScaleFlagArr;
            delete[] buffer->mpAnmMtx;
            delete buffer;
        }
    };

    // Owned native BCK tables sampled by the actual J3DAnmTransformKey class.
    // Root translation is linear; the child has an independent constant offset.
    struct Clip {
        J3DAnmTransformKey animation;
        std::array<J3DAnmTransformKeyTable, 6> tables{};
        std::array<float, 1> scales{1};
        std::array<s16, 1> rotations{};
        std::array<float, 8> translations{};

        Clip(s16 length, float origin, float slope, float child_y) {
            for (auto& table : tables) {
                table.mScaleInfo = {1, 0, 0};
                table.mRotationInfo = {1, 0, 0};
                table.mTranslateInfo = {1, 6, 0};
            }
            translations = {0, origin, slope, static_cast<float>(length), origin + slope * length, slope, 0, child_y};
            tables[0].mTranslateInfo = {2, 0, 0};
            tables[4].mTranslateInfo = {1, 7, 0};
            animation.mAttribute = J3DFrameCtrl::EMode_LOOP;
            animation.mFrameMax = length;
            animation.field_0x1e = 2;
            animation.mAnmTable = tables.data();
            animation.mScaleData = scales.data();
            animation.mRotData = rotations.data();
            animation.mTransData = translations.data();
        }
    };

    struct Groups {
        // The original constructor accepts a ResourceHolder pointer without
        // reading it. This fixture exercises group APIs only: no file-name or
        // archive lookup is attempted through the deliberately absent holder.
        XanimeResourceTable table{nullptr};
        std::array<XanimeGroupInfo, 3> groups;
        std::unique_ptr<HashSortTable> sort;
        std::unique_ptr<u32[]> hashes;
        std::unique_ptr<u32[]> indices;
        std::unique_ptr<u16[]> starts;
        std::unique_ptr<u16[]> counts;

        Groups(Clip& first, Clip& second) {
            constexpr std::array names{"Cycle", "Blend", "Once"};
            for (std::size_t index = 0; index < groups.size(); ++index) {
                auto& group = groups[index];
                group.init();
                group.mParent.mAnimationName = names[index];
                group.mHash = MR::getHashCode(names[index]);
                group.mBckName = "FixtureFirst";
                group.mRate = 0.5F;
                group._8 = 4;
                group.mStart = 2;
                group.mEnd = 8;
                group.mLoop = 2;
                group.mAttribute = J3DFrameCtrl::EMode_LOOP;
                group.mBckTableVariant = 1;
                group._20[0] = &first.animation;
                group.mWeights[0] = 1;
            }
            groups[1].mBckTableVariant = 2;
            groups[1]._20[1] = &second.animation;
            groups[1].mWeights[0] = 0.25F;
            groups[1].mWeights[1] = 0.75F;
            groups[2].mRate = 2;
            groups[2]._8 = 2;
            groups[2].mStart = 0;
            groups[2].mEnd = 4;
            groups[2].mLoop = 0;
            groups[2].mAttribute = J3DFrameCtrl::EMode_NONE;
            table._0 = 1;
            table.mMaxGroupInfoTableSize = 2;
            table.mAmountOfGroupInfos = static_cast<u32>(groups.size());
            table.mGroupInfos = groups.data();
            table.mSimpleGroupInfos = nullptr;
            table.createSortTable();
            sort.reset(table.mSortTable);
            hashes.reset(sort->mHashCodes);
            indices.reset(sort->_8);
            starts.reset(sort->_C);
            counts.reset(sort->_10);
        }
    };

    struct Player {
        XanimePlayer object;
        std::unique_ptr<XanimeCore> core;
        std::unique_ptr<XjointInfo[]> joints;
        std::unique_ptr<XanimeTrack[]> tracks;
        std::unique_ptr<XjointTransform[]> transforms;
        std::unique_ptr<XanimeGroupInfo> simple;

        Player(Model& model, Groups& groups)
            : object(model.object.get(), &groups.table), core(object.mCore), joints(core->mJointList), tracks(core->mTrackList) {}

        Player(Model& model, Groups& groups, Player& other)
            : object(model.object.get(), &groups.table, &other.object), core(object.mCore), tracks(core->mTrackList) {}

        void duplicate_simple_group() {
            object.duplicateSimpleGroup();
            simple.reset(object.mSimpleGroup);
        }

        void enable_transforms(Model& model) {
            core->enableJointTransform(&model.data);
            transforms.reset(core->mTransformList);
        }
    };

    struct Fixture {
        Globals globals;
        Model model;
        Clip first{10, 0, 10, 3};
        Clip second{20, 100, 2, 7};
        Groups groups{first, second};
        Player player{model, groups};

        void calculate() {
            player.object.calcAnm(0);
            model.object->calc();
        }
    };

    void construction_and_shared_storage() {
        Fixture fixture;
        auto& player = fixture.player.object;
        require(player.mModel == fixture.model.object.get() && player.mModelData == &fixture.model.data &&
                    player.mResourceTable == &fixture.groups.table && player.mCore->mTrackCount == 2 &&
                    player.mCore->mJointCount == 2 && player.mCore->mTransformList == nullptr,
                "Original constructor must retain actual model/resource objects and allocate the declared core");
        require(!player._7C && player._7E && player.mCurrentAnimation == nullptr && player.mPrevAnimation == nullptr &&
                    player._20 == &player._24[0] && player._54 == 0 && player._55 == 0,
                "Initial playback and both original frame-control slots must remain inactive");
        require(std::string_view(player.getCurrentAnimationName()) == "NULL" &&
                    std::string_view(player.getDefaultAnimationName()) == "NULL" && !player.isRun("Cycle"),
                "Unstarted player queries must use the original null-name and inactive semantics");
        near(player.mCore->mJointList[0]._28._C.x, 7, "Core initT must import the actual bind-pose translation");
        Player shared(fixture.model, fixture.groups, fixture.player);
        require(shared.core->mJointList == player.mCore->mJointList && shared.core->mTrackList != player.mCore->mTrackList,
                "Shared-player construction must share joint history and independently own tracks");
        require(shared.object._20 != player._20 && shared.object.mCore != player.mCore,
                "Shared players must retain independent playback clocks and core objects");
    }

    void authored_rate_phase_and_queries() {
        Fixture fixture;
        auto& player = fixture.player.object;
        player.changeAnimation("Cycle");
        require(player.isRun("Cycle") && !player.isRun("Blend") && player.mCurrentAnimation == &fixture.groups.groups[0] &&
                    player._54 == 1 && player._55 == 1 && player._7C,
                "Group selection must use original table identity and switch frame-control slots");
        near(player.getRate(), 0.5F, "Authored playback rate must reach the actual frame controller");
        near(player._20->getFrame(), 2.5F, "Initial group preparation must advance one authored-rate step");
        require(player._88 && player.checkPass(2.25F), "Post-movement pass query must inspect the preceding frame interval");
        near(player._20->getFrame(), 2.5F, "Pass queries must restore the live controller frame");
        player.updateAfterMovement();
        near(player._20->getFrame(), 2.5F, "Repeated movement update before calculation must not advance the same phase twice");
        fixture.calculate();
        near(fixture.model.object->getAnmMtx(0)[0][3], 25, "Actual model calculation must sample the authored BCK frame");
        near(fixture.model.object->getAnmMtx(1)[1][3], 3, "Child joint calculation must retain the actual BCK offset");
        require(!player._88 && !player.checkPass(2.25F) && player.checkPass(2.75F),
                "After calculation, pass queries must address the next interval");
        const auto* clock = player._20;
        player.changeAnimationByHash(MR::getHashCode("Cycle"));
        require(player._20 == clock, "Re-requesting the current group must retain its frame controller");
        near(player._20->getFrame(), 2.5F, "Re-requesting the current group must not restart time");
        player.updateAfterMovement();
        fixture.calculate();
        near(fixture.model.object->getAnmMtx(0)[0][3], 30, "Next calculation phase must advance by the authored half-frame");
        require(player.getNameStringPointer("Cycle") == fixture.groups.groups[0].mParent.mAnimationName &&
                    player.getNameStringPointer("Missing") == nullptr &&
                    std::string_view(player.getCurrentBckName()) == "FixtureFirst",
                "Group-name queries must preserve original retained strings and missing-group results");
    }

    void weighted_tracks_and_interpolation() {
        Fixture fixture;
        auto& player = fixture.player.object;
        player.changeAnimationByHash(MR::getHashCode("Blend"));
        fixture.calculate();
        near(fixture.first.animation.getFrame(), 2.5F, "Primary BCK must use its own duration");
        near(fixture.second.animation.getFrame(), 5, "Secondary BCK must preserve phase with its different duration");
        near(fixture.model.object->getAnmMtx(0)[0][3], 88.75F, "Actual Core must blend 25 and 110 using authored quarter/three-quarter weights");
        near(fixture.model.object->getAnmMtx(1)[1][3], 6, "Child transform must use the same actual track weights");
        TVec3f translation;
        player.getMainAnimationTrans(0, &translation);
        near(translation.x, 110, "Main-animation translation query must select the higher-weight real track");
        require(player.changeTrackWeight(0, 0.75F) && player.changeTrackWeight(1, 0.25F) && !player.changeTrackWeight(2, 1),
                "Track weight changes must enforce the current group's declared track count");
        player.updateAfterMovement();
        fixture.calculate();
        near(fixture.model.object->getAnmMtx(0)[0][3], 50.5F, "Changed weights must combine next-phase translations 30 and 112");
        player.changeInterpoleFrame(4);
        near(player._08, 0.25F, "Original four-frame interpolation starts with one quarter");
        require(player._20->_14 == 3, "Interpolation must retain its original decreasing divisor");
        player.updateInterpoleRatio();
        near(player._08, 0.5F, "Original next interpolation step must reach one half");
        player.changeSpeed(0);
        player.updateInterpoleRatio();
        near(player._08, 1, "A paused ordinary loop must finish original interpolation immediately");
        require(player._20->_14 == 0, "Paused-loop interpolation must clear the remaining divisor");
        player.changeInterpoleFrame(0);
        near(player.mCore->_1C, 1, "Zero interpolation must update the actual Core blend ratio");
    }

    void termination_default_and_timer() {
        Fixture fixture;
        auto& player = fixture.player.object;
        player.setDefaultAnimation("Cycle");
        player.changeAnimation("Once");
        player.changeInterpoleFrame(0);
        fixture.calculate();
        near(fixture.model.object->getAnmMtx(0)[0][3], 20, "One-shot animation must start at its authored two-frame rate");
        player.updateAfterMovement();
        require(player.isTerminate() && player.isTerminate("Once") && player._20->getRate() == 2,
                "Original player must retain the authored rate when its frame controller reports termination");
        fixture.calculate();
        near(fixture.model.object->getAnmMtx(0)[0][3], 40, "Terminated animation must sample the exact authored end frame");
        player.updateBeforeMovement();
        require(player.mCurrentAnimation == &fixture.groups.groups[2], "Default hold policy must retain the terminated group");
        player._7E = false;
        player.updateBeforeMovement();
        require(player.isRun("Cycle") && !player.isTerminate() && player.mPrevAnimation == &fixture.groups.groups[2],
                "Clearing original hold policy must transition a completed one-shot to its actual default");
        player.changeAnimation("Blend");
        player.stopAnimation("Once");
        require(player.isRun("Blend"), "Stopping a different named group must preserve current playback");
        player._78 = 2;
        player.updateBeforeMovement();
        require(player.isRun("Blend") && player._78 == 1, "Original countdown must retain playback until it reaches zero");
        player.updateBeforeMovement();
        require(player.isRun("Cycle") && std::string_view(player.getDefaultAnimationName()) == "Cycle",
                "Countdown completion must run the retained default group");
    }

    void simple_animation_and_independent_groups() {
        Fixture fixture;
        auto& player = fixture.player.object;
        Player other(fixture.model, fixture.groups, fixture.player);
        other.duplicate_simple_group();
        player.changeAnimationSimple(&fixture.first.animation);
        other.object.changeAnimationSimple(&fixture.second.animation);
        require(player.isAnimationRunSimple() && other.object.isAnimationRunSimple() &&
                    player.getSimpleGroup() != other.object.getSimpleGroup() &&
                    player.getSimpleGroup()->_20[0] == &fixture.first.animation &&
                    other.object.getSimpleGroup()->_20[0] == &fixture.second.animation,
                "Duplicate simple groups must retain distinct actual BCK bindings while sharing joint history");
        require(std::string_view(other.object.getCurrentAnimationName()) == "dup-non-group",
                "Duplicate simple group must preserve the original name");
        other.object.changeInterpoleFrame(0);
        other.object.updateAfterMovement();
        other.object.calcAnm(0);
        fixture.model.object->calc();
        near(fixture.model.object->getAnmMtx(0)[0][3], 102, "Direct typed simple animation must calculate through the real model at frame one");
        near(other.object._20->getEnd(), 20, "Simple animation controller must use its actual BCK duration");
        other.object.clearAnm(0);
        require(fixture.model.joints[0].mMtxCalc == nullptr, "Original clearAnm must remove the active joint calculator");
    }

    void model_rebinding_and_calculator_slots() {
        Fixture fixture;
        Model replacement;
        replacement.joints[1].mTransformInfo.mTranslate = {11, 12, 13};
        auto& player = fixture.player.object;
        fixture.player.enable_transforms(fixture.model);
        player.changeAnimation("Cycle");
        player.clearMtxCalc(0);
        player.setModel(replacement.object.get());
        require(player.mModel == replacement.object.get() && player.mModelData == &replacement.data &&
                    player.mCore->mTransformList[1]._0 == &replacement.joints[1],
                "setModel must rebind the existing actual per-joint transform storage");
        near(player.mCore->mTransformList[1].mTransformInfo.mTranslate.y, 12,
             "Rebinding must copy the replacement joint's actual bind-pose metadata");
        player.overWriteMtxCalc(1);
        require(replacement.joints[1].mMtxCalc == player.mCore && fixture.model.joints[1].mMtxCalc == nullptr,
                "Calculator attachment must address the selected model's actual joint");
        player.clearAnm(1);
        require(replacement.joints[1].mMtxCalc == nullptr, "Active clearAnm must detach the replacement calculator slot");
    }
}

int main() {
    try {
        construction_and_shared_storage();
        authored_rate_phase_and_queries();
        weighted_tracks_and_interpolation();
        termination_default_and_timer();
        simple_animation_and_independent_groups();
        model_rebinding_and_calculator_slots();
        std::cout << "6/6 original XanimePlayer lifecycle groups passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Original XanimePlayer regression failed: " << error.what() << '\n';
        return 1;
    }
}
