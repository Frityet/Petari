#include "Game/Enemy/AnimScaleController.hpp"
#include "Game/NPC/NPCActor.hpp"
#include "Game/Util/JointController.hpp"
#include "Game/Util/JointUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "JSystem/J3DGraphAnimator/J3DJoint.hpp"
#include "JSystem/J3DGraphAnimator/J3DJointTree.hpp"
#include "JSystem/J3DGraphAnimator/J3DModel.hpp"
#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/J3DGraphAnimator/J3DMtxBuffer.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "SceneExecutionFixture.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/J3dCommandScope.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/ResourceHolderCompat.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "runtime/RuntimeServices.hpp"

#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace aurora { extern AuroraConfig g_config; }

namespace {
    using Matrix = std::array<float, 12>;
    using Basic = J3DMtxCalcNoAnm<J3DMtxCalcCalcTransformBasic, J3DMtxCalcJ3DSysInitBasic>;
    constexpr Matrix identity{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};

    void require(bool condition, std::string_view message) {
        if (!condition) {
            smgpc::compat::JkrHostAllocationScope host;
            throw std::runtime_error(std::string(message));
        }
    }

    Matrix snapshot(const Mtx matrix) {
        Matrix result;
        for (int row = 0; row < 3; ++row)
            for (int column = 0; column < 4; ++column)
                result[row * 4 + column] = matrix[row][column];
        return result;
    }

    void assign(Mtx matrix, const Matrix& value) {
        for (int row = 0; row < 3; ++row)
            for (int column = 0; column < 4; ++column)
                matrix[row][column] = value[row * 4 + column];
    }

    void matrix_near(const Matrix& actual, const Matrix& expected, std::string_view message) {
        for (std::size_t index = 0; index < actual.size(); ++index)
            require(std::isfinite(actual[index]) && std::fabs(actual[index] - expected[index]) < 0.002F,
                    message);
    }

    struct Globals {
        J3DSys system = j3dSys;
        Matrix matrix = snapshot(J3DSys::mCurrentMtx);
        Vec scale = J3DSys::mCurrentS;
        Vec parent = J3DSys::mParentS;
        J3DJoint* joint = J3DMtxCalc::getJoint();
        J3DMtxBuffer* buffer = J3DMtxCalc::getMtxBuffer();
        J3DMtxCalc* calculator = J3DJoint::mCurrentMtxCalc;
        ~Globals() {
            j3dSys = system;
            assign(J3DSys::mCurrentMtx, matrix);
            J3DSys::mCurrentS = scale;
            J3DSys::mParentS = parent;
            J3DMtxCalc::setJoint(joint);
            J3DMtxCalc::setMtxBuffer(buffer);
            J3DJoint::mCurrentMtxCalc = calculator;
        }
    };

    // Complete SDK inputs for JointController's matrix operations; no Game
    // ModelManager or actor is substituted in these isolated traversal cases.
    struct Tree {
        Basic calculator;
        std::array<J3DJoint, 5> joints;
        std::array<J3DJoint*, 5> pointers;
        std::array<u8, 5> scale_flags{};
        Mtx matrices[5]{};
        J3DJointTree tree;
        J3DMtxBuffer buffer;
        J3DModel model;

        Tree() {
            for (u16 index = 0; index < joints.size(); ++index) {
                joints[index].mJntNo = index;
                pointers[index] = &joints[index];
            }
            joints[0].appendChild(&joints[1]);
            joints[1].appendChild(&joints[2]);
            joints[0].appendChild(&joints[3]);
            joints[0].setYounger(&joints[4]);
            joints[0].mTransformInfo.mTranslate = {1, 0, 0};
            joints[1].mTransformInfo.mTranslate = {0, 2, 0};
            joints[2].mTransformInfo.mTranslate = {0, 0, 3};
            joints[3].mTransformInfo.mTranslate = {0, 4, 0};
            joints[4].mTransformInfo.mTranslate = {0, 0, 5};
            tree.mRootNode = &joints[0];
            tree.mJointNodePointer = pointers.data();
            tree.mJointNum = joints.size();
            tree.setBasicMtxCalc(&calculator);
            buffer.mJointTree = &tree;
            buffer.mpScaleFlagArr = scale_flags.data();
            buffer.mpAnmMtx = matrices;
            buffer.mpUserAnmMtx = matrices;
            model.mMtxBuffer = &buffer;
        }

