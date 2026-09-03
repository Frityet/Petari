#include "J3dMaterialBlockData.hpp"

#include "J3dNameData.hpp"
#include "J3dNativeBlock.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"
#include "JSystem/J3DGraphLoader/J3DMaterialFactory.hpp"
#include "JSystem/JUtility/JUTNameTab.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <vector>

namespace smgpc::resource {
    namespace {
        using Bytes = std::span<const std::uint8_t>;

        Bytes checked(Bytes bytes, std::size_t offset, std::size_t size) {
            if (offset > bytes.size() || size > bytes.size() - offset) {
                throw std::runtime_error("J3D material metadata exceeds its containing block");
            }
            return bytes.subspan(offset, size);
        }

        std::uint16_t u16_at(Bytes bytes, std::size_t offset) {
            const auto value = checked(bytes, offset, 2);
            return static_cast<std::uint16_t>(std::uint16_t(value[0]) << 8 | value[1]);
        }
        std::uint32_t u32_at(Bytes bytes, std::size_t offset) {
            const auto value = checked(bytes, offset, 4);
            return std::uint32_t(value[0]) << 24 | std::uint32_t(value[1]) << 16 | std::uint32_t(value[2]) << 8 | value[3];
        }

        // Modifying the byte representation of these standard-layout metadata
        // records preserves opaque fields. Declared scalars are then native endian.
        template <typename T>
        void half(T& record, Bytes source, std::size_t offset) {
            const auto value = u16_at(source, offset);
            std::memcpy(reinterpret_cast<std::uint8_t*>(&record) + offset, &value, sizeof(value));
        }
        template <typename T>
        void word(T& record, Bytes source, std::size_t offset) {
            const auto value = u32_at(source, offset);
            std::memcpy(reinterpret_cast<std::uint8_t*>(&record) + offset, &value, sizeof(value));
        }
        template <typename T>
        void words(T& record, Bytes source, std::size_t first, std::size_t end) {
            for (auto offset = first; offset < end; offset += 4) word(record, source, offset);
        }
        template <typename T>
        void halves(T& record, Bytes source, std::size_t first, std::size_t end) {
            for (auto offset = first; offset < end; offset += 2) half(record, source, offset);
        }
        struct ByteFields {
            template <typename T> void operator()(T&, Bytes) const {}
        };
        struct WordFields {
            template <typename T> void operator()(T& value, Bytes source) const { words(value, source, 0, sizeof(T)); }
        };
        struct HalfFields {
            template <typename T> void operator()(T& value, Bytes source) const { halves(value, source, 0, sizeof(T)); }
        };

        void texture_matrix(J3DTexMtxInfo& value, Bytes source) {
            words(value, source, 4, 0x18);
            half(value, source, 0x18);
            words(value, source, 0x1C, sizeof(value));
        }
        void fog(J3DFogInfo& value, Bytes source) {
            half(value, source, 2);
            words(value, source, 4, 0x14);
            halves(value, source, 0x18, sizeof(value));
        }
        void nbt(J3DNBTScaleInfo& value, Bytes source) { words(value, source, 4, sizeof(value)); }

        struct SourceBlock {
            Bytes bytes;
            std::vector<std::uint32_t> offsets;

            SourceBlock(Bytes data, std::size_t header_size) : bytes(data) {
                checked(data, 0, header_size);
                for (std::size_t field = 0xC; field < header_size; field += 4) {
                    const auto offset = u32_at(bytes, field);
                    if (offset != 0) {
                        checked(bytes, offset, 0);
                        offsets.push_back(offset);
                    }
                }
                offsets.push_back(static_cast<std::uint32_t>(bytes.size()));
                std::sort(offsets.begin(), offsets.end());
            }

            Bytes table(std::size_t field) const {
                const auto offset = u32_at(bytes, field);
                if (offset == 0) return {};
                const auto end = std::upper_bound(offsets.begin(), offsets.end(), offset);
                return checked(bytes, offset, (end == offsets.end() ? bytes.size() : *end) - offset);
            }

            void require_records(std::size_t field, std::size_t stride, std::size_t count) const {
                if (count > table(field).size() / stride) {
                    throw std::runtime_error("J3D material table does not contain its referenced records");
                }
            }
        };

