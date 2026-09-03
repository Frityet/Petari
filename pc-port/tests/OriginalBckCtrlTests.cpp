#include "compat/MetrowerksStdCompat.hpp"
#include "Game/Animation/BckCtrl.hpp"
#include "Game/Animation/XanimeCore.hpp"
#include "Game/Animation/XanimePlayer.hpp"
#include "Game/Animation/XanimeResource.hpp"
#include "JSystem/J3DGraphAnimator/J3DModel.hpp"

#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
    void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
    void near(float actual, float expected, const char* message) {
        require(std::isfinite(actual) && std::fabs(actual - expected) < 0.00001F, message);
    }
    // Actual original model, player and resource-table constructors. The
    // table's absent ResourceHolder is never queried by these frame-control APIs.
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

    struct Fixture {
        Model model;
        XanimeResourceTable resources{nullptr};
        XanimePlayer player{model.object.get(), &resources};
        std::unique_ptr<XanimeCore> core{player.mCore};
        std::unique_ptr<XjointInfo[]> joints{core->mJointList};
        std::unique_ptr<XanimeTrack[]> tracks{core->mTrackList};
        Fixture() {
            auto* ctrl = player._20;
            ctrl->setStart(2); ctrl->setEnd(20); ctrl->setLoop(4);
            ctrl->setFrame(7); ctrl->setRate(1.25F); ctrl->setAttribute(2);
            ctrl->_14 = 7; player._84 = 13;
        }
    };
    void test_copy() {
        BckCtrlData source, result;
        source.mName = "Authored"; source.mPlayFrame = -32; source.mStartFrame = 6;
        source.mEndFrame = 19; source.mRepeatFrame = -9; source.mInterpole = 4;
        source.mLoopMode = 3; source._F = 0x41; source._10 = 0x82; source._11 = 0xc3;
        result = source;
        require(result.mName == source.mName && result.mPlayFrame == -32 && result.mStartFrame == 6 &&
                result.mEndFrame == 19 && result.mRepeatFrame == -9 && result.mInterpole == 4 &&
                result.mLoopMode == 3 && result._F == 0x41 && result._10 == 0x82 && result._11 == 0xc3,
                "assignment preserves all original fields and borrowed-name identity");
        result = result;
        require(result.mName == source.mName && result._11 == 0xc3, "self-assignment preserves values");
    }
    void test_default_preserves_state() {
        Fixture f;
        BckCtrlData control;
        BckCtrlFunction::reflectBckCtrlData(control, &f.player);
        auto* frame = f.player._20;
        require(frame->getStart() == 2 && frame->getEnd() == 20 && frame->getLoop() == 4 &&
                frame->_14 == 7 && frame->getAttribute() == 2, "default sentinel fields preserve authored state");
        near(frame->getFrame(), 7, "default leaves frame"); near(frame->getRate(), 1.25F, "default leaves rate");
        near(f.player._84, 13, "default leaves preceding frame");
    }
    void test_full_settings() {
        Fixture f;
        BckCtrlData control;
        control.mStartFrame = 3; control.mEndFrame = 11; control.mRepeatFrame = 5;
        control.mPlayFrame = 4; control.mInterpole = 0; control.mLoopMode = 4;
        BckCtrlFunction::reflectBckCtrlData(control, &f.player);
        auto* frame = f.player._20;
        require(frame->getStart() == 3 && frame->getEnd() == 11 && frame->getLoop() == 5 &&
                frame->getAttribute() == 4 && frame->_14 == 0, "explicit range, loop and interpolation settings");
        near(frame->getFrame(), 3, "start rewinds current frame"); near(f.player._84, 3, "start synchronizes previous frame");
        near(frame->getRate(), 2, "eight-frame range over four playback frames");
        near(f.player._08, 1, "zero interpolation is immediate"); near(f.core->_1C, 1, "zero interpolation reaches original core");
    }
    void test_ordered_bounds() {
        Fixture f;
        BckCtrlData control;
        control.mStartFrame = 21; control.mEndFrame = 9; control.mRepeatFrame = 12;
        BckCtrlFunction::reflectBckCtrlData(control, &f.player);
        require(f.player._20->getStart() == 2 && f.player._20->getEnd() == 9 && f.player._20->getLoop() == 4,
                "invalid start rejected and repeat checked against newly reduced end");
        near(f.player._20->getFrame(), 7, "rejected start does not reset frame");
        near(f.player._84, 13, "rejected start does not reset preceding frame");
    }
    void test_reverse_interval() {
        Fixture f;
        BckCtrlData control;
        control.mStartFrame = 8; control.mEndFrame = 0; control.mPlayFrame = 4;
        BckCtrlFunction::reflectBckCtrlData(control, &f.player);
        require(f.player._20->getStart() == 8 && f.player._20->getEnd() == 0 && f.player._20->getLoop() == 8,
                "original accepts end below start without range normalization");
        near(f.player._20->getRate(), -2, "reverse interval preserves signed authored speed");
    }
    void test_inclusive_bounds() {
        Fixture f;
        BckCtrlData control;
        control.mStartFrame = 20; control.mEndFrame = 20; control.mRepeatFrame = 20; control.mPlayFrame = 32767;
        BckCtrlFunction::reflectBckCtrlData(control, &f.player);
        require(f.player._20->getStart() == 20 && f.player._20->getLoop() == 20, "frame bounds are inclusive");
        near(f.player._20->getRate(), 0, "zero interval has zero rate even with nonzero duration");
    }
    void test_interpolation_before_loop_mode() {
        Fixture f;
        BckCtrlData control;
        control.mPlayFrame = 0; control.mInterpole = 4; control.mLoopMode = 1;
        BckCtrlFunction::reflectBckCtrlData(control, &f.player);
        near(f.player._20->getRate(), 0, "zero play frame stops speed");
        require(f.player._20->getAttribute() == 1 && f.player._20->_14 == 0,
                "interpolation observes prior loop mode before final attribute assignment");
        near(f.player._08, 1, "stopped prior mode completes interpolation");
        near(f.core->_1C, 0, "original core copy remains at changeInterpoleFrame's pre-update value");
    }
}
int main() {
    const std::array tests{
        std::pair{"complete data assignment", test_copy}, std::pair{"sentinels preserve state", test_default_preserves_state},
        std::pair{"authored settings", test_full_settings}, std::pair{"ordered frame bounds", test_ordered_bounds},
        std::pair{"reverse interval", test_reverse_interval}, std::pair{"inclusive bounds", test_inclusive_bounds},
        std::pair{"interpolation and loop-mode order", test_interpolation_before_loop_mode},
    };
    for (const auto& [name, test] : tests) {
        try { test(); std::cout << "PASS " << name << '\n'; }
        catch (const std::exception& error) { std::cerr << "FAIL " << name << ": " << error.what() << '\n'; return 1; }
    }
}
