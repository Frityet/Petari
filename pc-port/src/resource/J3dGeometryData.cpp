#include "J3dGeometryData.hpp"

#include "J3dNameData.hpp"
#include "J3dNativeBlock.hpp"
#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/J3DGraphBase/J3DShape.hpp"
#include "JSystem/J3DGraphBase/J3DShapeDraw.hpp"
#include "JSystem/J3DGraphBase/J3DShapeMtx.hpp"
#include "JSystem/J3DGraphLoader/J3DShapeFactory.hpp"
#include "JSystem/JSupport/JSupport.hpp"
#include "JSystem/JUtility/JUTNameTab.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace smgpc::resource {
    namespace {
        using Bytes = std::span<const std::uint8_t>;

        Bytes checked(Bytes bytes, std::size_t offset, std::size_t size) {
            if (offset > bytes.size() || size > bytes.size() - offset) {
                throw std::runtime_error("J3D geometry data exceeds its containing resource");
            }
            return bytes.subspan(offset, size);
        }

        std::uint16_t u16_at(Bytes bytes, std::size_t offset) {
            const auto value = checked(bytes, offset, 2);
            return static_cast<std::uint16_t>((std::uint16_t{value[0]} << 8U) | value[1]);
        }

        std::uint32_t u32_at(Bytes bytes, std::size_t offset) {
            const auto value = checked(bytes, offset, 4);
            return (std::uint32_t{value[0]} << 24U) | (std::uint32_t{value[1]} << 16U) |
                   (std::uint32_t{value[2]} << 8U) | value[3];
        }

        std::size_t table_offset(Bytes block, std::size_t field, std::size_t required) {
            const auto offset = u32_at(block, field);
            if (offset == 0 && required != 0) {
                throw std::runtime_error("J3D geometry is missing a required table");
            }
            checked(block, offset, required);
            return offset;
        }

        // Preserve unnamed/padding bytes in fixed-stride SDK records as well as
        // their declared scalar fields. The record types retain their PPC sizes.
        template <typename T>
        T record_at(Bytes bytes, std::size_t offset) {
            T value;
            const auto source = checked(bytes, offset, sizeof(T));
            std::memcpy(&value, source.data(), sizeof(T));
            return value;
        }

        template <typename T>
        void half(T& value, Bytes source, std::size_t offset) {
            const auto native = u16_at(source, offset);
            std::memcpy(reinterpret_cast<std::uint8_t*>(&value) + offset, &native, 2);
        }

        template <typename T>
        void word(T& value, Bytes source, std::size_t offset) {
            const auto native = u32_at(source, offset);
            std::memcpy(reinterpret_cast<std::uint8_t*>(&value) + offset, &native, 4);
        }

        struct Blocks {
            Bytes file;
            Bytes info;
            Bytes vertex;
            Bytes shape;
            std::size_t vertex_offset = 0;
        };

        Blocks find_blocks(Bytes bytes) {
            checked(bytes, 0, 0x20);
            const auto type = u32_at(bytes, 4);
            if (u32_at(bytes, 0) != 0x4a334432U ||
                (type != 0x626d6432U && type != 0x626d6433U && type != 0x62646c34U)) {
                throw std::runtime_error("J3D geometry requires a J3D2 BMD2/BMD3/BDL4 model");
            }
            const auto size = u32_at(bytes, 8);
            if (size < 0x20) throw std::runtime_error("J3D geometry file header is truncated");
            Blocks result;
            result.file = checked(bytes, 0, size);
            std::size_t cursor = 0x20;
            for (std::uint32_t i = 0; i < u32_at(bytes, 0xc); ++i) {
                checked(result.file, cursor, 8);
                const auto block_size = u32_at(result.file, cursor + 4);
                if (block_size < 8) throw std::runtime_error("J3D geometry block header is truncated");
                const auto block = checked(result.file, cursor, block_size);
                Bytes* destination = nullptr;
                switch (u32_at(block, 0)) {
                case 0x494e4631U: destination = &result.info; break;
                case 0x56545831U:
                    destination = &result.vertex;
                    result.vertex_offset = cursor;
                    break;
                case 0x53485031U: destination = &result.shape; break;
                default: break;
                }
                if (destination != nullptr) {
                    if (!destination->empty()) throw std::runtime_error("J3D geometry has duplicate construction blocks");
                    *destination = block;
                }
                cursor += block_size;
            }
            if (result.info.empty() || result.vertex.empty() || result.shape.empty()) {
                throw std::runtime_error("J3D geometry is missing INF1/VTX1/SHP1");
            }
            return result;
        }

        std::size_t physical_end(Bytes block, std::size_t offset, std::span<const std::size_t> offsets) {
            auto result = block.size();
            for (const auto other : offsets) {
                if (other > offset) result = std::min(result, other);
            }
            return result;
        }

        struct ShapeDelete {
            void operator()(J3DShape* shape) const noexcept {
                if (shape == nullptr) return;
                for (std::size_t i = 0; i < shape->mMtxGroupNum; ++i) {
                    delete shape->mShapeMtx[i];
                    delete shape->mShapeDraw[i];
                }
                delete[] shape->mShapeMtx;
                delete[] shape->mShapeDraw;
                delete shape;
            }
        };

        struct AlignedDelete {
            void operator()(u8* pointer) const noexcept { std::free(pointer); }
        };

        bool valid_descriptor_attr(GXAttr attr) {
            return (attr >= GX_VA_PNMTXIDX && attr <= GX_VA_TEX7) || attr == GX_VA_NBT || attr == GX_VA_NULL;
        }

    }  // namespace

    struct J3dGeometryData::Storage {
        using VertexBlock = J3dNativeBlock<J3DVertexBlock>;
        using ShapeBlock = J3dNativeBlock<J3DShapeBlock>;
        std::unique_ptr<VertexBlock> vertex;
        std::unique_ptr<ShapeBlock> shape;
        std::unique_ptr<JUTNameTab> names;
        std::unique_ptr<u8, AlignedDelete> commands;
        std::vector<std::unique_ptr<J3DShape, ShapeDelete>> shapes;
        std::vector<J3DShape*> shape_pointers;
        std::uint32_t packet_count = 0;
        std::uint32_t vertex_count = 0;
        std::uint32_t normal_count = 0;
        std::uint32_t color_count = 0;
        std::uint32_t texcoord_count = 0;
        bool attached = false;

        Storage(Bytes bytes, std::uint32_t load_flags) {
            const auto blocks = find_blocks(bytes);
            checked(blocks.info, 0, 0x18);
            packet_count = u32_at(blocks.info, 0xc);
            vertex_count = u32_at(blocks.info, 0x10);
            load_vertex(blocks);
            load_shapes(blocks.shape, blocks.info, load_flags);
        }

        ~Storage() {
            // The original table can alias command pointers while sorting. Own
            // the factory's allocation once and invalidate its shared cache.
            const auto cached = reinterpret_cast<std::uintptr_t>(J3DShape::sOldVcdVatCmd);
            const auto begin = reinterpret_cast<std::uintptr_t>(commands.get());
            if (commands && cached >= begin && cached - begin < shape_pointers.size() * J3DShape::kVcdVatDLSize) {
                J3DShape::resetVcdVatCache();
            }
        }

        void load_vertex(const Blocks& blocks) {
            const auto block = blocks.vertex;
            checked(block, 0, 0x40);
            VertexBlock::Builder builder;
            builder.header.mBlockType = u32_at(block, 0);
            builder.header.mBlockSize = u32_at(block, 4);
            std::array<std::size_t, 14> offsets{};
            for (std::size_t i = 0; i < offsets.size(); ++i) {
                offsets[i] = table_offset(block, 8 + i * 4, 0);
            }
            auto cursor = table_offset(block, 8, sizeof(GXVtxAttrFmtList));
            std::vector<GXVtxAttrFmtList> formats;
            for (;;) {
                auto value = record_at<GXVtxAttrFmtList>(block, cursor);
                const auto source = checked(block, cursor, sizeof(value));
                word(value, source, 0);
                word(value, source, 4);
                word(value, source, 8);
                formats.push_back(value);
                cursor += sizeof(value);
                if (value.attr == GX_VA_NULL) break;
                const bool color = value.attr == GX_VA_CLR0 || value.attr == GX_VA_CLR1;
                if ((value.attr >= GX_VA_POS && value.attr <= GX_VA_TEX7) || value.attr == GX_VA_NBT) {
                    const auto max_type = color ? GX_RGBA8 : GX_F32;
                    const auto max_count = value.attr == GX_VA_NRM || value.attr == GX_VA_NBT ? 2U : 1U;
                    if (static_cast<unsigned>(value.type) > static_cast<unsigned>(max_type) ||
                        static_cast<unsigned>(value.cnt) > max_count) {
                        throw std::runtime_error("J3D vertex format has an invalid component type/count");
                    }
                }
            }
            builder.header.mpVtxAttrFmtList = VertexBlock::Builder::pointer_offset(builder.append<GXVtxAttrFmtList>(formats));
            const auto find_format = [&](GXAttr attr) {
                for (const auto& format : formats) {
                    if (format.attr == attr) return format;
                }
                return GXVtxAttrFmtList{attr, GX_POS_XYZ, GX_F32, 0};
            };
            const auto normal_size = find_format(GX_VA_NRM).type == GX_F32 ? 12U : 6U;
            const auto count_to = [&](std::size_t begin, std::size_t end, std::uint32_t stride) -> std::uint32_t {
                if (begin == 0) return 0;
                if (end < begin) throw std::runtime_error("J3D vertex count boundary precedes its array");
                return static_cast<std::uint32_t>((end - begin) / stride + 1);
            };
            // Original readVertex uses these specific successor choices and +1;
            // native placement changes pointer distances, so derive from source.
            const auto normal_end = offsets[3] ? offsets[3] : offsets[4] ? offsets[4] : offsets[6] ? offsets[6] : block.size();
            const auto color_end = offsets[5] ? offsets[5] : offsets[6] ? offsets[6] : block.size();
            normal_count = count_to(offsets[2], normal_end, normal_size);
            color_count = count_to(offsets[4], color_end, 4);
            texcoord_count = count_to(offsets[6], block.size(), 8);

            const auto array = [&](std::size_t index, GXAttr attr, std::size_t footprint) -> void* {
                const auto source_offset = offsets[index];
                if (source_offset == 0) return nullptr;
                const auto format = find_format(attr);
                const bool color = attr == GX_VA_CLR0 || attr == GX_VA_CLR1;
                const auto scalar_size = color ? 1U : format.type == GX_F32 ? 4U :
                                         (format.type == GX_U16 || format.type == GX_S16) ? 2U : 1U;
                const auto physical_size = physical_end(block, source_offset, offsets) - source_offset;
                const auto size = std::max(physical_size, footprint);
                const auto aligned_size = (size + scalar_size - 1) / scalar_size * scalar_size;
                // Original CPU-count footprints may read into the following
                // table/block. Copy those actual bytes, never synthesize padding.
                const auto source = checked(blocks.file, blocks.vertex_offset + source_offset, aligned_size);
                std::size_t native_offset;
                if (scalar_size == 4) {
                    std::vector<float> values(source.size() / 4);
                    for (std::size_t i = 0; i < values.size(); ++i) values[i] = std::bit_cast<float>(u32_at(source, i * 4));
                    native_offset = builder.append<float>(values, 32);
                } else if (scalar_size == 2) {
                    std::vector<std::uint16_t> values(source.size() / 2);
                    for (std::size_t i = 0; i < values.size(); ++i) values[i] = u16_at(source, i * 2);
                    native_offset = builder.append<std::uint16_t>(values, 32);
                } else {
                    std::vector<std::uint8_t> values(source.begin(), source.end());
                    if (color && std::endian::native == std::endian::little) {
                        // J3D binds colors with stride four even for packed
                        // formats. Aurora's native arrays read numeric16/24 LE;
                        // RGB/RGBA byte-channel formats remain unchanged.
                        const auto packed = format.type == GX_RGB565 || format.type == GX_RGBA4 ? 2U :
                                            format.type == GX_RGBA6 ? 3U : 1U;
                        if (packed > 1) {
                            for (std::size_t i = 0; i + packed <= values.size(); i += 4) {
                                std::reverse(values.begin() + i, values.begin() + i + packed);
                            }
                        }
                    }
                    native_offset = builder.append_bytes(values, 32);
                }
                return VertexBlock::Builder::pointer_offset(native_offset);
            };
            const auto pos_size = find_format(GX_VA_POS).type == GX_F32 ? 12U : 6U;
            builder.header.mpVtxPosArray = array(1, GX_VA_POS, std::size_t{vertex_count} * pos_size);
            builder.header.mpVtxNrmArray = array(2, GX_VA_NRM, std::size_t{normal_count} * normal_size);
            // NBT array scalars use the normal format when the descriptor selects
            // NBT; the original array-binding code obtains its stride from NRM.
            builder.header.mpVtxNBTArray = array(3, GX_VA_NRM, 0);
            builder.header.mpVtxColorArray[0] = array(4, GX_VA_CLR0, std::size_t{color_count} * 4);
            builder.header.mpVtxColorArray[1] = array(5, GX_VA_CLR1, 0);
            for (std::size_t i = 0; i < 8; ++i) {
                builder.header.mpVtxTexCoordArray[i] = array(6 + i, static_cast<GXAttr>(GX_VA_TEX0 + i), i == 0 ? std::size_t{texcoord_count} * 8 : 0);
            }
            vertex = std::move(builder).finish();
        }

        void load_shapes(Bytes block, Bytes info, std::uint32_t flags) {
            static_assert(sizeof(J3DShapeInitData) == 40 && sizeof(J3DShapeMtxInitData) == 8 &&
                          sizeof(J3DShapeDrawInitData) == 8 && sizeof(GXVtxDescList) == 8);
            checked(block, 0, 0x2c);
            ShapeBlock::Builder builder;
            builder.header.mBlockType = u32_at(block, 0);
            builder.header.mBlockSize = u32_at(block, 4);
            builder.header.mShapeNum = u16_at(block, 8);
            const auto count = builder.header.mShapeNum;
            const auto remap = table_offset(block, 0x10, count * 2U);
            std::vector<u16> indices(count);
            std::size_t init_count = 0;
            for (std::size_t i = 0; i < count; ++i) {
                indices[i] = u16_at(block, remap + i * 2);
                init_count = std::max(init_count, std::size_t{indices[i]} + 1);
            }
            const auto init = table_offset(block, 0xc, init_count * 40);
            std::vector<J3DShapeInitData> initializers(init_count);
            for (std::size_t i = 0; i < init_count; ++i) {
                auto& value = initializers[i];
                value = record_at<J3DShapeInitData>(block, init + i * 40);
                const auto source = checked(block, init + i * 40, 40);
                for (std::size_t offset = 2; offset <= 8; offset += 2) half(value, source, offset);
                for (std::size_t offset = 12; offset < 40; offset += 4) word(value, source, offset);
            }
            std::size_t mtx_init_count = 0;
            std::size_t draw_init_count = 0;
            std::size_t desc_count = 0;
            const auto desc = table_offset(block, 0x18, count == 0 ? 0 : 8);
            for (const auto index : indices) {
                const auto& value = initializers[index];
                if (value.mShapeMtxType > 3) throw std::runtime_error("J3D shape has no original matrix class for its type");
                mtx_init_count = std::max(mtx_init_count, std::size_t{value.mMtxInitDataIndex} + value.mMtxGroupNum);
                draw_init_count = std::max(draw_init_count, std::size_t{value.mDrawInitDataIndex} + value.mMtxGroupNum);
                if (value.mVtxDescListIndex % sizeof(GXVtxDescList) != 0) throw std::runtime_error("J3D shape descriptor offset is unaligned");
                std::size_t row = value.mVtxDescListIndex / sizeof(GXVtxDescList);
                for (;;) {
                    const auto attr = static_cast<GXAttr>(u32_at(block, desc + row * 8));
                    const auto type = u32_at(block, desc + row * 8 + 4);
                    ++row;
                    if (!valid_descriptor_attr(attr) || (attr != GX_VA_NULL && type > GX_INDEX16)) {
                        throw std::runtime_error("J3D shape has an invalid GX vertex descriptor");
                    }
                    if (attr == GX_VA_NULL) break;
                }
                desc_count = std::max(desc_count, row);
            }
            std::vector<GXVtxDescList> descriptors(desc_count);
            for (std::size_t i = 0; i < desc_count; ++i) {
                descriptors[i] = {static_cast<GXAttr>(u32_at(block, desc + i * 8)), static_cast<GXAttrType>(u32_at(block, desc + i * 8 + 4))};
            }
            const auto mtx_init = table_offset(block, 0x24, mtx_init_count * 8);
            const auto draw_init = table_offset(block, 0x28, draw_init_count * 8);
            std::vector<J3DShapeMtxInitData> matrices(mtx_init_count);
            std::vector<J3DShapeDrawInitData> draws(draw_init_count);
            for (std::size_t i = 0; i < matrices.size(); ++i) {
                matrices[i] = {u16_at(block, mtx_init + i * 8), u16_at(block, mtx_init + i * 8 + 2), u32_at(block, mtx_init + i * 8 + 4)};
            }
            for (std::size_t i = 0; i < draws.size(); ++i) {
                draws[i] = {u32_at(block, draw_init + i * 8), u32_at(block, draw_init + i * 8 + 4)};
            }
            std::size_t matrix_count = 0;
            std::size_t display_size = 0;
            for (const auto index : indices) {
                const auto& value = initializers[index];
                for (std::size_t group = 0; group < value.mMtxGroupNum; ++group) {
                    const auto& matrix = matrices[value.mMtxInitDataIndex + group];
                    const auto& draw = draws[value.mDrawInitDataIndex + group];
                    if (value.mShapeMtxType == 3) {
                        matrix_count = std::max(matrix_count, std::size_t{matrix.mFirstUseMtxIndex} + matrix.mUseMtxCount);
                    }
                    display_size = std::max(display_size, std::size_t{draw.mDisplayListIndex} + draw.mDisplayListSize);
                }
            }
            const auto mtx = table_offset(block, 0x1c, matrix_count * 2);
            std::vector<u16> matrix_indices(matrix_count);
            for (std::size_t i = 0; i < matrix_indices.size(); ++i) matrix_indices[i] = u16_at(block, mtx + i * 2);
            for (const auto index : indices) {
                const auto& value = initializers[index];
                if (value.mShapeMtxType != 3) continue;
                for (std::size_t group = 0; group < value.mMtxGroupNum; ++group) {
                    const auto& matrix = matrices[value.mMtxInitDataIndex + group];
                    for (std::size_t slot = 10; slot < matrix.mUseMtxCount; ++slot) {
                        // Original multi-matrix loops skip carry entries before
                        // indexing their ten-slot load cache. Keep that suffix.
                        if (matrix_indices[matrix.mFirstUseMtxIndex + slot] != 0xffff) {
                            throw std::runtime_error("J3D shape has a live matrix beyond its original ten load slots");
                        }
                    }
                }
            }
            const auto display = table_offset(block, 0x20, display_size);
            builder.header.mpShapeInitData = init == 0 ? nullptr : ShapeBlock::Builder::pointer_offset(builder.append<J3DShapeInitData>(initializers));
            builder.header.mpIndexTable = remap == 0 ? nullptr : ShapeBlock::Builder::pointer_offset(builder.append<u16>(indices));
            builder.header.mpVtxDescList = desc == 0 ? nullptr : ShapeBlock::Builder::pointer_offset(builder.append<GXVtxDescList>(descriptors));
            builder.header.mpMtxTable = mtx == 0 ? nullptr : ShapeBlock::Builder::pointer_offset(builder.append<u16>(matrix_indices));
            builder.header.mpDisplayListData = display == 0 ? nullptr : ShapeBlock::Builder::pointer_offset(builder.append_bytes(checked(block, display, display_size), 32));
            builder.header.mpMtxInitData = mtx_init == 0 ? nullptr : ShapeBlock::Builder::pointer_offset(builder.append<J3DShapeMtxInitData>(matrices));
            builder.header.mpDrawInitData = draw_init == 0 ? nullptr : ShapeBlock::Builder::pointer_offset(builder.append<J3DShapeDrawInitData>(draws));
            const auto name_offset = table_offset(block, 0x14, 0);
            if (name_offset != 0) {
                J3dNameData source_names(block.subspan(name_offset));
                builder.header.mpNameTable = ShapeBlock::Builder::pointer_offset(builder.append_bytes(source_names.bytes(), alignof(ResNTAB)));
            }
            shape = std::move(builder).finish();

            // Establish complete, bounded INF coverage before original readShape
            // construction. Retain repeated commands exactly: retail recreates
            // that logical slot, while this owner also retains prior allocations.
            std::vector<u16> order;
            std::vector<bool> seen(count);
            std::size_t cursor = table_offset(info, 0x14, 4);
            for (;;) {
                const auto type = u16_at(info, cursor);
                const auto value = u16_at(info, cursor + 2);
                cursor += 4;
                if (type == 0) break;
                if (type == 0x12) {
                    if (value >= count) throw std::runtime_error("J3D hierarchy shape index is outside SHP1");
                    order.push_back(value);
                    seen[value] = true;
                }
            }
            if (std::find(seen.begin(), seen.end(), false) != seen.end()) {
                throw std::runtime_error("J3D hierarchy leaves an original shape pointer uninitialized");
            }
            J3DShapeFactory factory(shape->header());
            factory.allocVcdVatCmdBuffer(count);
            commands.reset(factory.mVcdVatCmdBuffer);
            shapes.reserve(order.size());
            shape_pointers.resize(count);
            GXVtxDescList* previous = nullptr;
            for (const auto index : order) {
                shapes.emplace_back(factory.create(index, flags, previous));
                shape_pointers[index] = shapes.back().get();
                previous = factory.getVtxDescList(index);
            }
            if (shape->header().mpNameTable != nullptr) {
                names = std::make_unique<JUTNameTab>(JSUConvertOffsetToPtr<ResNTAB>(&shape->header(), shape->header().mpNameTable));
            }
        }
    };

    J3dGeometryData::J3dGeometryData(Bytes bytes, std::uint32_t flags) : _storage(std::make_unique<Storage>(bytes, flags)) {}
    J3dGeometryData::~J3dGeometryData() = default;
    J3dGeometryData::J3dGeometryData(J3dGeometryData&&) noexcept = default;

    void J3dGeometryData::attach_to(J3DModelData& model) {
        if (!_storage || _storage->attached || model.mShapeTable.mShapeNodePointer != nullptr ||
            model.mShapeTable.mShapeNum != 0 || model.mVertexData.mVtxAttrFmtList != nullptr) {
            throw std::logic_error("J3D geometry can attach once to fresh original vertex/shape tables");
        }
        auto& storage = *_storage;
        auto& vertex = model.mVertexData;
        auto& block = storage.vertex->header();
        vertex.mPacketNum = storage.packet_count;
        vertex.mVtxNum = storage.vertex_count;
        vertex.mNrmNum = storage.normal_count;
        vertex.mColNum = storage.color_count;
        vertex.mTexCoordNum = storage.texcoord_count;
        vertex.mVtxAttrFmtList = JSUConvertOffsetToPtr<GXVtxAttrFmtList>(&block, block.mpVtxAttrFmtList);
        vertex.mVtxPosArray = JSUConvertOffsetToPtr<void>(&block, block.mpVtxPosArray);
        vertex.mVtxNrmArray = JSUConvertOffsetToPtr<void>(&block, block.mpVtxNrmArray);
        vertex.mVtxNBTArray = JSUConvertOffsetToPtr<void>(&block, block.mpVtxNBTArray);
        for (std::size_t i = 0; i < 2; ++i) vertex.mVtxColorArray[i] = JSUConvertOffsetToPtr<GXColor>(&block, block.mpVtxColorArray[i]);
        for (std::size_t i = 0; i < 8; ++i) vertex.mVtxTexCoordArray[i] = JSUConvertOffsetToPtr<void>(&block, block.mpVtxTexCoordArray[i]);
        model.mShapeTable.mShapeNum = storage.shape->header().mShapeNum;
        model.mShapeTable.mShapeNodePointer = storage.shape_pointers.data();
        model.mShapeTable.mShapeName = storage.names.get();
        storage.attached = true;
    }

    const J3DShapeBlock& J3dGeometryData::shape_block() const {
        if (!_storage) throw std::logic_error("J3D geometry owner has been moved");
        return _storage->shape->header();
    }

}  // namespace smgpc::resource
