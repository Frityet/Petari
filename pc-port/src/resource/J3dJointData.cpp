#include "J3dJointData.hpp"
#include "J3dNameData.hpp"

#include "JSystem/J3DGraphAnimator/J3DJoint.hpp"
#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/JUtility/JUTNameTab.hpp"

#include <algorithm>
#include <bit>
#include <stdexcept>
#include <vector>

namespace smgpc::resource {
    namespace {

        using Bytes = std::span<const std::uint8_t>;

        void require_range(Bytes data, std::size_t offset, std::size_t size) {
            if (offset > data.size() || size > data.size() - offset) {
                throw std::runtime_error("J3D joint resource range outside its containing block");
            }
        }

        std::uint16_t u16_at(Bytes data, std::size_t offset) {
            require_range(data, offset, 2);
            return static_cast<std::uint16_t>((std::uint16_t{data[offset]} << 8U) | data[offset + 1]);
        }

        std::uint32_t u32_at(Bytes data, std::size_t offset) {
            require_range(data, offset, 4);
            return (std::uint32_t{u16_at(data, offset)} << 16U) | u16_at(data, offset + 2);
        }

        float f32_at(Bytes data, std::size_t offset) {
            return std::bit_cast<float>(u32_at(data, offset));
        }

        Vec vec_at(Bytes data, std::size_t offset) {
            return {f32_at(data, offset), f32_at(data, offset + 4), f32_at(data, offset + 8)};
        }

        std::size_t table_offset(Bytes block, std::size_t field, std::size_t bytes) {
            const auto offset = u32_at(block, field);
            if (bytes != 0 && offset == 0) {
                throw std::runtime_error("J3D joint resource is missing a required table");
            }
            require_range(block, offset, bytes);
            return offset;
        }

        struct Blocks {
            Bytes info;
            Bytes joint;
            Bytes envelope;
            Bytes draw;
            std::uint32_t model_type = 0;
            std::uint16_t envelope_count_at_draw = 0;
        };

        Blocks find_blocks(Bytes bytes) {
            require_range(bytes, 0, 0x20);
            const auto type = u32_at(bytes, 4);
            if (u32_at(bytes, 0) != 0x4a334432U ||
                (type != 0x626d6432U && type != 0x626d6433U && type != 0x62646c34U)) {
                throw std::runtime_error("J3D joint resource requires a J3D2 BMD2/BMD3/BDL4 model");
            }
            const auto size = u32_at(bytes, 8);
            require_range(bytes, 0, size);
            if (size < 0x20) {
                throw std::runtime_error("J3D joint resource file header is truncated");
            }
            bytes = bytes.first(size);
            Blocks blocks;
            blocks.model_type = type == 0x62646c34U ? 1 : 0;
            std::size_t cursor = 0x20;
            const auto count = u32_at(bytes, 0xc);
            for (std::uint32_t i = 0; i < count; ++i) {
                require_range(bytes, cursor, 8);
                const auto block_size = u32_at(bytes, cursor + 4);
                if (block_size < 8) {
                    throw std::runtime_error("J3D joint resource block header is truncated");
                }
                require_range(bytes, cursor, block_size);
                Bytes* destination = nullptr;
                switch (u32_at(bytes, cursor)) {
                case 0x494e4631U: destination = &blocks.info; break;
                case 0x4a4e5431U: destination = &blocks.joint; break;
                case 0x45565031U: destination = &blocks.envelope; break;
                case 0x44525731U:
                    destination = &blocks.draw;
                    blocks.envelope_count_at_draw = blocks.envelope.empty() ? 0 : u16_at(blocks.envelope, 8);
                    break;
                default: break;
                }
                if (destination != nullptr) {
                    if (!destination->empty()) {
                        throw std::runtime_error("J3D joint resource has duplicate construction blocks");
                    }
                    *destination = bytes.subspan(cursor, block_size);
                }
                cursor += block_size;
            }
            if (blocks.info.empty() || blocks.joint.empty() || blocks.envelope.empty() || blocks.draw.empty()) {
                throw std::runtime_error("J3D joint resource is missing INF1/JNT1/EVP1/DRW1");
            }
            return blocks;
        }

    }  // namespace

