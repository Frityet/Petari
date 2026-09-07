#pragma once

#include "render/J3dMaterialRuntime.hpp"

#include <memory>
#include <span>

class J3DAnmTransformKey;
class J3DJointTree;
class J3DMtxBuffer;

namespace smgpc::render {
    struct J3dInfoSummary;
    struct J3dJointBlockSummary;
    struct J3dMatrix3x4;
}

namespace smgpc::compat {

    // Native storage for actual J3D joints, their original traversal and its
    // animation matrix buffer. This is a joint tree, not a partial J3DModel.
    class OriginalJ3dJointTree final {
    public:
        OriginalJ3dJointTree(const render::J3dInfoSummary&, const render::J3dJointBlockSummary&);
        ~OriginalJ3dJointTree();
        OriginalJ3dJointTree(const OriginalJ3dJointTree&) = delete;
        OriginalJ3dJointTree& operator=(const OriginalJ3dJointTree&) = delete;

        // The caller owns playback/frame control. This consumes the raw sample
        // frame; the original sampler and traversal do not add a loop policy.
        [[nodiscard]] std::span<const render::J3dMatrix3x4>
        calculate(const J3DAnmTransformKey* animation, float frame,
                  const render::J3dMatrix3x4& base_transform = {},
                  const std::array<float, 3>& base_scale = {1.0F, 1.0F, 1.0F});
        [[nodiscard]] J3DJointTree& joint_tree();
        [[nodiscard]] J3DMtxBuffer& matrix_buffer();

    private:
        struct Storage;
        std::unique_ptr<Storage> _storage;
    };

}  // namespace smgpc::compat