        void calculate() {
            Mtx base;
            PSMTXIdentity(base);
            tree.calc(&buffer, Vec{1, 1, 1}, base);
        }
    };

    struct Host {
        bool accept_pre = false;
        bool accept_post = false;
        int pre_calls = 0;
        int post_calls = 0;
        JointController* expected_controller = nullptr;
        J3DJoint* expected_joint = nullptr;
        Matrix pre_input{};
        Matrix post_input{};

        void check(const JointControllerInfo& info) const {
            require(info.mController == expected_controller && info.mJoint == expected_joint,
                    "original dispatch supplies the exact controller and traversed joint");
        }
        bool pre(TPos3f* matrix, const JointControllerInfo& info) {
            check(info);
            ++pre_calls;
            pre_input = snapshot(matrix->toMtxPtr());
            matrix->toMtxPtr()[0][3] += 10;
            return accept_pre;
        }
        bool post(TPos3f* matrix, const JointControllerInfo& info) {
            check(info);
            ++post_calls;
            post_input = snapshot(matrix->toMtxPtr());
            matrix->toMtxPtr()[2][3] += 100;
            return accept_post;
        }
    };

    void test_base_and_direct_phase_contract() {
        Globals globals;
        Tree fixture;
        fixture.calculate();
        JointController base;
        require(base.mModel == nullptr && base.mJoint == nullptr,
                "the actual constructor starts with no borrowed model or joint");
        base.mModel = &fixture.model;
        base.mJoint = &fixture.joints[1];
        const auto stored = snapshot(fixture.matrices[1]);
        const Matrix current{2, 0, 0, 7, 0, 3, 0, 8, 0, 0, 4, 9};
        assign(J3DSys::mCurrentMtx, current);
        base.registerCallBack();
        require(base.mJoint->mCallBack == JointController::staticCallBack && base.mJoint->mCallBackUserData == &base,
                "registerCallBack publishes the actual callback and native-width controller pointer");
        require(JointController::staticCallBack(base.mJoint, 0) == 0,
                "phase zero preserves the original callback return value");
        matrix_near(snapshot(fixture.matrices[1]), stored, "base false-return preserves stored matrix");
        matrix_near(snapshot(J3DSys::mCurrentMtx), current, "base false-return preserves current matrix");
        require(JointController::staticCallBack(base.mJoint, 7) == 0 &&
                    base.mJoint->mCallBackUserData == &base && base.mJoint->mCallBack == JointController::staticCallBack,
                "a nonstandard phase neither invokes a matrix hook nor retires its registration");
        JointController::staticCallBack(base.mJoint, 1);
        matrix_near(snapshot(fixture.matrices[1]), stored, "base post false-return preserves stored matrix");
        matrix_near(snapshot(J3DSys::mCurrentMtx), current, "post callback never changes current matrix");
        require(base.mJoint->mCallBack == nullptr && base.mJoint->mCallBackUserData == nullptr,
                "phase one clears the callback and its borrowed controller pointer");
        require(JointController::staticCallBack(nullptr, 0) == 0 &&
                    JointController::staticCallBack(base.mJoint, 0) == 0,
                "the original null-joint and unregistered-joint cases are inert");
    }

