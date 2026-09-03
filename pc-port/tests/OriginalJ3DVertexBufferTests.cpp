#include "compat/MetrowerksStdCompat.hpp"

#include "Game/Animation/XanimeCore.hpp"
#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/J3DGraphBase/J3DVertex.hpp"

#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    struct VertexArrays {
        std::array<Vec, 2> positions{{{1, 2, 3}, {4, 5, 6}}};
        std::array<Vec, 2> normals{{{0, 1, 0}, {0, 0, 1}}};
        std::array<GXColor, 2> colors{{{1, 2, 3, 4}, {5, 6, 7, 8}}};
        std::array<GXColor, 2> secondary_colors{{{9, 10, 11, 12}, {13, 14, 15, 16}}};
        J3DVertexData data;

        VertexArrays() {
            data.mVtxNum = positions.size();
            data.mNrmNum = normals.size();
            data.mColNum = colors.size();
            data.mVtxPosArray = positions.data();
            data.mVtxNrmArray = normals.data();
            data.mVtxColorArray[0] = colors.data();
            data.mVtxColorArray[1] = secondary_colors.data();
        }
    };

    void require_cleared(const J3DVertexBuffer& buffer) {
        require(buffer.mVtxData == nullptr, "init clears attached data");
        for (int index = 0; index < 2; ++index) {
            require(buffer.mVtxPosArray[index] == nullptr, "init clears both local position slots");
            require(buffer.mVtxNrmArray[index] == nullptr, "init clears both local normal slots");
            require(buffer.mVtxColArray[index] == nullptr, "init clears both local color slots");
            require(buffer.mTransformedVtxPosArray[index] == nullptr, "init clears both transformed position slots");
            require(buffer.mTransformedVtxNrmArray[index] == nullptr, "init clears both transformed normal slots");
        }
        require(buffer.mCurrentVtxPos == nullptr && buffer.mCurrentVtxNrm == nullptr && buffer.mCurrentVtxCol == nullptr,
                "init clears all current pointers");
    }

    void require_attached(const J3DVertexBuffer& buffer, VertexArrays& arrays) {
        require(buffer.getVertexData() == &arrays.data, "attachment retains actual vertex data identity");
        require(buffer.mVtxPosArray[0] == arrays.positions.data() && buffer.mVtxNrmArray[0] == arrays.normals.data(),
                "attachment seeds local slot zero with borrowed original arrays");
        require(buffer.mVtxColArray[0] == arrays.colors.data(), "attachment uses color channel zero");
        require(buffer.mVtxPosArray[1] == nullptr && buffer.mVtxNrmArray[1] == nullptr && buffer.mVtxColArray[1] == nullptr,
                "attachment clears alternate local slots even when the resource has a second color channel");
        require(buffer.mTransformedVtxPosArray[0] == arrays.positions.data() &&
                    buffer.mTransformedVtxNrmArray[0] == arrays.normals.data(),
                "attachment seeds transformed slot zero with the original position and normal arrays");
        require(buffer.mTransformedVtxPosArray[1] == nullptr && buffer.mTransformedVtxNrmArray[1] == nullptr,
                "attachment clears alternate transformed slots");
        require(buffer.mCurrentVtxPos == arrays.positions.data() && buffer.mCurrentVtxNrm == arrays.normals.data() &&
                    buffer.mCurrentVtxCol == arrays.colors.data(),
                "attachment invokes the original frameInit selection");
    }

    void test_initialization_and_attachment() {
        J3DVertexBuffer buffer;
        require_cleared(buffer);

        VertexArrays arrays;
        buffer.setVertexData(&arrays.data);
        require_attached(buffer, arrays);
        arrays.positions[0].x = 17;
        require(static_cast<Vec*>(buffer.getCurrentVtxPos())[0].x == 17, "attachment aliases resource storage without copying");

        J3DVertexData empty_data;
        buffer.setVertexData(&empty_data);
        require(buffer.getVertexData() == &empty_data, "an actual empty resource remains attached");
        require(buffer.getCurrentVtxPos() == nullptr && buffer.getCurrentVtxNrm() == nullptr && buffer.mCurrentVtxCol == nullptr,
                "valid empty resource clears current arrays");
        buffer.init();
        require_cleared(buffer);
        require(arrays.positions[0].x == 17, "reset does not delete or alter resource storage");
    }

    void test_frame_selection_and_independent_swaps() {
        VertexArrays resource;
        VertexArrays alternate;
        VertexArrays transformed;
        J3DVertexBuffer buffer;
        buffer.setVertexData(&resource.data);
        buffer.mVtxPosArray[1] = alternate.positions.data();
        buffer.mVtxNrmArray[1] = alternate.normals.data();
        buffer.mVtxColArray[1] = alternate.colors.data();
        buffer.mTransformedVtxPosArray[1] = transformed.positions.data();
        buffer.mTransformedVtxNrmArray[1] = transformed.normals.data();

        buffer.swapTransformedVtxPos();
        buffer.swapTransformedVtxNrm();
        require(buffer.getTransformedVtxPos(0) == transformed.positions.data() &&
                    buffer.getTransformedVtxNrm(0) == transformed.normals.data(),
                "transformed swaps select the separate transformed arrays");
        require(buffer.getTransformedVtxPos(1) == resource.positions.data() &&
                    buffer.getTransformedVtxNrm(1) == resource.normals.data(),
                "transformed swaps retain the original arrays in slot one");
        buffer.frameInit();
        require(buffer.getCurrentVtxPos() == resource.positions.data() && buffer.getCurrentVtxNrm() == resource.normals.data(),
                "frameInit selects local arrays regardless of transformed slots");

        buffer.swapVtxPosArrayPointer();
        buffer.swapVtxNrmArrayPointer();
        require(buffer.getCurrentVtxPos() == resource.positions.data() && buffer.getCurrentVtxNrm() == resource.normals.data(),
                "local swaps do not implicitly change current arrays before frameInit");
        buffer.frameInit();
        require(buffer.getCurrentVtxPos() == alternate.positions.data() && buffer.getCurrentVtxNrm() == alternate.normals.data(),
                "next frame selects swapped local arrays");
        require(buffer.getVtxPosArrayPointer(1) == resource.positions.data() &&
                    buffer.getVtxNrmArrayPointer(1) == resource.normals.data(),
                "local swaps preserve original arrays in slot one");
        require(buffer.mCurrentVtxCol == resource.colors.data(), "position and normal swaps leave color selection unchanged");

        buffer.setCurrentVtxPos(transformed.positions.data());
        buffer.setCurrentVtxNrm(transformed.normals.data());
        buffer.setCurrentVtxCol(transformed.colors.data());
        require(buffer.getCurrentVtxPos() == transformed.positions.data() && buffer.getCurrentVtxNrm() == transformed.normals.data() &&
                    buffer.mCurrentVtxCol == transformed.colors.data(),
                "explicit current-array setters support a subsequent deformation pass");
        buffer.frameInit();
        require(buffer.getCurrentVtxPos() == alternate.positions.data() && buffer.getCurrentVtxNrm() == alternate.normals.data() &&
                    buffer.mCurrentVtxCol == resource.colors.data(),
                "next frame resets deformation selections to local slot zero");

        buffer.setVertexData(&alternate.data);
        require_attached(buffer, alternate);
        require(resource.data.mVtxPosArray == resource.positions.data() &&
                    resource.data.mVtxColorArray[1] == resource.secondary_colors.data(),
                "reattachment does not rewrite the old resource's arrays");
    }

    void test_external_storage_survives_destruction() {
        auto arrays = std::make_unique<VertexArrays>();
        VertexArrays alternate;
        {
            auto buffer = std::make_unique<J3DVertexBuffer>();
            buffer->setVertexData(&arrays->data);
            buffer->mVtxPosArray[1] = alternate.positions.data();
            buffer->mVtxNrmArray[1] = alternate.normals.data();
            buffer->mVtxColArray[1] = alternate.colors.data();
            buffer->mTransformedVtxPosArray[1] = alternate.positions.data();
            buffer->mTransformedVtxNrmArray[1] = alternate.normals.data();
        }
        arrays->positions[1].x = 29;
        alternate.colors[1].a = 37;
        require(arrays->data.getVtxPosArray() == arrays->positions.data() && arrays->positions[1].x == 29,
                "deleting the buffer leaves its borrowed resource and arrays valid");
        require(alternate.colors[1].a == 37, "destruction does not delete alternate or transformed borrowed arrays");
    }

    void test_original_joint_transform_accessor() {
        J3DModelData model;
        std::array<J3DJoint, 2> joints;
        std::array<J3DJoint*, 2> joint_pointers{&joints[0], &joints[1]};
        model.getJointTree().mJointNum = joint_pointers.size();
        model.getJointTree().mJointNodePointer = joint_pointers.data();

        XanimeCore core(1, joint_pointers.size(), 0);
        // Original Core destruction is empty: this fixture owns the allocations.
        std::unique_ptr<XjointInfo[]> joint_storage(core.mJointList);
        std::unique_ptr<XanimeTrack[]> track_storage(core.mTrackList);
        require(core.getJointTransform(0) == nullptr && core.getJointTransform(1) == nullptr,
                "original accessor returns null before optional transform allocation");
        core.enableJointTransform(&model);
        std::unique_ptr<XjointTransform[]> transform_storage(core.mTransformList);
        require(core.getJointTransform(0) == &transform_storage[0] && core.getJointTransform(1) == &transform_storage[1],
                "original accessor uses the native typed element stride");
        core.getJointTransform(1)->_2C.x = 43;
        require(transform_storage[1]._2C.x == 43 && transform_storage[0]._2C.x == 0,
                "mutation through the accessor reaches only the selected original joint transform");

        XanimeCore upper(1, &core);
        std::unique_ptr<XanimeTrack[]> upper_track_storage(upper.mTrackList);
        require(upper.getJointTransform(1) == core.getJointTransform(1),
                "shared original cores expose the same transform object");
    }
}  // namespace

int main() {
    try {
        test_initialization_and_attachment();
        test_frame_selection_and_independent_swaps();
        test_external_storage_survives_destruction();
        test_original_joint_transform_accessor();
        std::cout << "4/4 original J3D vertex buffer and transform accessor groups passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[fail] " << error.what() << '\n';
        return 1;
    }
}