        void validate_material(const SourceBlock& source) {
            const auto count = u16_at(source.bytes, 8);
            source.require_records(0x10, 2, count);
            const auto ids = source.table(0x10);
            const auto init = source.table(0xC);
            const auto reference = [&](std::size_t field, std::size_t stride, std::uint32_t index, std::uint32_t sentinel) {
                if (index != sentinel) source.require_records(field, stride, std::size_t(index) + 1);
            };
            for (std::size_t i = 0; i < count; ++i) {
                const auto id = u16_at(ids, i * 2);
                source.require_records(0xC, 0x14C, std::size_t(id) + 1);
                const auto row = init.subspan(std::size_t(id) * 0x14C, 0x14C);
                constexpr std::array<std::pair<std::size_t, std::size_t>, 7> byte_columns{{
                    {0x1C, 4}, {0x24, 1}, {0x34, 1}, {0x58, 1}, {0x78, 1}, {0x74, 4}, {0x7C, 1}}};
                for (std::size_t field = 0; field < byte_columns.size(); ++field) {
                    const auto [column, stride] = byte_columns[field];
                    reference(column, stride, row[field + 1], 0xff);
                }
                struct Range { std::size_t first, count, column, stride; };
                constexpr Range ranges[]{
                    {8, 2, 0x20, 4}, {0xC, 4, 0x28, 8}, {0x14, 2, 0x2C, 4},
                    {0x28, 8, 0x38, 4}, {0x48, 8, 0x40, 0x64}, {0x84, 8, 0x48, 2},
                    {0x94, 4, 0x54, 4}, {0xBC, 16, 0x4C, 4}, {0xDC, 4, 0x50, 8},
                    {0xE4, 16, 0x5C, 0x14}, {0x104, 16, 0x60, 4}, {0x124, 4, 0x64, 4},
                    {0x144, 1, 0x68, 0x2C}, {0x146, 1, 0x6C, 8}, {0x148, 1, 0x70, 4}, {0x14A, 1, 0x80, 0x10}};
                for (const auto& range : ranges) {
                    for (std::size_t j = 0; j < range.count; ++j) {
                        reference(range.column, range.stride, u16_at(row, range.first + j * 2), 0xffff);
                    }
                }
                if (row[3] != 0xff && source.table(0x34)[row[3]] > 8) {
                    throw std::runtime_error("J3D material exceeds the eight texture-coordinate slots");
                }
                if (row[4] != 0xff && source.table(0x58)[row[4]] > 16) {
                    throw std::runtime_error("J3D material exceeds the sixteen TEV-stage slots");
                }
            }
            const auto indirect = u32_at(source.bytes, 0x18), names = u32_at(source.bytes, 0x14);
            if (indirect != 0 && std::uint32_t(indirect - names) > 4) {
                // These records use logical material indices, independently of
                // the MAT3 initialization-record remap.
                source.require_records(0x18, 0x138, count);
                const auto records = source.table(0x18);
                for (std::size_t i = 0; i < count; ++i) {
                    if (records[i * 0x138] == 1 && records[i * 0x138 + 1] > 3) {
                        throw std::runtime_error("J3D indirect material exceeds its three matrix slots");
                    }
                }
            }
        }

        template <typename Header>
        class Decoder {
        public:
            using Block = J3dNativeBlock<Header>;
            typename Block::Builder builder;
            SourceBlock source;

            Decoder(Bytes bytes, std::size_t header_size) : source(bytes, header_size) {
                builder.header.mBlockType = u32_at(bytes, 0);
                builder.header.mBlockSize = u32_at(bytes, 4);
                builder.header.mMaterialNum = u16_at(bytes, 8);
            }

            template <typename T, typename Convert>
            void* table(std::size_t field, std::size_t stride, Convert convert) {
                static_assert(std::is_standard_layout_v<T>);
                if (sizeof(T) != stride) throw std::logic_error("J3D material record layout differs from its decoder");
                const auto offset = u32_at(source.bytes, field);
                if (offset == 0) return nullptr;
                const auto bytes = source.table(field);
                std::vector<T> records(bytes.size() / stride);
                for (std::size_t i = 0; i < records.size(); ++i) {
                    const auto row = bytes.subspan(i * stride, stride);
                    std::memcpy(&records[i], row.data(), stride);
                    convert(records[i], row);
                }
                const auto native = builder.template append<T>(records);
                // Keep trailing authored bytes too; table alignment is not an
                // implicit count field and must not become synthetic records.
                if (const auto tail = bytes.size() % stride; tail != 0) {
                    (void)builder.append_bytes(bytes.last(tail));
                }
                return Block::Builder::pointer_offset(native);
            }

            void* opaque(std::size_t field) {
                if (u32_at(source.bytes, field) == 0) return nullptr;
                return Block::Builder::pointer_offset(builder.append_bytes(source.table(field), 4));
            }