    void test_delegator_acceptance_and_real_traversal() {
        Globals globals;
        for (bool accept_pre : {false, true}) {
            for (bool accept_post : {false, true}) {
                Tree fixture;
                Host host{.accept_pre = accept_pre, .accept_post = accept_post};
                JointControlDelegator<Host> controller(&host, &Host::pre, &Host::post);
                host.expected_controller = &controller;
                host.expected_joint = &fixture.joints[1];
                controller.mModel = &fixture.model;
                controller.mJoint = host.expected_joint;
                controller.registerCallBack();
                JointController::staticCallBack(host.expected_joint, -1);
                require(host.pre_calls == 0 && host.post_calls == 0,
                        "unknown phases do not dispatch either member-function pointer");
                fixture.calculate();
                const float x = accept_pre ? 11 : 1;
                const float z = accept_post ? 100 : 0;
                matrix_near(snapshot(fixture.matrices[1]), {1, 0, 0, x, 0, 1, 0, 2, 0, 0, 1, z},
                            "stored joint matrix accepts only hooks that return true");
                matrix_near(snapshot(fixture.matrices[2]), {1, 0, 0, x, 0, 1, 0, 2, 0, 0, 1, 3},
                            "accepted pre changes reach actual descendants, post changes occur after them");
                matrix_near(snapshot(fixture.matrices[3]), {1, 0, 0, 1, 0, 1, 0, 4, 0, 0, 1, 0},
                            "controller changes do not leak into the joint's sibling");
                matrix_near(snapshot(fixture.matrices[4]), {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 5},
                            "controller changes do not leak into the root's sibling");
                matrix_near(host.pre_input, {1, 0, 0, 1, 0, 1, 0, 2, 0, 0, 1, 0},
                            "pre hook begins with the model's newly calculated matrix");
                matrix_near(host.post_input, {1, 0, 0, x, 0, 1, 0, 2, 0, 0, 1, 0},
                            "post hook begins with the accepted stored pre result");
                require(host.pre_calls == 1 && host.post_calls == 1 &&
                            controller.mJoint->mCallBack == nullptr && controller.mJoint->mCallBackUserData == nullptr,
                        "both original delegated hooks run once and clear the shared joint registration");
                matrix_near(snapshot(J3DSys::mCurrentMtx), identity,
                            "post acceptance only writes animation storage, preserving restored traversal state");
                fixture.calculate();
                require(host.pre_calls == 1 && host.post_calls == 1,
                        "a second traversal does not reuse a retired callback");
            }
        }
        Tree fixture;
        Host host;
        JointControlDelegator<Host> empty(&host, nullptr, nullptr);
        empty.mModel = &fixture.model;
        empty.mJoint = &fixture.joints[1];
        empty.registerCallBack();
        fixture.calculate();
        require(host.pre_calls == 0 && host.post_calls == 0 && empty.mJoint->mCallBack == nullptr,
                "null delegated hooks retain false-return behavior through the actual traversal");
    }

    class NpcScaleBinding {
    public:
        explicit NpcScaleBinding(NPCActor& actor) : _actor(actor) {
            _scale = std::make_unique<AnimScaleController>(nullptr);
            _delegator.reset(MR::createJointDelegatorWithNullChildFunc(&actor, &NPCActor::calcJointScale, "Body"));
            actor.mScaleController = _scale.get();
            actor.mDelegator = _delegator.get();
        }
        ~NpcScaleBinding() {
            _actor.mDelegator = nullptr;
            _actor.mScaleController = nullptr;
        }
        void set(const TVec3f& scale) { _scale->_C = scale; }
    private:
        NPCActor& _actor;
        std::unique_ptr<AnimScaleController> _scale;
        std::unique_ptr<JointControlDelegator<NPCActor>> _delegator;
    };

    Matrix scaled_basis(Matrix matrix, const TVec3f& scale) {
        for (int row = 0; row < 3; ++row) {
            matrix[row * 4] *= scale.x;
            matrix[row * 4 + 1] *= scale.y;
            matrix[row * 4 + 2] *= scale.z;
        }
        return matrix;
    }