    struct J3dJointData::Storage {
        std::uint32_t flags = 0;
        std::uint32_t model_type = 0;
        std::uint32_t packet_count = 0;
        std::uint32_t vertex_count = 0;
        std::uint16_t draw_count = 0;
        std::uint16_t full_weight_count = 0;
        std::vector<J3DModelHierarchy> hierarchy;
        std::vector<J3DJoint> joints;
        std::vector<J3DJoint*> joint_pointers;
        std::unique_ptr<J3DMtxCalc> basic;
        J3dNameData names;
        std::vector<u8> mix_counts;
        std::vector<u16> mix_indices;
        std::vector<float> mix_weights;
        std::unique_ptr<Mtx[]> inverse_matrices;
        std::vector<u8> draw_flags;
        std::vector<u16> draw_indices;
        std::unique_ptr<u16[]> important_indices;
        bool attached = false;

        Storage(Bytes bytes, std::uint32_t load_flags) {
            const auto blocks = find_blocks(bytes);
            model_type = blocks.model_type;
            load_joints(blocks.joint);
            load_info(blocks.info, load_flags);
            load_envelopes(blocks.envelope);
            load_draw(blocks.draw, blocks.envelope_count_at_draw);
        }

        void load_joints(Bytes block) {
            require_range(block, 0, 0x18);
            const auto count = u16_at(block, 8);
            const auto init = table_offset(block, 0xc, count == 0 ? 0 : 0x40);
            const auto indices = table_offset(block, 0x10, count * 2U);
            joints.resize(count);
            joint_pointers.reserve(count);
            for (std::size_t i = 0; i < count; ++i) {
                const auto source = init + u16_at(block, indices + i * 2) * 0x40U;
                require_range(block, source, 0x40);
                auto& joint = joints[i];
                // J3DJointFactory::create: logical numbering, remapped authored
                // metadata, low-byte kind, and the original 0xff sentinel.
                joint.mJntNo = static_cast<u16>(i);
                joint.mKind = static_cast<u8>(u16_at(block, source));
                joint.mScaleCompensate = block[source + 2] == 0xff ? 0 : block[source + 2];
                joint.mTransformInfo.mScale = vec_at(block, source + 4);
                joint.mTransformInfo.mRotation.x = std::bit_cast<s16>(u16_at(block, source + 0x10));
                joint.mTransformInfo.mRotation.y = std::bit_cast<s16>(u16_at(block, source + 0x12));
                joint.mTransformInfo.mRotation.z = std::bit_cast<s16>(u16_at(block, source + 0x14));
                joint.mTransformInfo.mTranslate = vec_at(block, source + 0x18);
                joint.mBoundingSphereRadius = f32_at(block, source + 0x24);
                joint.mMin = vec_at(block, source + 0x28);
                joint.mMax = vec_at(block, source + 0x34);
                joint.mMtxCalc = nullptr;
                joint_pointers.push_back(&joint);
            }
            const auto name_offset = u32_at(block, 0x14);
            if (name_offset != 0) {
                require_range(block, name_offset, 4);
                names = J3dNameData(block.subspan(name_offset));
            }
        }