            void* names(std::size_t field) {
                if (u32_at(source.bytes, field) == 0) return nullptr;
                J3dNameData names(source.table(field));
                return Block::Builder::pointer_offset(builder.append_bytes(names.bytes(), alignof(ResNTAB)));
            }
        };

        auto decode_material(Bytes bytes) {
            Decoder<J3DMaterialBlock> decoder(bytes, 0x84);
            validate_material(decoder.source);
            auto& block = decoder.builder.header;
            block.mpMaterialInitData = decoder.table<J3DMaterialInitData>(0xC, 0x14C, [](auto& value, Bytes row) {
                for (const auto [first, end] : std::array<std::pair<std::size_t, std::size_t>, 8>{{
                         {8, 0x18}, {0x28, 0x38}, {0x48, 0x58}, {0x84, 0x9C},
                         {0xBC, 0x104}, {0x104, 0x124}, {0x124, 0x12C}, {0x144, 0x14C}}}) {
                    halves(value, row, first, end);
                }
            });
            block.mpMaterialID = decoder.table<u16>(0x10, 2, HalfFields{});
            block.mpNameTable = decoder.names(0x14);
            // The retail constructor ignores the legacy four-byte indirect
            // placeholder. Preserve that decision from original file offsets.
            const auto indirect = u32_at(bytes, 0x18), names = u32_at(bytes, 0x14);
            if (indirect != 0 && std::uint32_t(indirect - names) > 4) {
                // The original factory repeats this offset-distance test. A
                // native empty name table is four bytes; preserve the accepted
                // branch even if its source alignment padding was larger.
                (void)decoder.builder.append_bytes(std::array<u8, 4>{});
                block.mpIndInitData = decoder.table<J3DIndInitData>(0x18, 0x138, [](auto& value, Bytes row) {
                    // Retail compares the stored byte with exactly one.
                    value.mEnabled = row[0] == 1;
                    for (std::size_t i = 0; i < 3; ++i) words(value, row, 0x14 + i * 28, 0x2C + i * 28);
                });
            }
            block.mpCullMode = decoder.table<GXCullMode>(0x1C, 4, WordFields{});
            block.mpMatColor = decoder.table<GXColor>(0x20, 4, ByteFields{});
            block.mpColorChanNum = decoder.table<u8>(0x24, 1, ByteFields{});
            block.mpColorChanInfo = decoder.table<J3DColorChanInfo>(0x28, 8, ByteFields{});
            block.mpAmbColor = decoder.table<GXColor>(0x2C, 4, ByteFields{});
            block.mpLightInfo = decoder.table<J3DLightInfo>(0x30, 0x34, [](auto& value, Bytes row) {
                words(value, row, 0, 0x18);
                words(value, row, 0x1C, 0x34);
            });
            block.mpTexGenNum = decoder.table<u8>(0x34, 1, ByteFields{});
            block.mpTexCoordInfo = decoder.table<J3DTexCoordInfo>(0x38, 4, ByteFields{});
            block.mpTexCoord2Info = decoder.opaque(0x3C);
            block.mpTexMtxInfo = decoder.table<J3DTexMtxInfo>(0x40, 0x64, texture_matrix);
            block.field_0x44 = decoder.table<J3DTexMtxInfo>(0x44, 0x64, texture_matrix);
            block.mpTexNo = decoder.table<u16>(0x48, 2, HalfFields{});
            block.mpTevOrderInfo = decoder.table<J3DTevOrderInfo>(0x4C, 4, ByteFields{});
            block.mpTevColor = decoder.table<GXColorS10>(0x50, 8, HalfFields{});
            block.mpTevKColor = decoder.table<GXColor>(0x54, 4, ByteFields{});
            block.mpTevStageNum = decoder.table<u8>(0x58, 1, ByteFields{});
            block.mpTevStageInfo = decoder.table<J3DTevStageInfo>(0x5C, 0x14, ByteFields{});
            block.mpTevSwapModeInfo = decoder.table<J3DTevSwapModeInfo>(0x60, 4, ByteFields{});
            block.mpTevSwapModeTableInfo = decoder.table<J3DTevSwapModeTableInfo>(0x64, 4, ByteFields{});
            block.mpFogInfo = decoder.table<J3DFogInfo>(0x68, 0x2C, fog);
            block.mpAlphaCompInfo = decoder.table<J3DAlphaCompInfo>(0x6C, 8, ByteFields{});
            block.mpBlendInfo = decoder.table<J3DBlendInfo>(0x70, 4, ByteFields{});
            block.mpZModeInfo = decoder.table<J3DZModeInfo>(0x74, 4, ByteFields{});
            block.mpZCompLoc = decoder.table<u8>(0x78, 1, ByteFields{});
            block.mpDither = decoder.table<u8>(0x7C, 1, ByteFields{});
            block.mpNBTScaleInfo = decoder.table<J3DNBTScaleInfo>(0x80, 0x10, nbt);
            return std::move(decoder.builder).finish();
        }