    bool test_real_npc_model_sharing() {
        const auto* disc = std::getenv("SMGPC_REAL_DISC");
        if (disc == nullptr || *disc == '\0') {
            std::cout << "[skip] real NPC model ownership requires SMGPC_REAL_DISC\n";
            return false;
        }
        require(aurora_dvd_open(disc), "the actual disc fixture must open");
        struct CloseDisc { ~CloseDisc() { aurora_dvd_close(); } } close;
        DVDInit();
        aurora::g_config.mem1Size = 24U << 20;
        smgpc::resource::GameResourceRuntime process;
        smgpc::runtime::DvdFileSystemService dvd("/");
        smgpc::runtime::SceneScheduler scheduler;
        smgpc::runtime::SceneSchedulerBinding scheduled(scheduler);
        const auto registered_before = smgpc::compat::name_obj_runtime_state_count();
        const auto actors_before = smgpc::compat::actor_runtime_state_count();
        for (int generation = 0; generation < 2; ++generation) {
            std::weak_ptr<smgpc::compat::JkrAllocationDomain> retired;
            {
                Globals globals;
                auto domain = smgpc::compat::JkrAllocationDomain::create(process.host_heaps(), 16U << 20);
                retired = domain;
                smgpc::test::SceneExecutionFixture execution(scheduler, domain);
                smgpc::compat::ResourceHolderService resources(dvd, domain, process.mem1_heap());
                smgpc::compat::JkrAllocationScope game(domain);
                smgpc::compat::J3dCommandScope commands;
                NPCActor first("First original joint-controller NPC");
                NPCActor second("Second original joint-controller NPC");
                first.initModelManagerWithAnm("Tico", nullptr, false);
                second.initModelManagerWithAnm("Tico", nullptr, false);
                first.mPosition.set(10, 20, 30);
                second.mPosition.set(-40, 50, 60);
                auto* first_model = MR::getJ3DModel(&first);
                auto* second_model = MR::getJ3DModel(&second);
                auto* joint = MR::getJoint(&first, "Body");
                require(first_model != second_model && first.mModelManager != second.mModelManager &&
                            first_model->getModelData() == second_model->getModelData() &&
                            first_model->mMtxBuffer != second_model->mMtxBuffer && joint == MR::getJoint(&second, "Body"),
                        "actual NPC owners share resource joints but retain distinct model and matrix storage");
                require(JKRHeap::findFromRoot(first.mModelManager) == &domain->heap() &&
                            JKRHeap::findFromRoot(second.mModelManager) == &domain->heap(),
                        "both complete original ModelManagers belong to the retained Game scene heap");
                NpcScaleBinding first_scale(first);
                NpcScaleBinding second_scale(second);
                JointController indexed;
                MR::setJointControllerParam(&indexed, &first, joint->mJntNo);
                require(indexed.mModel == first_model && indexed.mJoint == joint &&
                            first.mDelegator->mModel == first_model && first.mDelegator->mJoint == joint &&
                            second.mDelegator->mModel == second_model && second.mDelegator->mJoint == joint,
                        "named/indexed original binding preserves each actor's model and the shared joint identity");

                // Establish only the original scale-controller input. The
                // normal LiveActor calcAnmMtx virtual path registers the NPC
                // callback and executes its complete ModelManager traversal.
                first.calcAnmMtx();
                const auto first_unit = snapshot(first_model->getAnmMtx(joint->mJntNo));
                second.calcAnmMtx();
                const auto second_unit = snapshot(second_model->getAnmMtx(joint->mJntNo));
                const TVec3f first_value{2, 3, 4};
                first_scale.set(first_value);
                first.calcAnmMtx();
                const auto first_scaled = snapshot(first_model->getAnmMtx(joint->mJntNo));
                matrix_near(first_scaled, scaled_basis(first_unit, first_value),
                            "original NPC calcJointScale changes the actual model's stored joint basis");
                matrix_near(snapshot(second_model->getAnmMtx(joint->mJntNo)), second_unit,
                            "first NPC traversal cannot write the second NPC's matrix buffer");
                require(joint->mCallBack == nullptr && joint->mCallBackUserData == nullptr,
                        "completed NPC traversal retires its pointer from the shared model resource");
                const TVec3f second_value{0.5F, 1.5F, 2.5F};
                second_scale.set(second_value);
                second.calcAnmMtx();
                matrix_near(snapshot(second_model->getAnmMtx(joint->mJntNo)), scaled_basis(second_unit, second_value),
                            "the second original NPC registers and applies its own scale controller");
                matrix_near(snapshot(first_model->getAnmMtx(joint->mJntNo)), first_scaled,
                            "the second shared-resource traversal preserves the first actor's completed pose");
                first_scale.set(TVec3f{1, 1, 1});
                first.calcAnmMtx();
                matrix_near(snapshot(first_model->getAnmMtx(joint->mJntNo)), first_unit,
                            "returning to the first NPC applies fresh input rather than a stale shared callback");
                require(joint->mCallBack == nullptr && joint->mCallBackUserData == nullptr,
                        "all borrowed callback state clears before typed actor/model retirement");
            }
            require(retired.expired() && smgpc::compat::name_obj_runtime_state_count() == registered_before &&
                        smgpc::compat::actor_runtime_state_count() == actors_before && scheduler.snapshot().empty(),
                    "typed NPC/model/executor retirement releases registrations and its original scene heap");
        }
        return true;
    }
}

int main() try {
    test_base_and_direct_phase_contract();
    test_delegator_acceptance_and_real_traversal();
    const bool real = test_real_npc_model_sharing();
    std::cout << "Original JointController: 2/2 SDK groups passed; real NPC resource group "
              << (real ? "passed in 2 scene generations" : "skipped") << '\n';
    return 0;
} catch (const std::exception& error) {
    std::cerr << "[fail] original JointController: " << error.what() << '\n';
    return 1;
}