        void load_info(Bytes block, std::uint32_t load_flags) {
            require_range(block, 0, 0x18);
            flags = load_flags | u16_at(block, 8);
            packet_count = u32_at(block, 0xc);
            vertex_count = u32_at(block, 0x10);
            switch (flags & 0xf) {
            case 0:
                basic = std::make_unique<J3DMtxCalcNoAnm<J3DMtxCalcCalcTransformBasic, J3DMtxCalcJ3DSysInitBasic>>();
                break;
            case 1:
                basic = std::make_unique<J3DMtxCalcNoAnm<J3DMtxCalcCalcTransformSoftimage, J3DMtxCalcJ3DSysInitSoftimage>>();
                break;
            case 2:
                basic = std::make_unique<J3DMtxCalcNoAnm<J3DMtxCalcCalcTransformMaya, J3DMtxCalcJ3DSysInitMaya>>();
                break;
            default:
                throw std::runtime_error("J3D joint resource selects no original matrix calculator");
            }
            std::size_t cursor = table_offset(block, 0x14, 4);
            std::size_t depth = 0;
            for (;;) {
                const auto type = u16_at(block, cursor);
                const auto value = u16_at(block, cursor + 2);
                hierarchy.push_back({type, value});
                cursor += 4;
                switch (type) {
                case 0:
                    if (depth != 0) {
                        throw std::runtime_error("J3D joint hierarchy has unclosed child scopes");
                    }
                    return;
                case 1: ++depth; break;
                case 2:
                    if (depth == 0) {
                        throw std::runtime_error("J3D joint hierarchy closes an absent child scope");
                    }
                    --depth;
                    break;
                case 0x10:
                    if (value >= joints.size()) {
                        throw std::runtime_error("J3D hierarchy joint index is outside JNT1");
                    }
                    break;
                case 0x11:
                case 0x12:
                    // The complete owner validates these against its retained
                    // material/shape tables before original makeHierarchy.
                    break;
                default:
                    throw std::runtime_error("J3D joint hierarchy contains an unknown command");
                }
            }
        }

        void load_envelopes(Bytes block) {
            require_range(block, 0, 0x1c);
            const auto count = u16_at(block, 8);
            const auto counts = table_offset(block, 0xc, count);
            mix_counts.assign(block.begin() + counts, block.begin() + counts + count);
            std::size_t total = 0;
            for (const auto mix_count : mix_counts) {
                // The original envelope calculation enters a do/while loop.
                if (mix_count == 0) {
                    throw std::runtime_error("J3D envelope has no matrix influences");
                }
                total += mix_count;
            }
            const auto indices = table_offset(block, 0x10, total * 2);
            const auto weights = table_offset(block, 0x14, total * 4);
            mix_indices.reserve(total);
            mix_weights.reserve(total);
            std::size_t required_inverse_count = 0;
            for (std::size_t i = 0; i < total; ++i) {
                const auto index = u16_at(block, indices + i * 2);
                if (index >= joints.size()) {
                    throw std::runtime_error("J3D envelope influence is outside JNT1");
                }
                required_inverse_count = std::max(required_inverse_count, std::size_t{index} + 1);
                mix_indices.push_back(index);
                mix_weights.push_back(f32_at(block, weights + i * 4));
            }
            const auto inverse = table_offset(block, 0x18, required_inverse_count * sizeof(Mtx));
            if (inverse != 0) {
                // EVP1 has no inverse-table count. Retain every complete matrix
                // readable from this pointer within its block, including ones
                // not referenced by the current envelope set.
                const auto available = (block.size() - inverse) / sizeof(Mtx);
                inverse_matrices = std::make_unique<Mtx[]>(available);
                for (std::size_t i = 0; i < available; ++i) {
                    for (std::size_t row = 0; row < 3; ++row) {
                        for (std::size_t column = 0; column < 4; ++column) {
                            inverse_matrices[i][row][column] = f32_at(block, inverse + i * 48 + row * 16 + column * 4);
                        }
                    }
                }
            }
        }

