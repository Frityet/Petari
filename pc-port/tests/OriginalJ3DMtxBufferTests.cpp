#include "compat/MetrowerksStdCompat.hpp"
#include "JSystem/J3DGraphAnimator/J3DJoint.hpp"
#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/J3DGraphAnimator/J3DModel.hpp"
#include "JSystem/J3DGraphAnimator/J3DMtxBuffer.hpp"
#include "JSystem/JMath/JMath.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) throw std::runtime_error(std::string(message));
    }

    void matrix_near(const float* actual, const float* expected, std::size_t count, std::string_view message) {
        for (std::size_t index = 0; index < count; ++index) {
            if (!std::isfinite(actual[index]) || std::fabs(actual[index] - expected[index]) > 0.000002F) {
                throw std::runtime_error(std::string(message) + "; component=" + std::to_string(index));
            }
        }
    }

    void release_joint_only_storage(J3DMtxBuffer& buffer) {
        for (auto bank = 0; bank < 2; ++bank) {
            if (buffer.mpDrawMtxArr[bank] != nullptr && buffer.mpDrawMtxArr[bank] != &J3DMtxBuffer::sNoUseDrawMtxPtr) {
                for (u32 view = 0; view < buffer.mMtxNum; ++view) ::operator delete[](buffer.mpDrawMtxArr[bank][view], 0x20);
                delete[] buffer.mpDrawMtxArr[bank];
            }
            if (buffer.mpNrmMtxArr[bank] != nullptr && buffer.mpNrmMtxArr[bank] != &J3DMtxBuffer::sNoUseNrmMtxPtr) {
                for (u32 view = 0; view < buffer.mMtxNum; ++view) ::operator delete[](buffer.mpNrmMtxArr[bank][view], 0x20);
                delete[] buffer.mpNrmMtxArr[bank];
            }
        }
        delete[] buffer.mpScaleFlagArr;
        delete[] buffer.mpEvlpScaleFlagArr;
        delete[] buffer.mpAnmMtx;
        delete[] buffer.mpWeightEvlpMtx;
    }

    // These are actual intentionally joint-only J3DModelData objects. No BMD is
    // represented as decoded material/shape data. The original buffer destructor
    // is empty; this fixture explicitly owns each allocation made by create().
    struct Fixture {
        J3DModelData model;
        J3DMtxBuffer buffer;
        std::array<J3DJoint, 2> joints;
        std::array<J3DJoint*, 2> joint_pointers{&joints[0], &joints[1]};
        std::array<u8, 3> draw_flags{0, 1, 1};
        std::array<u16, 3> draw_indices{0, 0, 1};
        std::array<u8, 2> mix_counts{2, 1};
        std::array<u16, 3> mix_indices{0, 1, 0};
        std::array<float, 3> weights{0.25F, 0.75F, 1.0F};
        Mtx inverse[2]{};
        Mtx previous_view;

        explicit Fixture(bool envelopes = true, u32 flags = 0U, u32 views = 2U) {
            std::memcpy(previous_view, j3dSys.mViewMtx, sizeof(Mtx));
            auto& tree = model.getJointTree();
            model.mFlags = flags;
            tree.mJointNum = 2;
            tree.mJointNodePointer = joint_pointers.data();
            tree.mWEvlpMtxNum = envelopes ? 2 : 0;
            tree.mWEvlpMixMtxNum = mix_counts.data();
            tree.mWEvlpMixMtxIndex = mix_indices.data();
            tree.mWEvlpMixWeight = weights.data();
            tree.mInvJointMtx = inverse;
            tree.mDrawMtxData.mEntryNum = envelopes ? 3 : 2;
            tree.mDrawMtxData.mDrawFullWgtMtxNum = envelopes ? 1 : 2;
            tree.mDrawMtxData.mDrawMtxFlag = draw_flags.data();
            tree.mDrawMtxData.mDrawMtxIndex = draw_indices.data();
            if (!envelopes) {
                draw_flags = {0, 0, 0};
                draw_indices = {0, 1, 0};
            }
            require(buffer.create(&model, views) == kJ3DError_Success, "Original matrix-buffer create must succeed");
            for (auto index = 0; index < 2; ++index) {
                PSMTXIdentity(inverse[index]);
                PSMTXIdentity(buffer.getAnmMtx(index));
                buffer.setScaleFlag(index, 1);
            }
            PSMTXIdentity(j3dSys.mViewMtx);
        }

        ~Fixture() {
            std::memcpy(j3dSys.mViewMtx, previous_view, sizeof(Mtx));
            release_joint_only_storage(buffer);
        }
    };

    void allocation_and_view_banks() {
        Fixture fixture(false);
        auto& buffer = fixture.buffer;
        require(buffer.mJointTree == &fixture.model.getJointTree() && buffer.mpUserAnmMtx == buffer.mpAnmMtx,
                "Original allocation must retain the actual tree and user-animation alias");
        require(buffer.mMtxNum == 2U && buffer.mCurrentViewNo == 0U && buffer.mpWeightEvlpMtx == nullptr,
                "View count, initial view, and absent envelope storage must follow original create");
        for (auto bank = 0; bank < 2; ++bank) {
            for (auto view = 0; view < 2; ++view) {
                require(reinterpret_cast<std::uintptr_t>(buffer.mpDrawMtxArr[bank][view]) % 32U == 0U &&
                            reinterpret_cast<std::uintptr_t>(buffer.mpNrmMtxArr[bank][view]) % 32U == 0U,
                        "Every actual draw and normal allocation must retain original 32-byte alignment");
            }
        }
        auto* bank0_view0 = buffer.mpDrawMtxArr[0][0];
        auto* bank1_view0 = buffer.mpDrawMtxArr[1][0];
        auto* bank0_view1 = buffer.mpDrawMtxArr[0][1];
        auto* bank1_view1 = buffer.mpDrawMtxArr[1][1];
        buffer.swapDrawMtx();
        require(buffer.getDrawMtxPtr() == bank0_view0 && buffer.mpDrawMtxArr[0][0] == bank1_view0 &&
                    buffer.mpDrawMtxArr[0][1] == bank0_view1 && buffer.mpDrawMtxArr[1][1] == bank1_view1,
                "Swapping one view must leave the other view's two banks intact");
        buffer.mCurrentViewNo = 1;
        auto* prior_normal = buffer.mpNrmMtxArr[0][1];
        buffer.swapNrmMtx();
        require(buffer.getDrawMtxPtr() == bank1_view1 && buffer.getNrmMtxPtr() == prior_normal,
                "Current-view draw and normal access must select their actual independent banks");
    }

    void no_animation_and_concat_view_allocation() {
        for (const auto flags : std::array<u32, 2>{0x100U, 0x10U}) {
            Fixture fixture(false, flags, 1U);
            auto& buffer = fixture.buffer;
            require(buffer.mpAnmMtx != nullptr && buffer.mpUserAnmMtx == buffer.mpAnmMtx &&
                        buffer.mpDrawMtxArr[0] == &J3DMtxBuffer::sNoUseDrawMtxPtr &&
                        buffer.mpDrawMtxArr[1] == &J3DMtxBuffer::sNoUseDrawMtxPtr &&
                        buffer.mpNrmMtxArr[0] == &J3DMtxBuffer::sNoUseNrmMtxPtr &&
                        buffer.mpNrmMtxArr[1] == &J3DMtxBuffer::sNoUseNrmMtxPtr &&
                        buffer.mpBumpMtxArr[0] == nullptr && buffer.mpBumpMtxArr[1] == nullptr,
                    "No-animation and concat-view flags must select original shared unused draw/normal storage");
        }
    }

    void envelopes_draw_modes_and_normals() {
        Fixture fixture;
        auto& buffer = fixture.buffer;
        Mtx joint0{{1, 0, 0, 8}, {0, 1, 0, -4}, {0, 0, 1, 2}};
        Mtx joint1{{2, 0, 0, -4}, {0, 3, 0, 8}, {0, 0, 4, -2}};
        buffer.setAnmMtx(0, joint0);
        buffer.setAnmMtx(1, joint1);
        buffer.setScaleFlag(1, 0);
        fixture.inverse[0][0][3] = -2;
        fixture.inverse[1][0][3] = 1;
        fixture.inverse[1][1][3] = -1;
        fixture.inverse[1][2][3] = 2;
        buffer.calcWeightEnvelopeMtx();
        const Mtx envelope0{{1.75F, 0, 0, 0}, {0, 2.5F, 0, 2.75F}, {0, 0, 3.25F, 5}};
        const Mtx envelope1{{1, 0, 0, 6}, {0, 1, 0, -4}, {0, 0, 1, 2}};
        matrix_near(&buffer.getWeightAnmMtx(0)[0][0], &envelope0[0][0], 12, "Weighted inverse-bind matrices must retain authored weights");
        matrix_near(&buffer.getWeightAnmMtx(1)[0][0], &envelope1[0][0], 12, "Each envelope must start with zero accumulation");
        require(buffer.getEnvScaleFlag(0) == 0 && buffer.getEnvScaleFlag(1) == 1,
                "Envelope unit-scale flags must AND the actual contributing joint flags");
        const Mtx view{{0, -1, 0, 10}, {1, 0, 0, 20}, {0, 0, 1, 30}};
        std::memcpy(j3dSys.mViewMtx, view, sizeof(Mtx));
        buffer.calcDrawMtx(0, j3dDefaultScale, j3dDefaultMtx);
        const Mtx rigid_view{{0, -1, 0, 14}, {1, 0, 0, 28}, {0, 0, 1, 32}};
        const Mtx envelope_view{{0, -2.5F, 0, 7.25F}, {1.75F, 0, 0, 20}, {0, 0, 3.25F, 35}};
        matrix_near(&(*buffer.getDrawMtx(0))[0][0], &rigid_view[0][0], 12, "Mode0 must concatenate the actual global view with rigid joints");
        matrix_near(&(*buffer.getDrawMtx(1))[0][0], &envelope_view[0][0], 12, "Mode0 must concatenate the same view with weighted envelopes");
        buffer.calcNrmMtx();
        const Mtx33 rigid_normal{{0, -1, 0}, {1, 0, 0}, {0, 0, 1}};
        const Mtx33 weighted_normal{{0, -0.4F, 0}, {1.0F / 1.75F, 0, 0}, {0, 0, 1.0F / 3.25F}};
        matrix_near(&(*buffer.getNrmMtx(0))[0][0], &rigid_normal[0][0], 9, "Unit-scale rigid normals must copy the 3x3 basis");
        matrix_near(&(*buffer.getNrmMtx(1))[0][0], &weighted_normal[0][0], 9, "Scaled envelope normals must use inverse transpose");

        PSMTXIdentity(j3dSys.mViewMtx);
        const Mtx base{{1, 0, 0, 100}, {0, 1, 0, 200}, {0, 0, 1, 300}};
        const Vec scale{2, 3, 4};
        buffer.calcDrawMtx(2, scale, base);
        const Mtx based_rigid{{2, 0, 0, 116}, {0, 3, 0, 188}, {0, 0, 4, 308}};
        const Mtx based_envelope{{3.5F, 0, 0, 100}, {0, 7.5F, 0, 208.25F}, {0, 0, 13, 320}};
        matrix_near(&(*buffer.getDrawMtx(0))[0][0], &based_rigid[0][0], 12, "Mode2 must compose separate base scale, translation and joints");
        matrix_near(&(*buffer.getDrawMtx(1))[0][0], &based_envelope[0][0], 12, "Mode2 must use the same base for envelopes");

        const Mtx sentinel{{-123, -123, -123, -123}, {-123, -123, -123, -123}, {-123, -123, -123, -123}};
        std::memcpy(*buffer.getDrawMtx(2), sentinel, sizeof(Mtx));
        buffer.calcDrawMtx(1, scale, base);
        matrix_near(&(*buffer.getDrawMtx(0))[0][0], &joint0[0][0], 12, "Mode1 must copy rigid animation matrices without view/base");
        matrix_near(&(*buffer.getDrawMtx(1))[0][0], &envelope0[0][0], 12, "Mode1 must copy the first envelope");
        // Retail 80432550 reloads the full-weight count for this second loop.
        matrix_near(&(*buffer.getDrawMtx(2))[0][0], &sentinel[0][0], 12,
                    "Mode1 must preserve the retail full-weight loop bound rather than replacing it with envelope count");
    }

    void billboard_modes_preserve_translation_and_up() {
        Fixture fixture(false);
        auto& buffer = fixture.buffer;
        fixture.joints[0].setMtxType(1);
        fixture.joints[1].setMtxType(2);
        const Mtx spherical{{0, -3, 0, 7}, {2, 0, 0, 8}, {0, 0, -4, 9}};
        const Mtx cylindrical{{2, 0, 0, 11}, {0, 3, 0, 12}, {0, 4, 4, 13}};
        std::memcpy(*buffer.getDrawMtx(0), spherical, sizeof(Mtx));
        std::memcpy(*buffer.getDrawMtx(1), cylindrical, sizeof(Mtx));
        buffer.calcBBoardMtx();
        const Mtx billboard{{2, 0, 0, 7}, {0, 3, 0, 8}, {0, 0, 4, 9}};
        const Mtx33 normal{{0.5F, 0, 0}, {0, 1.0F / 3.0F, 0}, {0, 0, 0.25F}};
        matrix_near(&(*buffer.getDrawMtx(0))[0][0], &billboard[0][0], 12,
                    "Ordinary billboard must use full square roots and preserve translation");
        matrix_near(&(*buffer.getNrmMtx(0))[0][0], &normal[0][0], 9,
                    "Ordinary billboard normals must use reciprocal diagonal scale");
        const auto& y = *buffer.getDrawMtx(1);
        require(std::bit_cast<std::uint32_t>(y[0][0]) == 0x3FFFF400U && y[0][1] == 0 && y[1][1] == 3 && y[2][1] == 4 &&
                    y[0][3] == 11 && y[1][3] == 12 && y[2][3] == 13,
                "Y billboard must use retail approximate sqrt while retaining authored up and translation");
        const float retail_z_scale = std::bit_cast<float>(0x407FF400U);
        require(std::fabs(y[1][2] - -0.8F * retail_z_scale) < 0.000002F &&
                    std::fabs(y[2][2] - 0.6F * retail_z_scale) < 0.000002F,
                "Y billboard forward basis must remain perpendicular to its retained up direction");
    }

    void actual_model_constructor_calc_and_view_chain() {
        struct Globals {
            J3DSys system = j3dSys;
            Mtx current;
            Vec scale = J3DSys::mCurrentS;
            Vec parent_scale = J3DSys::mParentS;
            J3DJoint* joint = J3DMtxCalc::getJoint();
            J3DMtxBuffer* buffer = J3DMtxCalc::getMtxBuffer();
            J3DMtxCalc* calculator = J3DJoint::mCurrentMtxCalc;
            Globals() { std::memcpy(current, J3DSys::mCurrentMtx, sizeof(Mtx)); }
            ~Globals() {
                j3dSys = system;
                std::memcpy(J3DSys::mCurrentMtx, current, sizeof(Mtx));
                J3DSys::mCurrentS = scale;
                J3DSys::mParentS = parent_scale;
                J3DMtxCalc::setJoint(joint);
                J3DMtxCalc::setMtxBuffer(buffer);
                J3DJoint::mCurrentMtxCalc = calculator;
            }
        } globals;
        Fixture fixture(false);
        J3DMtxCalcNoAnm<J3DMtxCalcCalcTransformBasic, J3DMtxCalcJ3DSysInitBasic> calculator;
        auto& tree = fixture.model.getJointTree();
        tree.setBasicMtxCalc(&calculator);
        tree.mRootNode = &fixture.joints[0];
        fixture.joints[0].mChild = &fixture.joints[1];
        fixture.joints[0].mJntNo = 0;
        fixture.joints[1].mJntNo = 1;
        fixture.joints[0].mTransformInfo.mTranslate = {1, 2, 3};
        fixture.joints[1].mTransformInfo.mTranslate = {4, 0, 0};
        struct ModelOwner {
            J3DModel model;
            explicit ModelOwner(J3DModelData& data) : model(&data, 0, 1) {}
            ~ModelOwner() {
                release_joint_only_storage(*model.getMtxBuffer());
                delete model.mMtxBuffer;
            }
        } owner(fixture.model);
        require(owner.model.getModelData() == &fixture.model && owner.model.getMtxBuffer() != &fixture.buffer &&
                    owner.model.mMatPacket == nullptr && owner.model.mShapePacket == nullptr,
                "Real Model constructor must allocate its own buffer and retain the actual joint-only data");
        Mtx base{{1, 0, 0, 10}, {0, 1, 0, 20}, {0, 0, 1, 30}};
        owner.model.setBaseTRMtx(base);
        owner.model.setBaseScale(Vec{2, 3, 4});
        J3DModel* model = &owner.model;
        model->calc();
        const Mtx root{{2, 0, 0, 12}, {0, 3, 0, 26}, {0, 0, 4, 42}};
        const Mtx child{{2, 0, 0, 20}, {0, 3, 0, 26}, {0, 0, 4, 42}};
        matrix_near(&model->getAnmMtx(0)[0][0], &root[0][0], 12,
                    "Real Model::calc must execute original root-joint base-scale calculation");
        matrix_near(&model->getAnmMtx(1)[0][0], &child[0][0], 12,
                    "Real Model::calc must traverse the actual child with inherited transform");
        auto* previous_front = model->getMtxBuffer()->getDrawMtxPtr();
        auto* previous_back = model->getMtxBuffer()->mpDrawMtxArr[0][0];
        j3dSys.mViewMtx[1][3] = 5;
        model->viewCalc();
        const Mtx child_view{{2, 0, 0, 20}, {0, 3, 0, 31}, {0, 0, 4, 42}};
        const Mtx33 child_normal{{0.5F, 0, 0}, {0, 1.0F / 3.0F, 0}, {0, 0, 0.25F}};
        require(model->getMtxBuffer()->getDrawMtxPtr() == previous_back &&
                    model->getMtxBuffer()->mpDrawMtxArr[0][0] == previous_front,
                "Real Model::viewCalc must swap original draw-buffer banks");
        matrix_near(&(*model->getMtxBuffer()->getDrawMtx(1))[0][0], &child_view[0][0], 12,
                    "Real Model::viewCalc must calculate the selected view through actual buffer methods");
        matrix_near(&(*model->getMtxBuffer()->getNrmMtx(1))[0][0], &child_normal[0][0], 9,
                    "Real Model::viewCalc must calculate normals from original scale flags");
    }

    void shared_fast_sqrt_edge_contract() {
        require(std::bit_cast<std::uint32_t>(JMAFastSqrt(1.0F)) == 0x3F7FF400U &&
                    std::bit_cast<std::uint32_t>(JMath::fastSqrt(4.0F)) == 0x3FFFF400U,
                "Shared fast sqrt must preserve the measured retail frsqrte/multiply values");
        require(JMAFastSqrt(-4.0F) == -4.0F && std::bit_cast<std::uint32_t>(JMAFastSqrt(-0.0F)) == 0x80000000U,
                "Nonpositive fast-sqrt inputs must return unchanged, including signed zero");
        require(std::isnan(JMAFastSqrt(std::numeric_limits<float>::infinity())) &&
                    std::isnan(JMAFastSqrt(std::numeric_limits<float>::quiet_NaN())),
                "Retail estimate multiplication must retain its exceptional-value classifications");
    }
}

int main() {
    try {
        allocation_and_view_banks();
        no_animation_and_concat_view_allocation();
        envelopes_draw_modes_and_normals();
        billboard_modes_preserve_translation_and_up();
        actual_model_constructor_calc_and_view_chain();
        shared_fast_sqrt_edge_contract();
        std::cout << "6/6 original J3D matrix-buffer groups passed\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "Original J3D matrix-buffer regression failed: " << exception.what() << '\n';
        return 1;
    }
}
