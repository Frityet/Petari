#include <aurora/exception.hpp>
#include "J3dJointData.hpp"
#include "J3dNameData.hpp"
#include "J3dNativeBlock.hpp"
#include "JSystem/J3DGraphLoader/J3DJointFactory.hpp"
#include "JSystem/J3DGraphLoader/J3DModelLoader.hpp"

#include "JSystem/J3DGraphAnimator/J3DJoint.hpp"
#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/JUtility/JUTNameTab.hpp"
#include "JSystem/JSupport/JSupport.hpp"

#include <algorithm>
#include <bit>
#include <array>
#include <stdexcept>
#include <vector>

namespace smgpc::resource {
    namespace {

        using Bytes = std::span<const std::uint8_t>;

        void require_range(Bytes data, std::size_t offset, std::size_t size) {
            if (offset > data.size() || size > data.size() - offset) {
                aurora::throw_host_exception<std::runtime_error>("J3D joint resource range outside its containing block");
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
                aurora::throw_host_exception<std::runtime_error>("J3D joint resource is missing a required table");
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
                (type != 0x626d6432U && type != 0x626d6433U && type != 0x62646c33U && type != 0x62646c34U)) {
                aurora::throw_host_exception<std::runtime_error>("J3D joint resource requires a J3D2 BMD2/BMD3/BDL3/BDL4 model");
            }
            const auto size = u32_at(bytes, 8);
            require_range(bytes, 0, size);
            if (size < 0x20) {
                aurora::throw_host_exception<std::runtime_error>("J3D joint resource file header is truncated");
            }
            bytes = bytes.first(size);
            Blocks blocks;
            blocks.model_type = type == 0x62646c33U || type == 0x62646c34U ? 1 : 0;
            std::size_t cursor = 0x20;
            const auto count = u32_at(bytes, 0xc);
            for (std::uint32_t i = 0; i < count; ++i) {
                require_range(bytes, cursor, 8);
                const auto block_size = u32_at(bytes, cursor + 4);
                if (block_size < 8) {
                    aurora::throw_host_exception<std::runtime_error>("J3D joint resource block header is truncated");
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
                        aurora::throw_host_exception<std::runtime_error>("J3D joint resource has duplicate construction blocks");
                    }
                    *destination = bytes.subspan(cursor, block_size);
                }
                cursor += block_size;
            }
            if (blocks.info.empty() || blocks.joint.empty() || blocks.envelope.empty() || blocks.draw.empty()) {
                aurora::throw_host_exception<std::runtime_error>("J3D joint resource is missing INF1/JNT1/EVP1/DRW1");
            }
            return blocks;
        }

    }  // namespace

    struct J3dJointData::Storage {
        std::uint32_t flags = 0;
        std::uint32_t model_type = 0;
        bool draw_before_envelopes = false;
        std::unique_ptr<J3dNativeBlock<J3DModelInfoBlock>> info;
        std::unique_ptr<J3dNativeBlock<J3DJointBlock>> joint;
        std::unique_ptr<J3dNativeBlock<J3DEnvelopeBlock>> envelope;
        std::unique_ptr<J3dNativeBlock<J3DDrawBlock>> draw;
        std::vector<std::unique_ptr<J3DJoint>> joints;
        std::unique_ptr<J3DJoint*[]> joint_pointers;
        std::unique_ptr<J3DMtxCalc> basic;
        std::unique_ptr<JUTNameTab> names;
        std::unique_ptr<u16[]> important_indices;
        bool attached = false;

        Storage(Bytes bytes, std::uint32_t load_flags) {
            const auto blocks = find_blocks(bytes);
            flags = load_flags;
            model_type = blocks.model_type;
            draw_before_envelopes = blocks.draw.data() < blocks.envelope.data();
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
            std::vector<u16> remap(count);
            std::size_t init_count = 0;
            for (std::size_t i = 0; i < count; ++i) {
                remap[i] = u16_at(block, indices + i * 2);
                init_count = std::max(init_count, std::size_t{remap[i]} + 1);
            }
            require_range(block, init, init_count * 0x40);
            std::vector<J3DJointInitData> initializers(init_count);
            for (std::size_t i = 0; i < init_count; ++i) {
                const auto source = init + i * 0x40;
                auto& entry = initializers[i];
                entry.mKind = u16_at(block, source);
                entry.mScaleCompensate = block[source + 2];
                entry.mTransformInfo.mScale = vec_at(block, source + 4);
                entry.mTransformInfo.mRotation.x = std::bit_cast<s16>(u16_at(block, source + 0x10));
                entry.mTransformInfo.mRotation.y = std::bit_cast<s16>(u16_at(block, source + 0x12));
                entry.mTransformInfo.mRotation.z = std::bit_cast<s16>(u16_at(block, source + 0x14));
                entry.mTransformInfo.mTranslate = vec_at(block, source + 0x18);
                entry.mRadius = f32_at(block, source + 0x24);
                entry.mMin = vec_at(block, source + 0x28);
                entry.mMax = vec_at(block, source + 0x34);
            }
            using NativeJointBlock = J3dNativeBlock<J3DJointBlock>;
            NativeJointBlock::Builder builder;
            builder.header.mBlockType = u32_at(block, 0);
            builder.header.mBlockSize = u32_at(block, 4);
            builder.header.mJointNum = count;
            builder.header.mpJointInitData = NativeJointBlock::Builder::pointer_offset(builder.append<J3DJointInitData>(initializers));
            builder.header.mpIndexTable = NativeJointBlock::Builder::pointer_offset(builder.append<u16>(remap));
            const auto name_offset = u32_at(block, 0x14);
            if (name_offset != 0) {
                require_range(block, name_offset, 4);
                const J3dNameData decoded_names(block.subspan(name_offset));
                builder.header.mpNameTable = NativeJointBlock::Builder::pointer_offset(
                    builder.append_bytes(decoded_names.bytes(), alignof(ResNTAB)));
            }
            joint = std::move(builder).finish();
        }

        void load_info(Bytes block, std::uint32_t load_flags) {
            require_range(block, 0, 0x18);
            using NativeInfoBlock = J3dNativeBlock<J3DModelInfoBlock>;
            NativeInfoBlock::Builder builder;
            builder.header.mBlockType = u32_at(block, 0);
            builder.header.mBlockSize = u32_at(block, 4);
            builder.header.mFlags = u16_at(block, 8);
            builder.header.mPacketNum = u32_at(block, 0xc);
            builder.header.mVtxNum = u32_at(block, 0x10);
            if (((load_flags | builder.header.mFlags) & 0xf) > 2)
                aurora::throw_host_exception<std::runtime_error>("J3D joint resource selects no original matrix calculator");
            std::vector<J3DModelHierarchy> hierarchy;
            std::size_t cursor = table_offset(block, 0x14, 4);
            std::size_t depth = 0;
            for (bool done = false; !done;) {
                const auto type = u16_at(block, cursor);
                const auto value = u16_at(block, cursor + 2);
                hierarchy.push_back({type, value});
                cursor += 4;
                switch (type) {
                case 0:
                    if (depth != 0) {
                        aurora::throw_host_exception<std::runtime_error>("J3D joint hierarchy has unclosed child scopes");
                    }
                    done = true;
                    break;
                case 1: ++depth; break;
                case 2:
                    if (depth == 0) {
                        aurora::throw_host_exception<std::runtime_error>("J3D joint hierarchy closes an absent child scope");
                    }
                    --depth;
                    break;
                case 0x10:
                    if (value >= joint->header().mJointNum) {
                        aurora::throw_host_exception<std::runtime_error>("J3D hierarchy joint index is outside JNT1");
                    }
                    break;
                case 0x11:
                case 0x12:
                    // The complete owner validates these against its retained
                    // material/shape tables before original makeHierarchy.
                    break;
                default:
                    aurora::throw_host_exception<std::runtime_error>("J3D joint hierarchy contains an unknown command");
                }
            }
            builder.header.mpHierarchy = NativeInfoBlock::Builder::pointer_offset(builder.append<J3DModelHierarchy>(hierarchy));
            info = std::move(builder).finish();
        }

        void load_envelopes(Bytes block) {
            require_range(block, 0, 0x1c);
            const auto count = u16_at(block, 8);
            const auto counts = table_offset(block, 0xc, count);
            const auto mix_counts = block.subspan(counts, count);
            std::size_t total = 0;
            for (const auto mix_count : mix_counts) {
                // The original envelope calculation enters a do/while loop.
                if (mix_count == 0) {
                    aurora::throw_host_exception<std::runtime_error>("J3D envelope has no matrix influences");
                }
                total += mix_count;
            }
            const auto indices = table_offset(block, 0x10, total * 2);
            const auto weights = table_offset(block, 0x14, total * 4);
            std::vector<u16> mix_indices;
            std::vector<f32> mix_weights;
            mix_indices.reserve(total);
            mix_weights.reserve(total);
            std::size_t required_inverse_count = 0;
            for (std::size_t i = 0; i < total; ++i) {
                const auto index = u16_at(block, indices + i * 2);
                if (index >= joint->header().mJointNum) {
                    aurora::throw_host_exception<std::runtime_error>("J3D envelope influence is outside JNT1");
                }
                required_inverse_count = std::max(required_inverse_count, std::size_t{index} + 1);
                mix_indices.push_back(index);
                mix_weights.push_back(f32_at(block, weights + i * 4));
            }
            const auto inverse = table_offset(block, 0x18, required_inverse_count * sizeof(Mtx));
            std::vector<std::array<f32, 12>> inverse_matrices;
            if (inverse != 0) {
                // EVP1 has no inverse-table count. Retain every complete matrix
                // readable from this pointer within its block, including ones
                // not referenced by the current envelope set.
                const auto available = (block.size() - inverse) / sizeof(Mtx);
                inverse_matrices.resize(available);
                for (std::size_t i = 0; i < available; ++i) {
                    for (std::size_t row = 0; row < 3; ++row) {
                        for (std::size_t column = 0; column < 4; ++column) {
                            inverse_matrices[i][row * 4 + column] = f32_at(block, inverse + i * 48 + row * 16 + column * 4);
                        }
                    }
                }
            }
            using NativeEnvelopeBlock = J3dNativeBlock<J3DEnvelopeBlock>;
            NativeEnvelopeBlock::Builder builder;
            builder.header.mBlockType = u32_at(block, 0);
            builder.header.mBlockSize = u32_at(block, 4);
            builder.header.mWEvlpMtxNum = count;
            if (counts != 0)
                builder.header.mpWEvlpMixMtxNum = NativeEnvelopeBlock::Builder::pointer_offset(builder.append<u8>(mix_counts));
            if (indices != 0)
                builder.header.mpWEvlpMixIndex = NativeEnvelopeBlock::Builder::pointer_offset(builder.append<u16>(mix_indices));
            if (weights != 0)
                builder.header.mpWEvlpMixWeight = NativeEnvelopeBlock::Builder::pointer_offset(builder.append<f32>(mix_weights));
            if (inverse != 0)
                builder.header.mpInvJointMtx = NativeEnvelopeBlock::Builder::pointer_offset(builder.append<std::array<f32, 12>>(inverse_matrices));
            envelope = std::move(builder).finish();
        }

        void load_draw(Bytes block, std::uint16_t envelope_count_at_draw) {
            require_range(block, 0, 0x14);
            const auto serialized_count = u16_at(block, 8);
            if (serialized_count < envelope_count_at_draw) {
                aurora::throw_host_exception<std::runtime_error>("J3D draw matrix count underflows the original envelope subtraction");
            }
            // Retail readDraw (0x8043ea68) subtracts the envelope count here;
            // the serialized arrays themselves retain their complete extent.
            const auto draw_count = static_cast<u16>(serialized_count - envelope_count_at_draw);
            const auto flags_offset = table_offset(block, 0xc, serialized_count);
            const auto indices = table_offset(block, 0x10, serialized_count * 2U);
            const auto draw_flags = block.subspan(flags_offset, serialized_count);
            std::vector<u16> draw_indices;
            draw_indices.reserve(serialized_count);
            for (std::size_t i = 0; i < serialized_count; ++i) {
                draw_indices.push_back(u16_at(block, indices + i * 2));
            }
            u16 full_weight_count = 0;
            for (; full_weight_count < draw_count; ++full_weight_count) {
                if (draw_flags[full_weight_count] == 1) {
                    break;
                }
            }
            if (full_weight_count + envelope->header().mWEvlpMtxNum > draw_count) {
                aurora::throw_host_exception<std::runtime_error>("J3D important-matrix output exceeds the original draw allocation");
            }
            for (std::size_t i = 0; i < draw_count; ++i) {
                const auto extent = draw_flags[i] == 0 ? joint->header().mJointNum : envelope->header().mWEvlpMtxNum;
                if (draw_indices[i] >= extent) {
                    aurora::throw_host_exception<std::runtime_error>("J3D draw matrix index is outside its joint/envelope table");
                }
            }
            using NativeDrawBlock = J3dNativeBlock<J3DDrawBlock>;
            NativeDrawBlock::Builder builder;
            builder.header.mBlockType = u32_at(block, 0);
            builder.header.mBlockSize = u32_at(block, 4);
            builder.header.mMtxNum = serialized_count;
            if (flags_offset != 0)
                builder.header.mpDrawMtxFlag = NativeDrawBlock::Builder::pointer_offset(builder.append<u8>(draw_flags));
            if (indices != 0)
                builder.header.mpDrawMtxIndex = NativeDrawBlock::Builder::pointer_offset(builder.append<u16>(draw_indices));
            draw = std::move(builder).finish();
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
            aurora::throw_host_exception<std::logic_error>("J3D joint resource requires a fresh attachment destination");
        }
        auto& data = *_storage;
        // Original SDK readers populate the actual destination. The resource
        // owns only decoded metadata and the allocations they publish.
        J3DModelLoader_v26 loader;
        loader.mpModelData = &model;
        model.setModelDataType(data.model_type);
        loader.readInformation(&data.info->header(), data.flags);
        data.basic.reset(tree.mBasicMtxCalc);
        loader.readJoint(&data.joint->header());
        data.names.reset(tree.mJointName);
        data.joint_pointers.reset(tree.mJointNodePointer);
        {
            aurora::allocation::HostAllocationScope host;
            data.joints.reserve(tree.mJointNum);
            for (u16 i = 0; i < tree.mJointNum; ++i)
                data.joints.emplace_back(tree.mJointNodePointer[i]);
        }
        if (data.draw_before_envelopes) {
            loader.readDraw(&data.draw->header());
            loader.readEnvelop(&data.envelope->header());
        } else {
            loader.readEnvelop(&data.envelope->header());
            loader.readDraw(&data.draw->header());
        }
        data.important_indices.reset(tree.mWEvlpImportantMtxIdx);
        data.attached = true;
    }

}  // namespace smgpc::resource