        void load_draw(Bytes block, std::uint16_t envelope_count_at_draw) {
            require_range(block, 0, 0x14);
            const auto serialized_count = u16_at(block, 8);
            if (serialized_count < envelope_count_at_draw) {
                throw std::runtime_error("J3D draw matrix count underflows the original envelope subtraction");
            }
            // Retail readDraw (0x8043ea68) subtracts the envelope count here;
            // the serialized arrays themselves retain their complete extent.
            draw_count = static_cast<u16>(serialized_count - envelope_count_at_draw);
            const auto flags_offset = table_offset(block, 0xc, serialized_count);
            const auto indices = table_offset(block, 0x10, serialized_count * 2U);
            draw_flags.assign(block.begin() + flags_offset, block.begin() + flags_offset + serialized_count);
            draw_indices.reserve(serialized_count);
            for (std::size_t i = 0; i < serialized_count; ++i) {
                draw_indices.push_back(u16_at(block, indices + i * 2));
            }
            for (; full_weight_count < draw_count; ++full_weight_count) {
                if (draw_flags[full_weight_count] == 1) {
                    break;
                }
            }
            if (full_weight_count + mix_counts.size() > draw_count) {
                throw std::runtime_error("J3D important-matrix output exceeds the original draw allocation");
            }
            for (std::size_t i = 0; i < draw_count; ++i) {
                const auto extent = draw_flags[i] == 0 ? joints.size() : mix_counts.size();
                if (draw_indices[i] >= extent) {
                    throw std::runtime_error("J3D draw matrix index is outside its joint/envelope table");
                }
            }
            // findImportantMtxIndex fills this after all tables have been
            // attached. Its pre-finalization contents are intentionally unused.
            important_indices = std::unique_ptr<u16[]>(new u16[draw_count]);
        }
    };

    J3dJointData::J3dJointData(Bytes bytes, std::uint32_t load_flags)
        : _storage(std::make_unique<Storage>(bytes, load_flags)) {}
    J3dJointData::~J3dJointData() = default;
    J3dJointData::J3dJointData(J3dJointData&&) noexcept = default;

    void J3dJointData::attach_to(J3DModelData& model) {
        auto& tree = model.mJointTree;
        if (!_storage || _storage->attached || tree.mHierarchy != nullptr || tree.mRootNode != nullptr ||
            tree.mBasicMtxCalc != nullptr || tree.mJointNodePointer != nullptr || tree.mJointNum != 0 ||
            tree.mWEvlpMtxNum != 0 || tree.mWEvlpMixMtxNum != nullptr || tree.mWEvlpMixMtxIndex != nullptr ||
            tree.mWEvlpMixWeight != nullptr || tree.mInvJointMtx != nullptr || tree.mWEvlpImportantMtxIdx != nullptr ||
            tree.mDrawMtxData.mEntryNum != 0 || tree.mDrawMtxData.mDrawMtxFlag != nullptr ||
            tree.mDrawMtxData.mDrawMtxIndex != nullptr || tree.mJointName != nullptr) {
            throw std::logic_error("J3D joint resource requires a fresh attachment destination");
        }
        auto& data = *_storage;
        model.mFlags = data.flags;
        model.mVertexData.mPacketNum = data.packet_count;
        model.mVertexData.mVtxNum = data.vertex_count;
        tree.mFlags = data.flags;
        tree.mModelDataType = data.model_type;
        tree.mHierarchy = data.hierarchy.data();
        tree.mBasicMtxCalc = data.basic.get();
        tree.mJointNodePointer = data.joint_pointers.data();
        tree.mJointNum = static_cast<u16>(data.joints.size());
        tree.mJointName = data.names.table();
        tree.mWEvlpMtxNum = static_cast<u16>(data.mix_counts.size());
        tree.mWEvlpMixMtxNum = data.mix_counts.data();
        tree.mWEvlpMixMtxIndex = data.mix_indices.data();
        tree.mWEvlpMixWeight = data.mix_weights.data();
        tree.mInvJointMtx = data.inverse_matrices.get();
        tree.mWEvlpImportantMtxIdx = data.important_indices.get();
        tree.mDrawMtxData.mEntryNum = data.draw_count;
        tree.mDrawMtxData.mDrawFullWgtMtxNum = data.full_weight_count;
        tree.mDrawMtxData.mDrawMtxFlag = data.draw_flags.data();
        tree.mDrawMtxData.mDrawMtxIndex = data.draw_indices.data();
        data.attached = true;
    }

}  // namespace smgpc::resource
