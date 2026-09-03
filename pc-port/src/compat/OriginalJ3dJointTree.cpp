#include "compat/OriginalJ3dJointTree.hpp"

#include "JSystem/J3DGraphAnimator/J3DAnimation.hpp"
#include "JSystem/J3DGraphAnimator/J3DJoint.hpp"
#include "JSystem/J3DGraphAnimator/J3DJointTree.hpp"
#include "JSystem/J3DGraphAnimator/J3DMtxBuffer.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "render/J3dMaterialRuntime.hpp"
#include "render/J3dModel.hpp"

#include <algorithm>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace smgpc::compat {
    namespace {

        std::recursive_mutex sTraversalMutex;

        class TraversalScope final {
        public:
            TraversalScope()
                : _lock(sTraversalMutex), _buffer(J3DMtxCalc::mMtxBuffer), _joint(J3DMtxCalc::mJoint),
                  _calculator(J3DJoint::mCurrentMtxCalc), _current_scale(J3DSys::mCurrentS),
                  _parent_scale(J3DSys::mParentS) {
                std::memcpy(_matrix, J3DSys::mCurrentMtx, sizeof(_matrix));
            }

            ~TraversalScope() {
                J3DMtxCalc::mMtxBuffer = _buffer;
                J3DMtxCalc::mJoint = _joint;
                J3DJoint::mCurrentMtxCalc = _calculator;
                J3DSys::mCurrentS = _current_scale;
                J3DSys::mParentS = _parent_scale;
                std::memcpy(J3DSys::mCurrentMtx, _matrix, sizeof(_matrix));
            }

        private:
            std::unique_lock<std::recursive_mutex> _lock;
            J3DMtxBuffer* _buffer;
            J3DJoint* _joint;
            J3DMtxCalc* _calculator;
            Vec _current_scale;
            Vec _parent_scale;
            Mtx _matrix;
        };

        // Renderer frame/resource adapter. The transform sampling, scale-mode
        // calculation and traversal remain the original SDK implementations.
        class AnimationCalculator final : public J3DMtxCalc {
        public:
            J3DMtxCalc* basic = nullptr;
            void (*calculate_transform)(const J3DTransformInfo&) = nullptr;
            const J3DAnmTransformKey* animation = nullptr;
            float frame = 0.0F;

            void init(const Vec& scale, const Mtx& matrix) override {
                basic->init(scale, matrix);
            }

            void calc() override {
                J3DTransformInfo transform;
                animation->calcTransform(frame, getJoint()->getJntNo(), &transform);
                calculate_transform(transform);
            }
        };

        template<class Transform, class Init>
        std::unique_ptr<J3DMtxCalc> make_calculator(AnimationCalculator& animation) {
            animation.calculate_transform = &Transform::calcTransform;
            return std::make_unique<J3DMtxCalcNoAnm<Transform, Init>>();
        }

    }  // namespace

    struct OriginalJ3dJointTree::Storage {
        J3DJointTree tree;
        J3DMtxBuffer buffer;
        std::vector<J3DJoint> joints;
        std::vector<J3DJoint*> joint_pointers;
        std::unique_ptr<Mtx[]> animation_matrices;
        std::vector<u8> scale_flags;
        std::vector<render::J3dMatrix3x4> matrices;
        std::vector<J3DModelHierarchy> hierarchy;
        std::unique_ptr<J3DMtxCalc> basic;
        AnimationCalculator animation;
        bool calculating = false;

        Storage(const render::J3dInfoSummary& info, const render::J3dJointBlockSummary& source)
            : joints(source.joint_count), joint_pointers(source.joint_count),
              animation_matrices(std::make_unique<Mtx[]>(source.joint_count)),
              scale_flags(source.joint_count), matrices(source.joint_count) {
            if (source.joints.size() != source.joint_count) {
                throw std::runtime_error("J3D joint initialization table does not match its declared count");
            }
            // J3DModelLoader::readInformation selects exactly these three modes.
            switch (info.flags & 0x0fU) {
            case 0:
                basic = make_calculator<J3DMtxCalcCalcTransformBasic, J3DMtxCalcJ3DSysInitBasic>(animation);
                break;
            case 1:
                basic = make_calculator<J3DMtxCalcCalcTransformSoftimage, J3DMtxCalcJ3DSysInitSoftimage>(animation);
                break;
            case 2:
                basic = make_calculator<J3DMtxCalcCalcTransformMaya, J3DMtxCalcJ3DSysInitMaya>(animation);
                break;
            default:
                throw std::runtime_error("J3D model has an unsupported original joint matrix mode");
            }
            animation.basic = basic.get();
            tree.mFlags = info.flags;
            tree.mJointNum = source.joint_count;
            tree.mJointNodePointer = joint_pointers.data();
            tree.setBasicMtxCalc(basic.get());
            buffer.mJointTree = &tree;
            buffer.mpAnmMtx = animation_matrices.get();
            buffer.mpUserAnmMtx = buffer.mpAnmMtx;
            buffer.mpScaleFlagArr = scale_flags.data();

            for (std::size_t i = 0; i < joints.size(); ++i) {
                const auto& input = source.joints[i];
                auto& joint = joints[i];
                joint_pointers[i] = &joint;
                // Same decoded fields and sentinel conversion as the original
                // J3DJointFactory::create; pointers refer to native objects.
                joint.mJntNo = static_cast<u16>(i);
                joint.mKind = static_cast<u8>(input.kind);
                joint.mScaleCompensate = input.scale_compensate == 0xffU ? 0U : input.scale_compensate;
                joint.mTransformInfo = {{input.scale[0], input.scale[1], input.scale[2]},
                                       {input.rotation[0], input.rotation[1], input.rotation[2]},
                                       {input.translation[0], input.translation[1], input.translation[2]}};
                joint.mBoundingSphereRadius = input.radius;
                joint.mMin = {input.min[0], input.min[1], input.min[2]};
                joint.mMax = {input.max[0], input.max[1], input.max[2]};
            }
            for (const auto& command : info.hierarchy) {
                hierarchy.push_back({command.type, command.value});
            }
            tree.mHierarchy = hierarchy.data();
            link_joints();
        }

        void link_joints() {
            // The joint links follow makeHierarchy's parent/current-joint
            // scopes. Material and shape commands remain with the existing
            // renderer; they do not change those joint scopes in J3D.
            struct Scope { J3DJoint* parent; J3DJoint* current; };
            std::vector<Scope> scopes{{nullptr, nullptr}};
            std::vector<bool> linked(joints.size());
            bool ended = false;
            for (const auto& command : hierarchy) {
                auto& scope = scopes.back();
                switch (command.mType) {
                case 0:
                    ended = true;
                    break;
                case 1:
                    scopes.push_back({scope.current, scope.current});
                    break;
                case 2:
                    if (scopes.size() == 1U) {
                        throw std::runtime_error("J3D hierarchy closes a missing child scope");
                    }
                    scopes.pop_back();
                    break;
                case 0x10:
                    if (command.mValue >= joints.size() || linked[command.mValue]) {
                        throw std::runtime_error("J3D hierarchy contains an invalid or repeated joint");
                    }
                    linked[command.mValue] = true;
                    scope.current = &joints[command.mValue];
                    if (scope.parent != nullptr) {
                        scope.parent->appendChild(scope.current);
                    } else {
                        if (tree.mRootNode != nullptr) {
                            throw std::runtime_error("J3D hierarchy contains disconnected root joints");
                        }
                        tree.mRootNode = scope.current;
                    }
                    break;
                case 0x11:
                case 0x12:
                    break;
                default:
                    throw std::runtime_error("J3D hierarchy contains an unknown command");
                }
                if (ended) {
                    break;
                }
            }
            if (!ended || std::find(linked.begin(), linked.end(), false) != linked.end()) {
                throw std::runtime_error("J3D hierarchy does not define the complete joint tree");
            }
        }
    };

    OriginalJ3dJointTree::OriginalJ3dJointTree(const render::J3dInfoSummary& info,
                                            const render::J3dJointBlockSummary& joints)
        : _storage(std::make_unique<Storage>(info, joints)) {
    }

    OriginalJ3dJointTree::~OriginalJ3dJointTree() = default;

    std::span<const render::J3dMatrix3x4> OriginalJ3dJointTree::calculate(const J3DAnmTransformKey* animation,
                                                                      float frame,
                                                                      const render::J3dMatrix3x4& base_transform,
                                                                      const std::array<float, 3>& base_scale) {
        auto& storage = *_storage;
        if (animation != nullptr && animation->field_0x1e < storage.tree.mJointNum) {
            throw std::runtime_error("J3D transform animation does not contain every model joint");
        }
        TraversalScope scope;
        if (storage.calculating) {
            throw std::logic_error("J3D joint calculation cannot reenter its own matrix buffer");
        }
        storage.calculating = true;
        storage.animation.animation = animation;
        storage.animation.frame = frame;
        struct AnimationScope {
            Storage& storage;
            ~AnimationScope() {
                storage.tree.setBasicMtxCalc(storage.basic.get());
                storage.animation.animation = nullptr;
                storage.calculating = false;
            }
        } animation_scope{storage};
        storage.tree.setBasicMtxCalc(animation != nullptr ? &storage.animation : storage.basic.get());
        const Vec scale{base_scale[0], base_scale[1], base_scale[2]};
        Mtx base;
        std::memcpy(base, base_transform.m.data(), sizeof(base));
        storage.tree.calc(&storage.buffer, scale, base);
        for (std::size_t joint = 0; joint < storage.matrices.size(); ++joint) {
            std::memcpy(storage.matrices[joint].m.data(), storage.animation_matrices[joint], sizeof(Mtx));
        }
        return storage.matrices;
    }

    J3DJointTree& OriginalJ3dJointTree::joint_tree() {
        return _storage->tree;
    }

    J3DMtxBuffer& OriginalJ3dJointTree::matrix_buffer() {
        return _storage->buffer;
    }

}  // namespace smgpc::compat
