#include "Game/Animation/XanimeCore.hpp"
#include "JSystem/J3DGraphAnimator/J3DAnimation.hpp"
#include "JSystem/J3DGraphAnimator/J3DJoint.hpp"
#include "JSystem/J3DGraphAnimator/J3DJointTree.hpp"
#include "JSystem/J3DGraphAnimator/J3DMtxBuffer.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "compat/OriginalJ3dJointTree.hpp"
#include "render/J3dMaterialRuntime.hpp"
#include "render/J3dModel.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

    using Basic = J3DMtxCalcNoAnm<J3DMtxCalcCalcTransformBasic, J3DMtxCalcJ3DSysInitBasic>;
    using Softimage = J3DMtxCalcNoAnm<J3DMtxCalcCalcTransformSoftimage, J3DMtxCalcJ3DSysInitSoftimage>;
    using Maya = J3DMtxCalcNoAnm<J3DMtxCalcCalcTransformMaya, J3DMtxCalcJ3DSysInitMaya>;
    using Matrix = std::array<float, 12>;

    constexpr Matrix Identity{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void near(float actual, float expected, std::string_view message) {
        if (!std::isfinite(actual) || std::fabs(actual - expected) > 0.0002F) {
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

    void assign(Mtx destination, const Matrix& source) {
        for (std::size_t row = 0; row < 3; ++row) {
            for (std::size_t column = 0; column < 4; ++column) {
                destination[row][column] = source[row * 4 + column];
            }
        }
    }

    void matrix_near(const Matrix& actual, const Matrix& expected, std::string_view message) {
        for (std::size_t component = 0; component < actual.size(); ++component) {
            near(actual[component], expected[component], std::string(message) + " component " + std::to_string(component));
        }
    }

    void vector_near(const Vec& actual, const Vec& expected, std::string_view message) {
        near(actual.x, expected.x, message);
        near(actual.y, expected.y, message);
        near(actual.z, expected.z, message);
    }

    struct Globals {
        Matrix matrix = snapshot(J3DSys::mCurrentMtx);
        Vec current_scale = J3DSys::mCurrentS;
        Vec parent_scale = J3DSys::mParentS;
        J3DJoint* joint = J3DMtxCalc::getJoint();
        J3DMtxBuffer* buffer = J3DMtxCalc::getMtxBuffer();
        J3DMtxCalc* calculator = J3DJoint::mCurrentMtxCalc;
        J3DMtxCalc* system_calculator = j3dSys.mCurrentMtxCalc;

        ~Globals() {
            assign(J3DSys::mCurrentMtx, matrix);
            J3DSys::mCurrentS = current_scale;
            J3DSys::mParentS = parent_scale;
            J3DMtxCalc::setJoint(joint);
            J3DMtxCalc::setMtxBuffer(buffer);
            J3DJoint::mCurrentMtxCalc = calculator;
            j3dSys.mCurrentMtxCalc = system_calculator;
        }
    };

    struct Observation {
        std::uint16_t joint;
        int phase;
        Matrix matrix;
        Vec current_scale;
        Vec parent_scale;
        J3DMtxCalc* calculator;
        J3DJoint* calculator_joint;
    };

    struct Trace {
        std::vector<std::string> calls;
        std::vector<Observation> callbacks;
        bool mutate_matrices = false;
        unsigned int replacement_calls = 0;
    };

    int replacement_callback(J3DJoint* joint, int) {
        ++static_cast<Trace*>(joint->mCallBackUserData)->replacement_calls;
        return 1;
    }

    int callback(J3DJoint* joint, int phase) {
        auto& trace = *static_cast<Trace*>(joint->mCallBackUserData);
        trace.calls.push_back("callback" + std::to_string(joint->getJntNo()) + ":" + std::to_string(phase));
        trace.callbacks.push_back({joint->getJntNo(), phase, snapshot(J3DSys::mCurrentMtx),
                                   J3DSys::mCurrentS, J3DSys::mParentS, J3DJoint::mCurrentMtxCalc,
                                   J3DMtxCalc::getJoint()});
        if (trace.mutate_matrices) {
            if (joint->getJntNo() == 0 && phase == 0) {
                J3DSys::mCurrentMtx[0][3] += 100;
            } else if (joint->getJntNo() == 1 && phase == 0) {
                // recursiveCalc retains the function pointer before phase zero.
                joint->setCallBack(replacement_callback);
            } else if (joint->getJntNo() == 1 && phase == 1) {
                J3DSys::mCurrentMtx[1][3] += 50;
            } else if (joint->getJntNo() == 0 && phase == 1) {
                J3DSys::mCurrentMtx[2][3] += 7;
            }
        }
        return 0;  // The original traversal ignores the return value.
    }

    const Observation& observed(const Trace& trace, std::uint16_t joint, int phase) {
        for (const auto& entry : trace.callbacks) {
            if (entry.joint == joint && entry.phase == phase) {
                return entry;
            }
        }
        throw std::runtime_error("expected original callback was not invoked");
    }

    template <class Original>
    class RecordingCalculator final : public Original {
    public:
        RecordingCalculator(Trace& trace, std::string name) : _trace(trace), _name(std::move(name)) {}

        void init(const Vec& scale, const Mtx& matrix) override {
            ++initializations;
            Original::init(scale, matrix);
        }

        void calc() override {
            _trace.calls.push_back(_name + std::to_string(J3DMtxCalc::getJoint()->getJntNo()));
            Original::calc();
        }

        unsigned int initializations = 0;

    private:
        Trace& _trace;
        std::string _name;
    };

    template <std::size_t Count>
    struct Tree {
        std::array<J3DJoint, Count> joints;
        std::array<J3DJoint*, Count> joint_pointers{};
        Mtx matrices[Count]{};
        std::array<u8, Count> scale_flags{};
        J3DMtxBuffer buffer;
        J3DJointTree tree;

        explicit Tree(J3DMtxCalc& calculator, Trace* trace = nullptr) {
            for (std::size_t index = 0; index < Count; ++index) {
                joints[index].mJntNo = static_cast<u16>(index);
                joint_pointers[index] = &joints[index];
                scale_flags[index] = 0xFF;
                if (trace != nullptr) {
                    joints[index].mCallBackUserData = trace;
                    joints[index].setCallBack(callback);
                }
            }
            tree.mRootNode = &joints[0];
            tree.mJointNodePointer = joint_pointers.data();
            tree.mJointNum = static_cast<u16>(Count);
            tree.setBasicMtxCalc(&calculator);
            buffer.mJointTree = &tree;
            buffer.mpScaleFlagArr = scale_flags.data();
            buffer.mpAnmMtx = matrices;
            buffer.mpUserAnmMtx = matrices;
        }

        void calculate(const Vec& scale = {1, 1, 1}, const Matrix& matrix = Identity) {
            Mtx original_matrix;
            assign(original_matrix, matrix);
            tree.calc(&buffer, scale, original_matrix);
        }

        void check(std::size_t index, const Matrix& expected, std::string_view message) {
            matrix_near(snapshot(matrices[index]), expected, message);
        }
    };

    J3DTransformInfo transform(const Vec& scale, const Vec& translation, s16 z_rotation = 0) {
        return {scale, {0, 0, z_rotation}, translation};
    }

    void test_tree_initialization_and_empty_root() {
        Globals globals;
        Trace trace;
        RecordingCalculator<Basic> basic(trace, "basic");
        RecordingCalculator<Softimage> softimage(trace, "softimage");
        RecordingCalculator<Maya> maya(trace, "maya");
        const Matrix base{0, -1, 0, 5, 1, 0, 0, 6, 0, 0, 1, 7};
        const Matrix scaled{0, -3, 0, 5, 2, 0, 0, 6, 0, 0, 4, 7};
        J3DJoint marker;
        for (auto* calculator : std::array<J3DMtxCalc*, 3>{&basic, &softimage, &maya}) {
            Tree<1> fixture(*calculator);
            fixture.tree.mRootNode = nullptr;
            J3DJoint::mCurrentMtxCalc = &basic;
            J3DMtxCalc::setJoint(&marker);
            J3DSys::mParentS = {7, 8, 9};
            fixture.calculate({2, 3, 4}, base);
            require(J3DMtxCalc::getMtxBuffer() == &fixture.buffer && J3DMtxCalc::getJoint() == &marker,
                    "an empty tree selects its buffer without calculating a joint");
            require(J3DJoint::mCurrentMtxCalc == &basic && trace.calls.empty(),
                    "an empty tree does not replace the current joint calculator or invoke it");
            vector_near(J3DSys::mCurrentS, {2, 3, 4}, "each initializer retains model scale");
            vector_near(J3DSys::mParentS, calculator == &softimage ? Vec{7, 8, 9} : Vec{1, 1, 1},
                        "only Basic/Maya reset the parent scale during initialization");
            matrix_near(snapshot(J3DSys::mCurrentMtx), calculator == &softimage ? base : scaled,
                        "only Basic/Maya bake model scale into the initial current matrix");
            require(fixture.scale_flags[0] == 0xFF, "an empty tree leaves joint output storage untouched");
        }
        require(basic.initializations == 1 && softimage.initializations == 1 && maya.initializations == 1,
                "tree.calc initializes the basic calculator even when no root exists");
    }

    void test_inherited_override_and_subtree_restoration() {
        Globals globals;
        Trace trace;
        RecordingCalculator<Basic> basic(trace, "basic");
        RecordingCalculator<Basic> override(trace, "override");
        Tree<5> fixture(basic, &trace);
        auto& joint = fixture.joints;
        joint[0].appendChild(&joint[1]);
        joint[0].appendChild(&joint[3]);
        joint[1].appendChild(&joint[2]);
        joint[0].setYounger(&joint[4]);
        joint[1].setMtxCalc(&override);
        joint[0].setTransformInfo(transform({2, 3, 4}, {10, 0, 0}));
        joint[1].setTransformInfo(transform({5, 7, 11}, {0, 2, 0}));
        joint[2].setTransformInfo(transform({1, 1, 1}, {0, 0, 3}));
        joint[3].setTransformInfo(transform({1, 1, 1}, {4, 0, 0}));
        joint[4].setTransformInfo(transform({1, 1, 1}, {0, 5, 0}));
        const Matrix base{1, 0, 0, 100, 0, 1, 0, 0, 0, 0, 1, 0};
        fixture.calculate({1, 1, 1}, base);
        const Matrix root{2, 0, 0, 110, 0, 3, 0, 0, 0, 0, 4, 0};
        const Matrix child{10, 0, 0, 110, 0, 21, 0, 6, 0, 0, 44, 0};
        fixture.check(0, root, "root transform");
        fixture.check(1, child, "child inherits root transform");
        fixture.check(2, {10, 0, 0, 110, 0, 21, 0, 6, 0, 0, 44, 132}, "grandchild inherits overridden subtree");
        fixture.check(3, {2, 0, 0, 118, 0, 3, 0, 0, 0, 0, 4, 0}, "child sibling restores root scale and matrix");
        fixture.check(4, {1, 0, 0, 100, 0, 1, 0, 5, 0, 0, 1, 0}, "root sibling restores model scale and matrix");
        require(trace.calls == std::vector<std::string>{
                    "basic0", "callback0:0", "override1", "callback1:0", "override2", "callback2:0", "callback2:1",
                    "callback1:1", "basic3", "callback3:0", "callback3:1", "callback0:1", "basic4", "callback4:0", "callback4:1"},
                "original recursion must preserve calc/phase-zero/children/phase-one/younger ordering");
        require(basic.initializations == 1 && override.initializations == 0,
                "a per-joint override inherits traversal state without running its initializer");
        require(observed(trace, 2, 0).calculator == &override && observed(trace, 1, 1).calculator == &basic,
                "the override applies to descendants and is restored before phase one");
        matrix_near(observed(trace, 1, 0).matrix, child, "phase zero observes the calculated joint matrix");
        matrix_near(observed(trace, 1, 1).matrix, root, "phase one observes the restored incoming matrix");
        vector_near(observed(trace, 1, 1).current_scale, {2, 3, 4}, "phase one restores incoming accumulated scale");
        require(observed(trace, 1, 1).calculator_joint == &joint[2] && J3DMtxCalc::getJoint() == &joint[4],
                "recursiveCalc does not restore the static calculator joint pointer");
        matrix_near(snapshot(J3DSys::mCurrentMtx), base, "ordinary traversal restores the model matrix");
        vector_near(J3DSys::mCurrentS, {1, 1, 1}, "ordinary traversal restores the model scale");
    }

    void test_original_callback_matrix_effects_and_cached_function() {
        Globals globals;
        Trace trace;
        trace.mutate_matrices = true;
        Basic basic;
        Tree<4> fixture(basic, &trace);
        auto& joint = fixture.joints;
        joint[0].appendChild(&joint[1]);
        joint[0].appendChild(&joint[2]);
        joint[0].setYounger(&joint[3]);
        joint[0].setTransformInfo(transform({1, 1, 1}, {1, 0, 0}));
        joint[1].setTransformInfo(transform({1, 1, 1}, {0, 2, 0}));
        joint[2].setTransformInfo(transform({1, 1, 1}, {0, 3, 0}));
        joint[3].setTransformInfo(transform({1, 1, 1}, {0, 0, 4}));
        fixture.calculate();
        fixture.check(0, {1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0}, "a callback does not implicitly rewrite the stored joint output");
        fixture.check(1, {1, 0, 0, 101, 0, 1, 0, 2, 0, 0, 1, 0}, "phase-zero current-matrix changes reach children");
        fixture.check(2, {1, 0, 0, 101, 0, 1, 0, 53, 0, 0, 1, 0}, "phase-one restored-matrix changes reach younger siblings");
        fixture.check(3, {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 11}, "root phase-one changes reach the next root sibling");
        require(trace.replacement_calls == 0 && joint[1].getCallBack() == replacement_callback,
                "phase one uses the cached callback even when phase zero replaces the joint callback");
        require(observed(trace, 1, 1).matrix[7] == 0 && observed(trace, 0, 1).matrix == Identity,
                "both phase-one callbacks run after the corresponding incoming matrix was restored");
    }

    enum class ScaleMode { Basic, Softimage, Maya, MayaCompensated };

    void test_scale_modes_and_maya_segment_compensation() {
        Globals globals;
        Basic basic;
        Softimage softimage;
        Maya maya;
        // Retail JMath::fastReciprocal is bare fres: powers of two retain
        // this estimate factor, including reciprocal(1), rather than 1/x.
        constexpr float reciprocal_factor = 0.9998779296875F;  // 0x3F7FF800
        constexpr float reciprocal_factor_squared = 0.999755859375F;
        for (ScaleMode mode : {ScaleMode::Basic, ScaleMode::Softimage, ScaleMode::Maya, ScaleMode::MayaCompensated}) {
            J3DMtxCalc& calculator = mode == ScaleMode::Basic ? static_cast<J3DMtxCalc&>(basic)
                                    : mode == ScaleMode::Softimage ? static_cast<J3DMtxCalc&>(softimage)
                                                                 : static_cast<J3DMtxCalc&>(maya);
            Trace trace;
            Tree<4> fixture(calculator, &trace);
            auto& joint = fixture.joints;
            joint[0].appendChild(&joint[1]);
            joint[0].appendChild(&joint[3]);
            joint[1].appendChild(&joint[2]);
            joint[0].setTransformInfo(transform({2, 4, 8}, {10, 20, 30}, 0x4000));
            joint[1].setTransformInfo(transform({4, 2, 8}, {1, 2, 3}, 0x4000));
            joint[2].setTransformInfo(transform({1, 1, 1}, {1, 1, 1}));
            joint[3].setTransformInfo(transform({1, 1, 1}, {1, 2, 3}));
            const bool compensated = mode == ScaleMode::MayaCompensated;
            joint[1].mScaleCompensate = joint[2].mScaleCompensate = joint[3].mScaleCompensate = compensated ? 1 : 0;
            fixture.calculate();
            fixture.check(0, {0, -4, 0, 10, 2, 0, 0, 20, 0, 0, 8, 30}, "each mode emits the same root transform");
            if (mode == ScaleMode::Softimage) {
                fixture.check(1, {-8, 0, 0, 2, 0, -8, 0, 22, 0, 0, 64, 54}, "Softimage applies cumulative scale after rotation concatenation");
                fixture.check(2, {-8, 0, 0, -6, 0, -8, 0, 14, 0, 0, 64, 118}, "Softimage scales translation by inherited accumulated scale");
                matrix_near(observed(trace, 1, 0).matrix, {-1, 0, 0, 2, 0, -1, 0, 22, 0, 0, 1, 54},
                            "Softimage keeps scale outside its current traversal matrix");
            } else if (compensated) {
                fixture.check(1, {-4 * reciprocal_factor, 0, 0, 2, 0, -2 * reciprocal_factor, 0, 22,
                                  0, 0, 8 * reciprocal_factor, 54},
                              "Maya row compensation retains the retail reciprocal estimate before concatenation");
                fixture.check(2, {-reciprocal_factor_squared, 0, 0, 2 - 4 * reciprocal_factor,
                                  0, -reciprocal_factor_squared, 0, 22 - 2 * reciprocal_factor,
                                  0, 0, reciprocal_factor_squared, 54 + 8 * reciprocal_factor},
                              "Maya uses the direct parent's scale and leaves translation uncompensated");
            } else {
                fixture.check(1, {-16, 0, 0, 2, 0, -4, 0, 22, 0, 0, 64, 54}, "Basic/uncompensated Maya concatenate local scaled transforms");
                fixture.check(2, {-16, 0, 0, -14, 0, -4, 0, 18, 0, 0, 64, 118}, "Basic/uncompensated Maya inherit the scaled parent basis");
            }
            fixture.check(3, compensated ? Matrix{0, -reciprocal_factor, 0, 2, reciprocal_factor, 0, 0, 22,
                                                 0, 0, reciprocal_factor, 54}
                                         : Matrix{0, -4, 0, 2, 2, 0, 0, 22, 0, 0, 8, 54},
                          "a sibling restores its direct parent's matrix and scale state");
            const bool maya_mode = mode == ScaleMode::Maya || compensated;
            require(fixture.scale_flags == std::array<u8, 4>{0, 0, static_cast<u8>(maya_mode), static_cast<u8>(maya_mode)},
                    "Maya scale flags report local scale while Basic/Softimage report accumulated scale");
            if (maya_mode) {
                vector_near(observed(trace, 2, 1).parent_scale, {4, 2, 8}, "grandchild phase one restores direct-parent scale");
                vector_near(observed(trace, 1, 1).parent_scale, {2, 4, 8}, "child phase one restores root local scale");
                vector_near(observed(trace, 3, 0).parent_scale, {1, 1, 1}, "Maya sets parent scale to the sibling's local scale");
                vector_near(observed(trace, 2, 0).current_scale, {1, 1, 1}, "Maya does not accumulate current scale during calc");
            }
        }
    }

    void test_accumulated_scale_cancellation_and_nullable_calculator() {
        Globals globals;
        Basic basic;
        Softimage softimage;
        Maya maya;
        for (auto* calculator : std::array<J3DMtxCalc*, 3>{&basic, &softimage, &maya}) {
            Tree<2> fixture(*calculator);
            fixture.joints[0].appendChild(&fixture.joints[1]);
            fixture.joints[0].setTransformInfo(transform({2, 2, 2}, {0, 0, 0}));
            fixture.joints[1].setTransformInfo(transform({0.5F, 0.5F, 0.5F}, {1, 1, 1}));
            fixture.calculate();
            const float basis = calculator == &basic ? 2 : 1;
            fixture.check(1, {basis, 0, 0, 2, 0, basis, 0, 2, 0, 0, basis, 2},
                          "Basic skips local scale when accumulated scale cancels exactly to one");
            require(fixture.scale_flags[1] == (calculator == &maya ? 0 : 1),
                    "cancellation follows each original mode's distinct scale-flag predicate");
        }

        Tree<1> fixture(basic);
        fixture.joints[0].setMtxCalc(&basic);
        Mtx identity;
        assign(identity, Identity);
        basic.init({1, 1, 1}, identity);
        J3DMtxCalc::setMtxBuffer(&fixture.buffer);
        J3DJoint::mCurrentMtxCalc = nullptr;
        fixture.joints[0].recursiveCalc();
        require(J3DJoint::mCurrentMtxCalc == &basic,
                "an override with no previous calculator remains current because original restoration checks for nonnull");
    }

    struct OwnerInput {
        smgpc::render::J3dInfoSummary info;
        smgpc::render::J3dJointBlockSummary joints;
    };

    OwnerInput owner_input() {
        OwnerInput input;
        input.info.flags = 0xA0;
        // Material/shape entries preserve current joint1 before its child
        // scope opens. The following joint3 remains a sibling under joint0.
        input.info.hierarchy = {{0x10, 0}, {1, 0}, {0x10, 1}, {0x11, 0}, {0x12, 0}, {1, 0},
                                {0x10, 2}, {2, 0}, {0x11, 1}, {0x12, 1}, {0x10, 3}, {2, 0}, {0, 0}};
        input.joints.joint_count = 4;
        input.joints.joints.resize(4);
        // Parsed joint summaries have already applied the file's remap. The
        // owner must retain their order and reconstruct parents from INF1.
        input.joints.remap_table = {3, 2, 1, 0};
        input.joints.parent_indices = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
        input.joints.joints[0].translation = {10, 0, 0};
        input.joints.joints[1].translation = {0, 2, 0};
        input.joints.joints[2].translation = {0, 0, 3};
        input.joints.joints[3].translation = {4, 0, 0};
        input.joints.joints[1].kind = 0x1234;
        input.joints.joints[1].scale_compensate = 0xFF;
        input.joints.joints[1].radius = 123;
        input.joints.joints[1].min = {-1, -2, -3};
        input.joints.joints[1].max = {4, 5, 6};
        input.joints.joints[2].scale_compensate = 1;
        return input;
    }

    void check_bind_pose(std::span<const smgpc::render::J3dMatrix3x4> matrices) {
        require(matrices.size() == 4, "the owner retains every decoded joint");
        matrix_near(matrices[0].m, {1, 0, 0, 10, 0, 1, 0, 0, 0, 0, 1, 0}, "owned root bind pose");
        matrix_near(matrices[1].m, {1, 0, 0, 10, 0, 1, 0, 2, 0, 0, 1, 0}, "owned child bind pose");
        matrix_near(matrices[2].m, {1, 0, 0, 10, 0, 1, 0, 2, 0, 0, 1, 3}, "material/shape scope retains the actual grandchild parent");
        matrix_near(matrices[3].m, {1, 0, 0, 14, 0, 1, 0, 0, 0, 0, 1, 0}, "owned sibling restores its parent scope");
    }

    void test_owner_hierarchy_storage_and_original_mode_selection() {
        Globals globals;
        auto owner = [] {
            auto input = owner_input();
            auto result = std::make_unique<smgpc::compat::OriginalJ3dJointTree>(input.info, input.joints);
            input.info.hierarchy.assign(32, {0, 0});
            input.joints.joints.assign(12, {});
            return result;
        }();
        auto& tree = owner->joint_tree();
        auto* root = tree.getRootNode();
        auto* child = tree.getJointNodePointer(1);
        require(root == tree.getJointNodePointer(0) && root->getChild() == child &&
                    child->getChild() == tree.getJointNodePointer(2) && child->getYounger() == tree.getJointNodePointer(3),
                "owned original joint links retain INF1 parent/current scopes after source summaries retire");
        require(tree.mHierarchy[3].mType == 0x11 && tree.mHierarchy[4].mType == 0x12 && tree.mHierarchy[5].mType == 1 &&
                    tree.mHierarchy[12].mType == 0,
                "the owner retains its complete original hierarchy independently of source storage");
        require(child->getJntNo() == 1 && child->mKind == 0x34 && child->getScaleCompensate() == 0 &&
                    tree.getJointNodePointer(2)->getScaleCompensate() == 1,
                "decoded fields preserve original joint indexing, kind narrowing and scale-compensation sentinel handling");
        near(child->getRadius(), 123, "the original bounding radius survives source retirement");
        vector_near(*child->getMin(), {-1, -2, -3}, "the original minimum survives source retirement");
        vector_near(*child->getMax(), {4, 5, 6}, "the original maximum survives source retirement");
        require(owner->matrix_buffer().getJointTree() == &tree &&
                    owner->matrix_buffer().mpUserAnmMtx == owner->matrix_buffer().mpAnmMtx,
                "owned animation storage uses the original matrix-buffer relationship");
        check_bind_pose(owner->calculate(nullptr, 0));

        for (std::uint16_t mode : {std::uint16_t{0}, std::uint16_t{1}, std::uint16_t{2}}) {
            auto input = owner_input();
            input.info.flags = 0xA0 | mode;
            input.joints.joints[0].scale = {2, 4, 8};
            input.joints.joints[0].rotation = {0, 0, 0x4000};
            input.joints.joints[0].translation = {10, 20, 30};
            input.joints.joints[1].scale = {4, 2, 8};
            input.joints.joints[1].rotation = {0, 0, 0x4000};
            input.joints.joints[1].translation = {1, 2, 3};
            smgpc::compat::OriginalJ3dJointTree original(input.info, input.joints);
            auto* calculator = original.joint_tree().getBasicMtxCalc();
            require(original.joint_tree().mFlags == input.info.flags &&
                        (mode == 0 ? dynamic_cast<Basic*>(calculator) != nullptr :
                         mode == 1 ? dynamic_cast<Softimage*>(calculator) != nullptr : dynamic_cast<Maya*>(calculator) != nullptr),
                    "INF1 low flag bits select the original Basic/Softimage/Maya calculator while preserving all flags");
            const auto matrices = original.calculate(nullptr, 0);
            matrix_near(matrices[1].m, mode == 1 ? Matrix{-8, 0, 0, 2, 0, -8, 0, 22, 0, 0, 64, 54}
                                                : Matrix{-16, 0, 0, 2, 0, -4, 0, 22, 0, 0, 64, 54},
                        "the owner's selected original mode governs its actual output matrices");
        }
    }

    void check_globals(const Globals& expected) {
        require(J3DMtxCalc::getMtxBuffer() == expected.buffer && J3DMtxCalc::getJoint() == expected.joint &&
                    J3DJoint::mCurrentMtxCalc == expected.calculator &&
                    j3dSys.mCurrentMtxCalc == expected.system_calculator,
                "the owner restores every outer J3D traversal pointer");
        require(snapshot(J3DSys::mCurrentMtx) == expected.matrix,
                "the owner restores every outer current-matrix bit");
        vector_near(J3DSys::mCurrentS, expected.current_scale, "the owner restores outer current scale");
        vector_near(J3DSys::mParentS, expected.parent_scale, "the owner restores outer parent scale");
    }

    int throwing_callback(J3DJoint*, int phase) {
        if (phase == 0) {
            assign(J3DSys::mCurrentMtx, {});
            J3DSys::mCurrentS = {-2, -3, -4};
            J3DSys::mParentS = {-5, -6, -7};
            J3DMtxCalc::setMtxBuffer(nullptr);
            J3DMtxCalc::setJoint(nullptr);
            J3DJoint::mCurrentMtxCalc = nullptr;
            j3dSys.mCurrentMtxCalc = nullptr;
            throw std::runtime_error("deliberate joint callback exception");
        }
        return 0;
    }

    int observe_original_core(J3DJoint* joint, int phase) {
        if (phase == 0) {
            auto* core = dynamic_cast<XanimeCore*>(J3DJoint::mCurrentMtxCalc);
            require(core != nullptr && j3dSys.mCurrentMtxCalc == core,
                    "renderer traversal invokes the actual game animation core and publishes its original system pointer");
            require(core->mTrackCount == 1 && core->_6 == 0 && core->mTrackList[0]._0 != nullptr,
                    "the real core consumes a bound original transform animation in its default calculation mode");
            near(core->mTrackList[0]._0->getFrame(), *static_cast<float*>(joint->mCallBackUserData),
                 "the core samples the frame owned by this calculation");
        }
        return 0;
    }

    void test_owner_raw_animation_and_exception_cleanup() {
        Globals restore_outer;
        Basic outer_calculator;
        J3DJoint outer_joint;
        J3DMtxBuffer outer_buffer;
        assign(J3DSys::mCurrentMtx, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
        J3DSys::mCurrentS = {13, 14, 15};
        J3DSys::mParentS = {16, 17, 18};
        J3DMtxCalc::setJoint(&outer_joint);
        J3DMtxCalc::setMtxBuffer(&outer_buffer);
        J3DJoint::mCurrentMtxCalc = &outer_calculator;
        j3dSys.mCurrentMtxCalc = &outer_calculator;
        const Globals expected;
        auto input = owner_input();
        smgpc::compat::OriginalJ3dJointTree owner(input.info, input.joints);
        auto* basic = owner.joint_tree().getBasicMtxCalc();
        {
            // A real original Key object with valid native tables. Its caller
            // owns all backing arrays and lends them only for calculate().
            J3DAnmTransformKey animation;
            std::array<J3DAnmTransformKeyTable, 12> tables{};
            std::array<float, 6> translation{0, 20, 5, 4, 40, 5};
            animation.field_0x1e = 4;
            animation.mAnmTable = tables.data();
            animation.mTransData = translation.data();
            tables[0].mTranslateInfo = {2, 0, 0};
            animation.setFrame(67);
            animation.mAttribute = 2;
            animation.mFrameMax = 4;
            for (const auto [frame, x] : std::array<std::array<float, 2>, 3>{{{-1, 20}, {1, 25}, {4, 40}}}) {
                auto expected_frame = frame;
                owner.joint_tree().getRootNode()->mCallBackUserData = &expected_frame;
                owner.joint_tree().getRootNode()->setCallBack(observe_original_core);
                const auto matrices = owner.calculate(&animation, frame);
                owner.joint_tree().getRootNode()->mCallBackUserData = nullptr;
                for (const auto& matrix : matrices) {
                    near(matrix.m[3], x, "owner sampling consumes raw frames without adding renderer loop policy");
                }
                require(animation.getFrame() == 67 && owner.joint_tree().getBasicMtxCalc() == basic,
                        "borrowed animation frame remains unchanged and the tree's basic calculator is restored");
                check_globals(expected);
            }

            owner.joint_tree().getRootNode()->setCallBack(throwing_callback);
            bool caught = false;
            try {
                static_cast<void>(owner.calculate(&animation, 1));
            } catch (const std::runtime_error& error) {
                caught = std::string_view(error.what()) == "deliberate joint callback exception";
            }
            require(caught && owner.joint_tree().getBasicMtxCalc() == basic,
                    "callback exceptions propagate while restoring the basic calculator and borrowed animation scope");
            check_globals(expected);
            owner.joint_tree().getRootNode()->setCallBack(nullptr);
        }
        check_bind_pose(owner.calculate(nullptr, 0));
        check_globals(expected);
    }

    struct NestedOwner {
        smgpc::compat::OriginalJ3dJointTree* nested;
        unsigned int calls = 0;
    };

    int nested_owner_callback(J3DJoint* joint, int phase) {
        if (phase == 0) {
            auto& nested = *static_cast<NestedOwner*>(joint->mCallBackUserData);
            ++nested.calls;
            const Globals current;
            check_bind_pose(nested.nested->calculate(nullptr, 0));
            check_globals(current);
        }
        return 0;
    }

    void test_owner_nested_traversal_and_rejected_self_reentry() {
        Globals globals;
        auto input = owner_input();
        smgpc::compat::OriginalJ3dJointTree first(input.info, input.joints);
        smgpc::compat::OriginalJ3dJointTree second(input.info, input.joints);
        NestedOwner nested{&second};
        auto* root = first.joint_tree().getRootNode();
        root->mCallBackUserData = &nested;
        root->setCallBack(nested_owner_callback);
        const Globals outer;
        check_bind_pose(first.calculate(nullptr, 0));
        require(nested.calls == 1, "a callback can calculate another actual joint owner once");
        check_globals(outer);
        nested.nested = &first;
        bool caught = false;
        try {
            static_cast<void>(first.calculate(nullptr, 0));
        } catch (const std::logic_error&) {
            caught = true;
        }
        require(caught && nested.calls == 2, "same-owner reentry is rejected before recursive adapter mutation");
        check_globals(outer);
        root->setCallBack(nullptr);
        check_bind_pose(first.calculate(nullptr, 0));
        check_globals(outer);
    }

    void test_owner_base_transform_and_scale_reach_original_calculators() {
        Globals globals;
        const smgpc::render::J3dMatrix3x4 base{{0, -1, 0, 100, 1, 0, 0, 200, 0, 0, 1, 300}};
        const std::array<float, 3> base_scale{2, 4, 8};
        for (std::uint16_t mode : {std::uint16_t{0}, std::uint16_t{1}, std::uint16_t{2}}) {
            for (bool cancel_scale : {false, true}) {
                auto input = owner_input();
                input.info.flags = 0xA0 | mode;
                const std::array<float, 3> root_scale = cancel_scale ? std::array<float, 3>{0.5F, 0.25F, 0.125F}
                                                                   : std::array<float, 3>{3, 5, 7};
                input.joints.joints[0].scale = root_scale;
                input.joints.joints[0].rotation = {0x4000, 0, 0};
                input.joints.joints[0].translation = {1, 2, 3};
                input.joints.joints[1].scale = {3, 5, 7};
                input.joints.joints[1].rotation = {0, 0x4000, 0};
                input.joints.joints[1].translation = {4, 5, 6};
                smgpc::compat::OriginalJ3dJointTree owner(input.info, input.joints);

                // The same authored local transforms can come from JNT1 or an
                // original Key sampler. Animated traversal uses XanimeCore's
                // own initialization and scale rules, including its distinct
                // Softimage behavior; bind traversal uses original J3D NoAnm.
                J3DAnmTransformKey animation;
                std::array<J3DAnmTransformKeyTable, 12> tables{};
                std::array<float, 6> scales{root_scale[0], root_scale[1], root_scale[2], 3, 5, 7};
                std::array<s16, 6> rotations{0x4000, 0, 0, 0, 0x4000, 0};
                std::array<float, 6> translations{1, 2, 3, 4, 5, 6};
                for (u16 axis = 0; axis < 6; ++axis) {
                    tables[axis].mScaleInfo = {1, axis, 0};
                    tables[axis].mRotationInfo = {1, axis, 0};
                    tables[axis].mTranslateInfo = {1, axis, 0};
                }
                animation.field_0x1e = 4;
                animation.mAnmTable = tables.data();
                animation.mScaleData = scales.data();
                animation.mRotData = rotations.data();
                animation.mTransData = translations.data();
                for (const auto* motion : std::array<const J3DAnmTransformKey*, 2>{nullptr, &animation}) {
                    const auto matrices = owner.calculate(motion, 0, base, base_scale);
                    Matrix root;
                    if (!cancel_scale) {
                        root = mode == 1 ? Matrix{0, 0, 56, 92, 6, 0, 0, 202, 0, 20, 0, 324}
                                         : Matrix{0, 0, 28, 92, 6, 0, 0, 202, 0, 40, 0, 324};
                    } else if (mode == 0) {
                        root = {0, 0, 4, 92, 2, 0, 0, 202, 0, 8, 0, 324};
                    } else if (mode == 1) {
                        root = {0, 0, 1, 92, 1, 0, 0, 202, 0, 1, 0, 324};
                    } else {
                        root = {0, 0, 0.5F, 92, 1, 0, 0, 202, 0, 2, 0, 324};
                    }
                    if (motion != nullptr && mode == 1) {
                        // Core uses Maya initialization for all three modes.
                        // Its SI path scales the local matrix and translation
                        // before concatenation, then scales the stored output.
                        root = cancel_scale ? Matrix{0, 0, 0.5F, 68, 1, 0, 0, 204, 0, 2, 0, 492}
                                            : Matrix{0, 0, 1568, 68, 36, 0, 0, 204, 0, 800, 0, 492};
                    }
                    matrix_near(matrices[0].m, root,
                                "caller base translation, rotation and separate nonuniform scale reach the original selected mode");
                    require(owner.matrix_buffer().getScaleFlag(0) == (cancel_scale && mode != 2 ? 1 : 0),
                            "the actual base scale participates in original accumulated/local scale predicates");
                    if (cancel_scale) {
                        const Matrix child = motion != nullptr && mode == 1 ? Matrix{-4.5F, 0, 0, 71, 0, 0, 49, 208, 0, 50, 0, 502}
                                             : mode == 0 ? Matrix{-12, 0, 0, 116, 0, 0, 14, 210, 0, 40, 0, 364}
                                             : mode == 1 ? Matrix{-3, 0, 0, 98, 0, 0, 7, 206, 0, 5, 0, 329}
                                                         : Matrix{-1.5F, 0, 0, 95, 0, 0, 7, 206, 0, 10, 0, 334};
                        matrix_near(matrices[1].m, child,
                                    "children inherit the mode-specific base rotation and scale state after cancellation");
                    }
                }
            }
        }
    }

}  // namespace

int main() {
    try {
        test_tree_initialization_and_empty_root();
        test_inherited_override_and_subtree_restoration();
        test_original_callback_matrix_effects_and_cached_function();
        test_scale_modes_and_maya_segment_compensation();
        test_accumulated_scale_cancellation_and_nullable_calculator();
        test_owner_hierarchy_storage_and_original_mode_selection();
        test_owner_raw_animation_and_exception_cleanup();
        test_owner_nested_traversal_and_rejected_self_reentry();
        test_owner_base_transform_and_scale_reach_original_calculators();
        std::cout << "9/9 original J3D joint-traversal groups passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[fail] " << error.what() << '\n';
        return 1;
    }
}