        auto decode_display_list(Bytes bytes) {
            using Block = J3dNativeBlock<J3DMaterialDLBlock>;
            Decoder<J3DMaterialDLBlock> decoder(bytes, 0x24);
            auto& block = decoder.builder.header;
            const auto count = block.mMaterialNum;
            decoder.source.require_records(0x10, 0x10, count);
            decoder.source.require_records(0x14, 8, count);
            decoder.source.require_records(0x18, 1, count);
            const auto source_init = u32_at(bytes, 0xC);
            if (count != 0 && source_init == 0) throw std::runtime_error("MDL3 has no display-list descriptors");
            decoder.source.require_records(0xC, 8, count);
            std::vector<J3DDisplayListInit> lists(count);
            for (std::size_t i = 0; i < count; ++i) {
                // Wii relative offsets are unsigned32 arithmetic. Resolve them
                // in source coordinates before widening the native header.
                const auto entry = source_init + static_cast<u32>(i * 8);
                const auto start = u32(entry + u32_at(bytes, entry));
                const auto size = u32_at(bytes, entry + 4);
                checked(bytes, start, size);
                lists[i].mOffset = start;
                lists[i].field_0x4 = size;
            }
            const auto native_init = decoder.builder.template append<J3DDisplayListInit>(lists);
            block.mpDisplayListInit = Block::Builder::pointer_offset(native_init);
            // A single retained command image preserves aliasing, patch offsets
            // and every byte in each original display-list extent.
            const auto command_image = decoder.builder.append_bytes(bytes, 32);
            auto native_lists = decoder.builder.template edit_array<J3DDisplayListInit>(native_init);
            for (std::size_t i = 0; i < count; ++i) {
                const auto delta = command_image + native_lists[i].mOffset - (native_init + i * sizeof(J3DDisplayListInit));
                if (delta > std::numeric_limits<u32>::max()) throw std::length_error("MDL3 native relative offset exceeds32 bits");
                native_lists[i].mOffset = static_cast<u32>(delta);
            }
            block.mpPatchingInfo = decoder.table<J3DPatchingInfo>(0x10, 0x10, [](auto& value, Bytes row) { halves(value, row, 0, 0xC); });
            block.mpCurrentMtxInfo = decoder.table<J3DCurrentMtxInfo>(0x14, 8, WordFields{});
            block.mpMaterialMode = decoder.table<u8>(0x18, 1, ByteFields{});
            block._1C = decoder.opaque(0x1C);
            block.mpNameTable = decoder.names(0x20);
            return std::move(decoder.builder).finish();
        }
    }

    struct J3dMaterialBlockData::Storage {
        std::vector<std::uint8_t> source;
        std::unique_ptr<J3dNativeBlock<J3DMaterialBlock>> material;
        std::unique_ptr<J3dNativeBlock<J3DMaterialDLBlock>> display_list;

        explicit Storage(Bytes bytes) : source(bytes.begin(), bytes.end()) {
            const auto size = u32_at(source, 4);
            if (size < 8 || size != source.size()) throw std::runtime_error("J3D material block must have its complete declared extent");
            switch (u32_at(source, 0)) {
            case 0x4D415433: material = decode_material(source); break;
            case 0x4D444C33: display_list = decode_display_list(source); break;
            default: throw std::runtime_error("Expected a MAT3 or MDL3 block for original material construction");
            }
        }
    };

    J3dMaterialBlockData::J3dMaterialBlockData(Bytes block) : _storage(std::make_unique<Storage>(block)) {}
    J3dMaterialBlockData::~J3dMaterialBlockData() = default;

    const J3DMaterialBlock& J3dMaterialBlockData::material() const {
        if (!_storage->material) throw std::logic_error("This resource contains MDL3 metadata");
        return _storage->material->header();
    }
    const J3DMaterialDLBlock& J3dMaterialBlockData::display_list() const {
        if (!_storage->display_list) throw std::logic_error("This resource contains MAT3 metadata");
        return _storage->display_list->header();
    }
    Bytes J3dMaterialBlockData::source_bytes() const { return _storage->source; }
}
