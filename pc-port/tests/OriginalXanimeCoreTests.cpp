#include "compat/MetrowerksStdCompat.hpp"

#include "Game/Animation/XanimeCore.hpp"
#include "Game/Util/MathUtil.hpp"
#include "JSystem/J3DGraphAnimator/J3DAnimation.hpp"
#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/J3DGraphAnimator/J3DMtxBuffer.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"

#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

extern u8 j3dTevSwapTableTable[1024];
extern u8 j3dAlphaCmpTable[768];
extern u8 j3dZModeTable[96];

namespace {
    using Matrix = std::array<float, 12>;
    constexpr Matrix Identity{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    constexpr Matrix RotateZ90{0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0};

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void near(float actual, float expected, std::string_view message, float tolerance = 0.002F) {
        if (!std::isfinite(actual) || std::fabs(actual - expected) > tolerance) {
            throw std::runtime_error(std::string(message) + ": actual=" + std::to_string(actual) +
                                     "; expected=" + std::to_string(expected));
        }
    }

    Matrix snapshot(const Mtx matrix) {
        Matrix result{};
        for (std::size_t row = 0; row < 3; ++row) {
            for (std::size_t column = 0; column < 4; ++column) {
                result[row * 4 + column] = matrix[row][column];
            }
        }
        return result;
    }

    void assign(Mtx matrix, const Matrix& values) {
        for (std::size_t row = 0; row < 3; ++row) {
            for (std::size_t column = 0; column < 4; ++column) {
                matrix[row][column] = values[row * 4 + column];
            }
        }
    }

    void vector_near(const Vec& actual, const Vec& expected, std::string_view message) {
        near(actual.x, expected.x, message);
        near(actual.y, expected.y, message);
        near(actual.z, expected.z, message);
    }

    void matrix_near(const Mtx actual, const Matrix& expected, std::string_view message) {
        const auto values = snapshot(actual);
        for (std::size_t index = 0; index < values.size(); ++index) {
            near(values[index], expected[index], std::string(message) + " component " + std::to_string(index));
        }
    }

    struct Globals {
        Matrix matrix = snapshot(J3DSys::mCurrentMtx);
        Vec scale = J3DSys::mCurrentS;
        Vec parent_scale = J3DSys::mParentS;
        J3DJoint* joint = J3DMtxCalc::getJoint();
        J3DMtxBuffer* buffer = J3DMtxCalc::getMtxBuffer();
        J3DMtxCalc* joint_calculator = J3DJoint::mCurrentMtxCalc;
        J3DMtxCalc* system_calculator = j3dSys.mCurrentMtxCalc;

        ~Globals() {
            assign(J3DSys::mCurrentMtx, matrix);
            J3DSys::mCurrentS = scale;
            J3DSys::mParentS = parent_scale;
            J3DMtxCalc::setJoint(joint);
            J3DMtxCalc::setMtxBuffer(buffer);
            J3DJoint::mCurrentMtxCalc = joint_calculator;
            j3dSys.mCurrentMtxCalc = system_calculator;
        }
    };

    // The original Core destructor is empty. The fixture owns its allocations,
    // while shared-core construction owns only its newly allocated track array.
    struct Core {
        XanimeCore object;
        std::unique_ptr<XjointInfo[]> joints;
        std::unique_ptr<XanimeTrack[]> tracks;
        std::unique_ptr<XjointTransform[]> transforms;

        Core(u32 track_count, u32 joint_count, u8 mode)
            : object(track_count, joint_count, mode), joints(object.mJointList), tracks(object.mTrackList) {}

        Core(u32 track_count, XanimeCore& other)
            : object(track_count, &other), tracks(object.mTrackList) {}

        void enable_transforms(J3DModelData& data) {
            require(object.mTransformList == nullptr, "fixture enables its transform allocation only once");
            object.enableJointTransform(&data);
            transforms.reset(object.mTransformList);
        }
    };

    template <std::size_t Count>
    struct Model {
        J3DModelData data;
        std::array<J3DJoint, Count> joints;
        std::array<J3DJoint*, Count> joint_pointers{};
        J3DMtxBuffer buffer;
        Mtx matrices[Count]{};
        std::array<u8, Count> flags{};

        Model() {
            auto& tree = data.getJointTree();
            tree.mRootNode = &joints[0];
            tree.mJointNum = static_cast<u16>(Count);
            tree.mJointNodePointer = joint_pointers.data();
            for (std::size_t index = 0; index < Count; ++index) {
                joints[index].mJntNo = static_cast<u16>(index);
                joint_pointers[index] = &joints[index];
                flags[index] = 0xFF;
                assign(matrices[index], Identity);
            }
            buffer.mJointTree = &tree;
            buffer.mpAnmMtx = matrices;
            buffer.mpUserAnmMtx = matrices;
            buffer.mpScaleFlagArr = flags.data();
        }

        void attach(XanimeCore& core) {
            data.setBasicMtxCalc(&core);
            core.initT(&data);
        }

        void calculate(const Vec& scale = {1, 1, 1}, const Matrix& matrix = Identity) {
            Mtx base;
            assign(base, matrix);
            data.getJointTree().calc(&buffer, scale, base);
        }

        void select(std::size_t index) {
            J3DMtxCalc::setJoint(&joints[index]);
            J3DMtxCalc::setMtxBuffer(&buffer);
        }
    };

    template <std::size_t Count>
    struct KeyClip {
        J3DAnmTransformKey animation;
        std::array<J3DAnmTransformKeyTable, Count * 3> tables{};
        std::array<float, Count * 3> scales{};
        std::array<s16, Count * 3> rotations{};
        std::array<float, Count * 3> translations{};

        KeyClip() {
            scales.fill(1);
            for (u16 axis = 0; axis < Count * 3; ++axis) {
                tables[axis].mScaleInfo = {1, axis, 0};
                tables[axis].mRotationInfo = {1, axis, 0};
                tables[axis].mTranslateInfo = {1, axis, 0};
            }
            animation.mFrameMax = 4;
            animation.field_0x1e = static_cast<u16>(Count);
            animation.mAnmTable = tables.data();
            animation.mScaleData = scales.data();
            animation.mRotData = rotations.data();
            animation.mTransData = translations.data();
        }

        void pose(std::size_t joint, const Vec& scale, const Vec& translation, s16 z_rotation = 0) {
            const auto at = joint * 3;
            scales[at] = scale.x;
            scales[at + 1] = scale.y;
            scales[at + 2] = scale.z;
            translations[at] = translation.x;
            translations[at + 1] = translation.y;
            translations[at + 2] = translation.z;
            rotations[at + 2] = z_rotation;
        }
    };

    struct LinearKeyClip {
        J3DAnmTransformKey animation;
        std::array<J3DAnmTransformKeyTable, 3> tables{};
        std::array<float, 3> scales{1, 1, 1};
        std::array<s16, 3> rotations{};
        // Two Hermite keys describe x = 2*frame; y/z are constant.
        std::array<float, 8> translations{0, 0, 2, 4, 8, 2, 10, 20};

        LinearKeyClip() {
            for (u16 axis = 0; axis < 3; ++axis) {
                tables[axis].mScaleInfo = {1, axis, 0};
                tables[axis].mRotationInfo = {1, axis, 0};
            }
            tables[0].mTranslateInfo = {2, 0, 0};
            tables[1].mTranslateInfo = {1, 6, 0};
            tables[2].mTranslateInfo = {1, 7, 0};
            animation.mFrameMax = 4;
            animation.field_0x1e = 1;
            animation.mAnmTable = tables.data();
            animation.mScaleData = scales.data();
            animation.mRotData = rotations.data();
            animation.mTransData = translations.data();
        }
    };

    struct FullClip {
        J3DAnmTransformFull animation;
        std::array<J3DAnmTransformFullTable, 3> tables{};
        std::array<float, 3> scales{1, 1, 1};
        std::array<s16, 3> rotations{};
        std::array<float, 9> translations{10, 20, 40, 4, 4, 4, 5, 5, 5};

        FullClip() {
            for (u16 axis = 0; axis < 3; ++axis) {
                tables[axis] = {1, axis, 1, axis, 3, static_cast<u16>(axis * 3)};
            }
            animation.mFrameMax = 2;
            animation.field_0x1e = 1;
            animation.mAnmTable = tables.data();
            animation.mScaleData = scales.data();
            animation.mRotData = rotations.data();
            animation.mTransData = translations.data();
        }

        void constant_translation(const Vec& translation) {
            translations = {translation.x, translation.x, translation.x,
                            translation.y, translation.y, translation.y,
                            translation.z, translation.z, translation.z};
        }
    };

    void test_genuine_system_initialization() {
        require(j3dSys.mCurrentMtxCalc == nullptr && j3dSys.mModel == nullptr && j3dSys.mNBTScale == nullptr,
                "the genuine static system receives BSS initialization before its original constructor");
        require(j3dSys.mFlags == 0 && j3dSys.mDrawMode == 1 && j3dSys.mMaterialMode == 0,
                "original system flags and draw/material defaults");
        matrix_near(j3dSys.mViewMtx, Identity, "original system view matrix");
        for (const auto& scale : J3DSys::sTexCoordScaleTable) {
            require(scale.field_0x00 == 1 && scale.field_0x02 == 1 && scale.field_0x04 == 0 && scale.field_0x06 == 0,
                    "constructor builds all eight texture scale records");
        }
        require(j3dTevSwapTableTable[4 * 0x1B] == 0 && j3dTevSwapTableTable[4 * 0x1B + 1] == 1 &&
                    j3dTevSwapTableTable[4 * 0x1B + 2] == 2 && j3dTevSwapTableTable[4 * 0x1B + 3] == 3,
                "the TEV RGBA selector table contains its identity entry");
        require(j3dAlphaCmpTable[765] == 7 && j3dAlphaCmpTable[766] == 3 && j3dAlphaCmpTable[767] == 7 &&
                    j3dZModeTable[93] == 1 && j3dZModeTable[94] == 7 && j3dZModeTable[95] == 1,
                "the original alpha and Z builders fill their final complete tuples");
    }

    void test_lifecycle_sharing_and_bind_pose() {
        Globals globals;
        Model<3> model;
        model.joints[0].appendChild(&model.joints[1]);
        model.joints[0].appendChild(&model.joints[2]);
        model.joints[0].setTransformInfo({{2, 3, 4}, {0, 0, 0x4000}, {5, 6, 7}});
        Core owner(2, 3, 2);
        auto& core = owner.object;
        require(core.mTrackCount == 2 && core.mJointCount == 3 && core._4 == 2 && core.mTransformList == nullptr,
                "original core constructor retains dimensions and convention");
        require(core._1C == 1 && core._20 == 1 && core.mFrameRatio == 0 && core._28 == 0 && core._29 == 0 && core._6 == 0,
                "original core lifecycle defaults");
        require(core.mTrackList[0].mWeight == 1 && core.mTrackList[1].mWeight == 0 &&
                    core.mTrackList[0]._0 == nullptr && core.mTrackList[1]._0 == nullptr,
                "original tracks start empty with track zero weighted");
        model.attach(core);
        vector_near(core.mJointList[0]._0._0, {2, 3, 4}, "initT frozen bind scale");
        vector_near(core.mJointList[0]._28._C, {5, 6, 7}, "initT previous bind translation");
        near(core.mJointList[0]._28.mRotation.z, std::sqrt(0.5F), "initT converts original Euler rotation");
        owner.enable_transforms(model.data);
        require(core.mTransformList[0]._4 == 0xFFFF && core.mTransformList[1]._4 == 0 && core.mTransformList[2]._4 == 0,
                "transform allocation resolves original child and younger-sibling parents");
        require(core.mTransformList[0]._0 == &model.joints[0] && core.mTransformList[0]._64 == nullptr &&
                    core.mTransformList[0]._68 == nullptr && core.mTransformList[0]._6C == nullptr,
                "transform records retain real joints and empty optional matrices");
        core.mTransformList[0].mScale.set(9, 8, 7);
        model.joints[0].setTransformInfo({{4, 5, 6}, {0, 0, 0}, {8, 9, 10}});
        core.reconfigJointTransform(&model.data);
        vector_near(core.mTransformList[0].mTransformInfo.mScale, {4, 5, 6}, "reconfig replaces authored transform");
        vector_near(core.mTransformList[0].mScale, {9, 8, 7}, "reconfig preserves local transform adjustment");
        Core shared(3, core);
        require(shared.object.mJointList == core.mJointList && shared.object.mTransformList == core.mTransformList &&
                    shared.object.mTrackList != core.mTrackList && shared.object.mTrackCount == 3,
                "shared construction shares poses/transforms but allocates independent tracks");
        Core destination(1, 3, 2);
        destination.object.shareJointTransform(&core);
        require(destination.object.mTransformList == core.mTransformList, "shareJointTransform retains original non-owning relationship");
        for (u32 index = 0; index < 3; ++index) {
            core.mJointList[index]._28._C.set(static_cast<float>(10 + index), 0.0F, 0.0F);
        }
        core.freezeCopy(&model.data, &destination.object, 0, 4);
        for (u32 index = 0; index < 3; ++index) {
            near(destination.object.mJointList[index]._0._C.x, static_cast<float>(10 + index), "recursive freezeCopy visits whole subtree");
            require(destination.object.mJointList[index]._5C == 0 && destination.object.mJointList[index]._60 == 0.25F,
                    "freezeCopy starts the authored interpolation duration");
        }
        core.freezeCopy(&model.data, &destination.object, 1, 0);
        require(destination.object.mJointList[1]._60 == 1 && destination.object.mJointList[2]._60 == 0.25F,
                "zero-duration freezeCopy affects the selected subtree without following its siblings");
    }

    void test_frames_single_track_freeze_and_cache() {
        Globals globals;
        Model<1> model;
        Core owner(1, 1, 2);
        auto& core = owner.object;
        model.attach(core);
        LinearKeyClip key;
        FullClip full;
        core.setBck(0, &key.animation);
        core.mFrameRatio = 0.5F;
        core.updateFrame();
        model.calculate();
        near(key.animation.getFrame(), 2, "updateFrame publishes normalized time to real Key animation");
        matrix_near(model.matrices[0], {1, 0, 0, 4, 0, 1, 0, 10, 0, 0, 1, 20}, "Hermite motion reaches original core output");
        require(j3dSys.mCurrentMtxCalc == &core, "calc publishes the actual core in J3DSys");
        core.setBck(0, &full.animation);
        require(core.mTrackList[0]._8 == 0 && core.mTrackList[0]._C == 0, "setBck resets original normalized per-track state");
        core.setWeight(0, 0);
        core._20 = 0;
        core.mFrameRatio = 0.24F;
        core.updateFrame();
        model.calculate();
        near(model.matrices[0][0][3], 10, "single-track path ignores blend weight and smoothing, before Full frame rounding boundary");
        core.mFrameRatio = 0.25F;
        core.updateFrame();
        model.calculate();
        near(model.matrices[0][0][3], 20, "original Full animation rounds exactly half a frame upward");
        core.mFrameRatio = 1;
        core.updateFrame();
        model.calculate();
        near(model.matrices[0][0][3], 40, "last original Full sample");
        core.doFreeze();
        require(core._28 == 1 && core._1C == 0, "freeze request does not immediately copy poses");
        core.mFrameRatio = 0;
        core.updateFrame();
        require(core._28 == 0 && core._29 == 1, "frame update promotes one freeze-copy request");
        model.calculate();
        near(model.matrices[0][0][3], 40, "zero freeze progress retains the previous visible pose");
        near(core.mJointList[0]._50.x, 10, "unblended translation remains available during freeze");
        core._1C = 0.5F;
        core.updateFrame();
        require(core._29 == 0, "freeze-copy flag lasts one update");
        model.calculate();
        near(model.matrices[0][0][3], 25, "half freeze progress interpolates old forty and new ten");
        core.setBck(0, nullptr);
        model.calculate();
        near(model.matrices[0][0][3], 25, "absent single-track resource uses the last calculated pose");
    }

    void test_two_tracks_weight_smoothing_and_quaternion() {
        Globals globals;
        Model<1> model;
        Core owner(2, 1, 2);
        auto& core = owner.object;
        model.attach(core);
        KeyClip<1> first;
        FullClip second;
        second.scales = {3, 5, 7};
        second.rotations[2] = 0x4000;
        second.constant_translation({8, 12, 16});
        core.setBck(0, &first.animation);
        core.setBck(1, &second.animation);
        core.setWeight(0, 1);
        core.setWeight(1, 1);
        core.updateFrame();
        model.calculate();
        const float h = std::sqrt(0.5F);
        matrix_near(model.matrices[0], {2 * h, -3 * h, 0, 4, 2 * h, 3 * h, 0, 6, 0, 0, 4, 8},
                    "equally weighted real Key and Full poses produce halfway rotation and scale");
        near(core.mJointList[0]._28.mRotation.z, h * 0.5F, "original quaternion cache retains linear unnormalized z");
        near(core.mJointList[0]._28.mRotation.w, (1 + h) * 0.5F, "matrix conversion handles original unnormalized quaternion");
        core.mTrackList[0]._C = 1;
        core.mTrackList[0]._8 = 0.75F;
        core.mFrameRatio = 0.25F;
        core.updateFrame();
        near(first.animation.getFrame(), 3, "track-specific normalized time overrides core ratio");
        near(second.animation.getFrame(), 0.5F, "other track continues using core ratio");
        second.constant_translation({24, 12, 16});
        core._20 = 0.25F;
        model.calculate();
        near(model.matrices[0][0][3], 6, "multi-track smoothing blends previous four with target twelve");
        near(core.mJointList[0]._50.x, 12, "raw blend translation precedes temporal smoothing");
        core.setWeight(0, 1);
        core.setWeight(1, -1);
        model.calculate();
        near(model.matrices[0][0][3], 6, "zero total weight preserves the cached pose even with nonzero signed weights");
    }

    void test_original_vector_blend_rounding_and_aliases() {
        struct Witness {
            std::uint32_t from;
            std::uint32_t to;
            std::uint32_t from_weight;
            std::uint32_t to_weight;
            std::uint32_t expected;
        };
        // Exact instruction-order witnesses from the retail PSvecBlend audit:
        // rounded from*weight is followed by fused to*weight + that product.
        constexpr std::array<Witness, 2> witnesses{{
            {0x3F800001U, 0xBF800000U, 0x3F800001U, 0x3F800002U, 0x00000000U},
            {0x3F800000U, 0x3F800001U, 0xBF800000U, 0x3F7FFFFEU, 0xA8800000U},
        }};
        for (const auto& witness : witnesses) {
            for (int alias = 0; alias < 3; ++alias) {
                TVec3f from(std::bit_cast<float>(witness.from));
                TVec3f to(std::bit_cast<float>(witness.to));
                TVec3f separate;
                auto* destination = alias == 1 ? &from : alias == 2 ? &to : &separate;
                MR::PSvecBlend(&from, &to, destination, std::bit_cast<float>(witness.from_weight),
                              std::bit_cast<float>(witness.to_weight));
                require(std::bit_cast<std::uint32_t>(destination->x) == witness.expected &&
                            std::bit_cast<std::uint32_t>(destination->y) == witness.expected &&
                            std::bit_cast<std::uint32_t>(destination->z) == witness.expected,
                        "original vector blend preserves exact rounding with separate or aliased output");
            }
        }
    }

    void test_special_pose_and_matrix_phases() {
        Globals globals;
        Model<1> model;
        Core owner(2, 1, 1);
        auto& core = owner.object;
        model.attach(core);
        KeyClip<1> first;
        FullClip second;
        second.scales = {3, 3, 3};
        second.constant_translation({8, 0, 0});
        core.setBck(0, &first.animation);
        core.setBck(1, &second.animation);
        core.setWeight(1, 1);
        core.mJointList[0]._5C = 0;
        core.mJointList[0]._60 = 0.25F;
        core._6 = 1;
        const Matrix sentinel{9, 0, 0, 90, 0, 8, 0, 80, 0, 0, 7, 70};
        assign(model.matrices[0], sentinel);
        model.calculate();
        require(snapshot(model.matrices[0]) == sentinel, "special blend phase updates cached poses without writing matrices");
        near(core.mJointList[0]._28._C.x, 1, "special blend advances per-joint quarter progress");
        near(core.mJointList[0]._28._0.x, 1.25F, "special blend interpolates scale from frozen bind pose");
        core._6 = 2;
        model.calculate();
        matrix_near(model.matrices[0], {1.25F, 0, 0, 1, 0, 1.25F, 0, 0, 0, 0, 1.25F, 0},
                    "special matrix phase uses Maya even when normal convention is SI");
        near(core.mJointList[0]._5C, 0.25F, "special matrix phase does not advance per-joint progress");
        core._6 = 1;
        core.mJointList[0]._5C = 0.9F;
        model.calculate();
        near(core.mJointList[0]._5C, 1, "special interpolation clamps progress after increment");
        near(core.mJointList[0]._28._C.x, 4, "completed special blend reaches weighted target");
        core.setWeight(0, 0);
        core.setWeight(1, 0);
        model.calculate();
        near(core.mJointList[0]._28._C.x, 4, "zero-weight special phase keeps the cached pose");
    }

    void test_original_scale_conventions_and_cancellation() {
        Globals globals;
        for (u8 mode : {u8{0}, u8{1}, u8{2}}) {
            Model<1> model;
            Core owner(1, 1, mode);
            model.attach(owner.object);
            KeyClip<1> clip;
            clip.pose(0, {2, 3, 4}, {1, 2, 3}, 0x4000);
            owner.object.setBck(0, &clip.animation);
            model.calculate({5, 7, 11}, {1, 0, 0, 10, 0, 1, 0, 20, 0, 0, 1, 30});
            const Matrix expected = mode == 1 ? Matrix{0, -315, 0, 35, 140, 0, 0, 118, 0, 0, 1936, 393}
                                              : Matrix{0, -15, 0, 15, 14, 0, 0, 34, 0, 0, 44, 63};
            matrix_near(model.matrices[0], expected, "Core convention preserves original base/local scale ordering");
            require(model.flags[0] == 0, "nonunit output scale flag");
            clip.pose(0, {0.5F, 0.25F, 0.125F}, {1, 2, 3}, 0x4000);
            model.calculate({2, 4, 8});
            const Matrix cancelled = mode == 0 ? Matrix{0, -2, 0, 2, 4, 0, 0, 8, 0, 0, 8, 24}
                                     : mode == 1 ? Matrix{0, -0.5F, 0, 4, 2, 0, 0, 32, 0, 0, 1, 192}
                                                 : Matrix{0, -0.5F, 0, 2, 2, 0, 0, 8, 0, 0, 1, 24};
            matrix_near(model.matrices[0], cancelled, "accumulated cancellation preserves each original convention");
            require(model.flags[0] == (mode == 2 ? 0 : 1), "Basic/SI accumulated predicate differs from Maya local predicate");
        }
    }

    void test_recursive_core_traversal_and_parent_compensation() {
        Globals globals;
        Model<3> model;
        Core owner(1, 3, 2);
        model.attach(owner.object);
        model.joints[0].appendChild(&model.joints[1]);
        model.joints[0].setYounger(&model.joints[2]);
        model.joints[1].mScaleCompensate = 1;
        KeyClip<3> clip;
        clip.pose(0, {2, 4, 8}, {1, 0, 0});
        clip.pose(1, {1, 1, 1}, {0, 3, 0});
        clip.pose(2, {1, 1, 1}, {0, 0, 5});
        owner.object.setBck(0, &clip.animation);
        owner.object.mFrameRatio = 0.5F;
        owner.object.updateFrame();
        const Matrix base{1, 0, 0, 100, 0, 1, 0, 200, 0, 0, 1, 300};
        model.calculate({1, 1, 1}, base);
        matrix_near(model.matrices[0], {2, 0, 0, 101, 0, 4, 0, 200, 0, 0, 8, 300}, "Core root matrix");
        matrix_near(model.matrices[1], {1, 0, 0, 101, 0, 1, 0, 212, 0, 0, 1, 300}, "child cancels parent scale while keeping parent-scaled translation");
        matrix_near(model.matrices[2], {1, 0, 0, 100, 0, 1, 0, 200, 0, 0, 1, 305}, "root sibling restores base state");
        matrix_near(J3DSys::mCurrentMtx, base, "recursive traversal restores incoming matrix");
        vector_near(J3DSys::mParentS, {1, 1, 1}, "recursive traversal restores incoming parent scale");
        require(J3DMtxCalc::getJoint() == &model.joints[2] && J3DMtxCalc::getMtxBuffer() == &model.buffer &&
                    j3dSys.mCurrentMtxCalc == &owner.object && J3DJoint::mCurrentMtxCalc == &owner.object,
                "original traversal retains selected buffer, final joint, and core identities");
        near(clip.animation.getFrame(), 2, "joint traversal samples without advancing animation time");
    }

    void test_optional_transform_matrices_and_offsets() {
        Globals globals;
        for (u8 mode : {u8{0}, u8{2}}) {
            Model<1> model;
            Core owner(1, 1, mode);
            auto& core = owner.object;
            model.attach(core);
            owner.enable_transforms(model.data);
            KeyClip<1> clip;
            clip.pose(0, {1, 1, 1}, {1, 2, 3});
            core.setBck(0, &clip.animation);
            Mtx first, second, world_rotation, base;
            assign(first, RotateZ90);
            assign(second, RotateZ90);
            assign(world_rotation, RotateZ90);
            assign(base, Identity);
            auto& transform = core.mTransformList[0];
            transform._64 = first;
            transform._68 = second;
            transform._6C = world_rotation;
            transform.mScale.set(2, 1, 1);
            transform._14.set(3, 1, 1);
            transform._2C.set(4, 5, 6);
            transform._38.set(10, 20, 30);
            transform._20 = 100;
            transform._24 = 200;
            transform._28 = 300;
            core.init({1, 1, 1}, base);
            model.select(0);
            core.calc();
            matrix_near(model.matrices[0], {0, 1, 0, 115, -6, 0, 0, 227, 0, 0, 1, 339},
                        "three real matrix pointers and separate local/world/output offsets reach original Maya calculation");
            matrix_near(J3DSys::mCurrentMtx, {0, 1, 0, 15, -6, 0, 0, 27, 0, 0, 1, 39},
                        "output-only adjustment does not leak into descendant traversal matrix");
        }

        Model<2> model;
        Core owner(1, 2, 2);
        model.joints[0].appendChild(&model.joints[1]);
        model.attach(owner.object);
        owner.enable_transforms(model.data);
        KeyClip<2> clip;
        clip.pose(1, {1, 1, 1}, {1, 0, 0});
        owner.object.setBck(0, &clip.animation);
        Mtx parent_rotation;
        assign(parent_rotation, RotateZ90);
        owner.object.mTransformList[0]._68 = parent_rotation;
        owner.object.mTransformList[0].mScale.set(2, 4, 8);
        model.calculate();
        matrix_near(model.matrices[1], {1, 0, 0, 0, 0, 1, 0, 2, 0, 0, 1, 0},
                    "child cancels its parent's additional matrix and local adjustment scale while retaining translated basis");
    }
}  // namespace

int main() {
    try {
        test_genuine_system_initialization();
        test_lifecycle_sharing_and_bind_pose();
        test_frames_single_track_freeze_and_cache();
        test_two_tracks_weight_smoothing_and_quaternion();
        test_original_vector_blend_rounding_and_aliases();
        test_special_pose_and_matrix_phases();
        test_original_scale_conventions_and_cancellation();
        test_recursive_core_traversal_and_parent_compensation();
        test_optional_transform_matrices_and_offsets();
        std::cout << "9/9 original XanimeCore system groups passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[fail] " << error.what() << '\n';
        return 1;
    }
}
