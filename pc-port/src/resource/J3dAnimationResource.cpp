#include "J3dAnimationResource.hpp"

#include "J3dNameData.hpp"
#include "J3dNativeBlock.hpp"
#include "J3dTransformAnimation.hpp"
#include "compat/JkrAllocationDomain.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace smgpc::resource {
    namespace {
        constexpr std::uint32_t tag(char a, char b, char c, char d) {
            return (std::uint32_t(static_cast<unsigned char>(a)) << 24) | (std::uint32_t(static_cast<unsigned char>(b)) << 16) |
                   (std::uint32_t(static_cast<unsigned char>(c)) << 8) | std::uint32_t(static_cast<unsigned char>(d));
        }
        constexpr auto J3D1 = tag('J', '3', 'D', '1');
        constexpr auto ANK1 = tag('A', 'N', 'K', '1'), ANF1 = tag('A', 'N', 'F', '1');
        constexpr auto PAK1 = tag('P', 'A', 'K', '1'), PAF1 = tag('P', 'A', 'F', '1');
        constexpr auto TTK1 = tag('T', 'T', 'K', '1'), TRK1 = tag('T', 'R', 'K', '1');
        constexpr auto TPT1 = tag('T', 'P', 'T', '1'), VAF1 = tag('V', 'A', 'F', '1');
        constexpr auto CLK1 = tag('C', 'L', 'K', '1'), CLF1 = tag('C', 'L', 'F', '1');
        constexpr auto VCK1 = tag('V', 'C', 'K', '1'), VCF1 = tag('V', 'C', 'F', '1');

        void require_range(std::size_t size, std::size_t offset, std::size_t count) {
            if (offset > size || count > size - offset)
                throw std::runtime_error("J3D animation range exceeds its containing block");
        }
        struct Reader {
            std::span<const std::uint8_t> bytes;
            std::uint8_t u8(std::size_t at) const {
                require_range(bytes.size(), at, 1);
                return bytes[at];
            }
            std::uint16_t u16(std::size_t at) const {
                require_range(bytes.size(), at, 2);
                return (std::uint16_t(bytes[at]) << 8) | bytes[at + 1];
            }
            std::uint32_t u32(std::size_t at) const {
                require_range(bytes.size(), at, 4);
                return (std::uint32_t(bytes[at]) << 24) | (std::uint32_t(bytes[at + 1]) << 16) | (std::uint32_t(bytes[at + 2]) << 8) | bytes[at + 3];
            }
            template <class T>
            T scalar(std::size_t at) const {
                if constexpr (sizeof(T) == 1)
                    return static_cast<T>(u8(at));
                else if constexpr (sizeof(T) == 2)
                    return std::bit_cast<T>(u16(at));
                else
                    return std::bit_cast<T>(u32(at));
            }
            template <class T>
            std::vector<T> values(std::size_t at, std::size_t count) const {
                if (count == 0)
                    return {};
                if (at == 0)
                    throw std::runtime_error("J3D animation nonempty table has a null offset");
                require_range(bytes.size(), at, count * sizeof(T));
                std::vector<T> result;
                result.reserve(count);
                for (std::size_t i = 0; i < count; ++i)
                    result.push_back(scalar<T>(at + i * sizeof(T)));
                return result;
            }
            J3DAnmKeyTableBase key(std::size_t at) const {
                return {u16(at), u16(at + 2), u16(at + 4)};
            }
        };

        // Same source sampler contract as the existing transform decoder: a
        // zero key reads nothing, one key is a scalar, all nonzero types use
        // separate tangents, and equal authored key times remain valid.
        template <class T>
        void validate_key(const J3DAnmKeyTableBase &key, const std::vector<T> &values) {
            if (key.mMaxFrame == 0)
                return;
            if (key.mMaxFrame == 1) {
                require_range(values.size(), key.mOffset, 1);
                return;
            }
            const std::size_t stride = key.mType == 0 ? 3 : 4;
            require_range(values.size(), key.mOffset, key.mMaxFrame * stride);
            float previous = static_cast<float>(values[key.mOffset]);
            if (!std::isfinite(previous))
                throw std::runtime_error("J3D animation key time is not finite");
            for (std::size_t i = 1; i < key.mMaxFrame; ++i) {
                const float time = static_cast<float>(values[key.mOffset + i * stride]);
                if (!std::isfinite(time) || time < previous)
                    throw std::runtime_error("J3D animation key times are not ordered");
                previous = time;
            }
        }
        std::size_t full_extent(std::uint16_t offset, std::uint16_t count) {
            if (count == 0)
                throw std::runtime_error("J3D full animation channel has no readable sample");
            return std::size_t(offset) + count;
        }

        struct NativeBlock {
            virtual ~NativeBlock() = default;
            virtual const JUTDataBlockHeader *header() const = 0;
        };
        template <class Header>
        struct Block final : NativeBlock {
            std::unique_ptr<J3dNativeBlock<Header>> data;
            explicit Block(std::unique_ptr<J3dNativeBlock<Header>> value) : data(std::move(value)) {
            }
            const JUTDataBlockHeader *header() const override {
                return reinterpret_cast<const JUTDataBlockHeader *>(&data->header());
            }
        };
        template <class Header>
        struct Decoder {
            Reader input;
            typename J3dNativeBlock<Header>::Builder builder;
            explicit Decoder(Reader reader) : input(reader) {
                builder.header.mHeader = {input.u32(0), input.u32(4)};
                builder.header.field_0x8 = input.u8(8);
            }
            template <class T>
            void *append(std::span<const T> values) {
                if (values.empty())
                    return nullptr;
                return builder.pointer_offset(builder.template append<T>(values));
            }
            template <class T>
            void *append(const std::vector<T> &values) {
                return append<T>(std::span<const T>(values));
            }
            template <class T>
            void *values(std::size_t source, std::size_t count) {
                return append(input.template values<T>(source, count));
            }
            void *names(std::uint32_t offset, std::size_t required) {
                if (offset == 0) {
                    if (required)
                        throw std::runtime_error("J3D animation has no material name table");
                    return nullptr;
                }
                require_range(input.bytes.size(), offset, 4);
                J3dNameData names(input.bytes.subspan(offset));
                if (names.resource()->mEntryNum < required)
                    throw std::runtime_error("J3D animation material names do not cover its update records");
                return builder.pointer_offset(builder.append_bytes(names.bytes(), alignof(ResNTAB)));
            }
            void *vectors(std::uint32_t offset, std::size_t count) {
                if (count == 0)
                    return nullptr;
                if (offset == 0)
                    throw std::runtime_error("J3D animation centers have a null offset");
                require_range(input.bytes.size(), offset, count * 12);
                std::vector<Vec> values;
                values.reserve(count);
                for (std::size_t i = 0; i < count; ++i)
                    values.push_back({input.template scalar<float>(offset + i * 12), input.template scalar<float>(offset + i * 12 + 4),
                                      input.template scalar<float>(offset + i * 12 + 8)});
                return append(values);
            }
            std::unique_ptr<NativeBlock> finish() {
                return std::make_unique<Block<Header>>(std::move(builder).finish());
            }
        };
        template <class Table, class Read>
        std::vector<Table> read_tables(Reader r, std::uint32_t offset, std::size_t count, std::size_t stride, Read read) {
            if (count && offset == 0)
                throw std::runtime_error("J3D animation descriptors have a null offset");
            if (count)
                require_range(r.bytes.size(), offset, count * stride);
            std::vector<Table> tables;
            tables.reserve(count);
            for (std::size_t i = 0; i < count; ++i)
                tables.push_back(read(offset + i * stride));
            return tables;
        }
        auto key_color(Reader r, std::uint32_t offset, std::size_t count) {
            return read_tables<J3DAnmColorKeyTable>(
                r, offset, count, 24, [&](auto at) { return J3DAnmColorKeyTable{r.key(at), r.key(at + 6), r.key(at + 12), r.key(at + 18)}; });
        }
        auto full_color(Reader r, std::uint32_t offset, std::size_t count) {
            return read_tables<J3DAnmColorFullTable>(r, offset, count, 16, [&](auto at) {
                return J3DAnmColorFullTable{r.u16(at), r.u16(at + 2), r.u16(at + 4), r.u16(at + 6),
                                            r.u16(at + 8), r.u16(at + 10), r.u16(at + 12), r.u16(at + 14)};
            });
        }
        std::array<J3DAnmKeyTableBase, 4> channels(const J3DAnmColorKeyTable &t) {
            return {t.mRInfo, t.mGInfo, t.mBInfo, t.mAInfo};
        }
        std::array<std::pair<std::uint16_t, std::uint16_t>, 4> channels(const J3DAnmColorFullTable &t) {
            return {{{t.mROffset, t.mRMaxFrame}, {t.mGOffset, t.mGMaxFrame}, {t.mBOffset, t.mBMaxFrame}, {t.mAOffset, t.mAMaxFrame}}};
        }

        std::unique_ptr<NativeBlock> decode_color(Reader r, bool keyed) {
            require_range(r.bytes.size(), 0, 0x34);
            const auto count = r.u16(0xe);
            if (keyed) {
                Decoder<J3DAnmColorKeyData> d(r);
                auto &h = d.builder.header;
                std::copy_n(r.bytes.begin() + 9, 3, h.field_0x9);
                h.mFrameMax = r.scalar<std::int16_t>(0xc);
                h.mUpdateMaterialNum = count;
                h.field_0x10 = r.u16(0x10);
                h.field_0x12 = r.u16(0x12);
                h.field_0x14 = r.u16(0x14);
                h.field_0x16 = r.u16(0x16);
                auto tables = key_color(r, r.u32(0x18), count);
                std::array<void **, 4> fields{&h.mRValOffset, &h.mGValOffset, &h.mBValOffset, &h.mAValOffset};
                for (std::size_t c = 0; c < 4; ++c) {
                    auto values = r.values<std::int16_t>(r.u32(0x24 + c * 4), r.u16(0x10 + c * 2));
                    for (const auto &t : tables)
                        validate_key(channels(t)[c], values);
                    *fields[c] = d.append(values);
                }
                h.mTableOffset = d.append(tables);
                h.mUpdateMaterialIDOffset = d.values<std::uint16_t>(r.u32(0x1c), count);
                h.mNameTabOffset = d.names(r.u32(0x20), count);
                return d.finish();
            }
            Decoder<J3DAnmColorFullData> d(r);
            auto &h = d.builder.header;
            std::copy_n(r.bytes.begin() + 9, 3, h.field_0x9);
            std::copy_n(r.bytes.begin() + 0x10, 8, h.field_0x10);
            h.mFrameMax = r.scalar<std::int16_t>(0xc);
            h.mUpdateMaterialNum = count;
            auto tables = full_color(r, r.u32(0x18), count);
            std::array<void **, 4> fields{&h.mRValuesOffset, &h.mGValuesOffset, &h.mBValuesOffset, &h.mAValuesOffset};
            for (std::size_t c = 0; c < 4; ++c) {
                std::size_t extent = 0;
                for (const auto &t : tables) {
                    auto [o, n] = channels(t)[c];
                    extent = std::max(extent, full_extent(o, n));
                }
                *fields[c] = d.values<std::uint8_t>(r.u32(0x24 + c * 4), extent);
            }
            h.mTableOffset = d.append(tables);
            h.mUpdateMaterialIDOffset = d.values<std::uint16_t>(r.u32(0x1c), count);
            h.mNameTabOffset = d.names(r.u32(0x20), count);
            return d.finish();
        }

        std::unique_ptr<NativeBlock> decode_pattern(Reader r) {
            require_range(r.bytes.size(), 0, 0x20);
            Decoder<J3DAnmTexPatternFullData> d(r);
            auto &h = d.builder.header;
            h.field_0x9 = r.u8(9);
            h.mFrameMax = r.scalar<std::int16_t>(0xa);
            h.field_0xc = r.u16(0xc);
            h.field_0xe = r.u16(0xe);
            auto tables = read_tables<J3DAnmTexPatternFullTable>(r, r.u32(0x10), h.field_0xc, 8, [&](auto at) {
                return J3DAnmTexPatternFullTable{r.u16(at), r.u16(at + 2), r.u8(at + 4), r.u16(at + 6)};
            });
            std::size_t extent = 0;
            for (const auto &t : tables)
                extent = std::max(extent, full_extent(t.mOffset, t.mMaxFrame));
            h.mTableOffset = d.append(tables);
            h.mValuesOffset = d.values<std::uint16_t>(r.u32(0x14), extent);
            h.mUpdateMaterialIDOffset = d.values<std::uint16_t>(r.u32(0x18), h.field_0xc);
            h.mNameTabOffset = d.names(r.u32(0x1c), h.field_0xc);
            return d.finish();
        }
        std::unique_ptr<NativeBlock> decode_visibility(Reader r) {
            require_range(r.bytes.size(), 0, 0x18);
            Decoder<J3DAnmVisibilityFullData> d(r);
            auto &h = d.builder.header;
            h.field_0x9 = r.u8(9);
            h.mFrameMax = r.scalar<std::int16_t>(0xa);
            h.field_0xc = r.u16(0xc);
            h.field_0xe = r.u16(0xe);
            auto tables = read_tables<J3DAnmVisibilityFullTable>(r, r.u32(0x10), h.field_0xc, 4,
                                                                 [&](auto at) { return J3DAnmVisibilityFullTable{r.u16(at), r.u16(at + 2)}; });
            std::size_t extent = 0;
            for (const auto &t : tables)
                extent = std::max(extent, full_extent(t._2, t._0));
            h.mTableOffset = d.append(tables);
            h.mValuesOffset = d.values<std::uint8_t>(r.u32(0x14), extent);
            return d.finish();
        }
        std::unique_ptr<NativeBlock> decode_cluster(Reader r, bool keyed) {
            require_range(r.bytes.size(), 0, 0x18);
            const auto count = r.u16(0xc);
            if (keyed) {
                Decoder<J3DAnmClusterKeyData> d(r);
                auto &h = d.builder.header;
                h.mFrameMax = r.scalar<std::int16_t>(0xa);
                h.field_0xc = r.scalar<std::int32_t>(0xc);
                auto tables =
                    read_tables<J3DAnmClusterKeyTable>(r, r.u32(0x10), count, 6, [&](auto at) { return J3DAnmClusterKeyTable{r.key(at)}; });
                auto values = r.values<float>(r.u32(0x14), r.u16(0xe));
                for (const auto &t : tables)
                    validate_key(t.mWeightTable, values);
                h.mTableOffset = d.append(tables);
                h.mWeightOffset = d.append(values);
                return d.finish();
            }
            Decoder<J3DAnmClusterFullData> d(r);
            auto &h = d.builder.header;
            h.mFrameMax = r.scalar<std::int16_t>(0xa);
            h.field_0xc = r.scalar<std::int32_t>(0xc);
            auto tables = read_tables<J3DAnmClusterFullTable>(r, r.u32(0x10), count, 4,
                                                              [&](auto at) { return J3DAnmClusterFullTable{r.u16(at), r.u16(at + 2)}; });
            std::size_t extent = 0;
            for (const auto &t : tables)
                extent = std::max(extent, full_extent(t.mOffset, t.mMaxFrame));
            h.mTableOffset = d.append(tables);
            h.mWeightOffset = d.values<float>(r.u32(0x14), extent);
            return d.finish();
        }
        std::unique_ptr<NativeBlock> decode_texture_srt(Reader r) {
            require_range(r.bytes.size(), 0, 0x60);
            Decoder<J3DAnmTextureSRTKeyData> d(r);
            auto &h = d.builder.header;
            h.field_0x9 = r.u8(9);
            h.field_0xa = r.scalar<std::int16_t>(0xa);
            h.field_0xc = r.u16(0xc);
            h.field_0xe = r.u16(0xe);
            h.field_0x10 = r.u16(0x10);
            h.field_0x12 = r.u16(0x12);
            h.field_0x34 = r.u16(0x34);
            h.field_0x36 = r.u16(0x36);
            h.field_0x38 = r.u16(0x38);
            h.field_0x3a = r.u16(0x3a);
            h.field_0x5c = r.scalar<std::int32_t>(0x5c);
            const auto group = [&](bool post) {
                const auto tracks = r.u16(post ? 0x34 : 0xc), count = std::uint16_t(tracks / 3);
                const auto at = post ? 0x3c : 0x14;
                auto tables = read_tables<J3DAnmTransformKeyTable>(
                    r, r.u32(at), tracks, 18, [&](auto p) { return J3DAnmTransformKeyTable{r.key(p), r.key(p + 6), r.key(p + 12)}; });
                auto scales = r.values<float>(r.u32(post ? 0x50 : 0x28), r.u16(post ? 0x36 : 0xe));
                auto rotations = r.values<std::int16_t>(r.u32(post ? 0x54 : 0x2c), r.u16(post ? 0x38 : 0x10));
                auto translations = r.values<float>(r.u32(post ? 0x58 : 0x30), r.u16(post ? 0x3a : 0x12));
                for (std::size_t i = 0; i < count; ++i) {
                    validate_key(tables[i * 3].mScaleInfo, scales);
                    validate_key(tables[i * 3 + 1].mScaleInfo, scales);
                    validate_key(tables[i * 3 + 2].mRotationInfo, rotations);
                    validate_key(tables[i * 3].mTranslateInfo, translations);
                    validate_key(tables[i * 3 + 1].mTranslateInfo, translations);
                }
                void *table = d.append(tables), *scale = d.append(scales), *rot = d.append(rotations), *trans = d.append(translations);
                void *ids = d.values<std::uint16_t>(r.u32(post ? 0x40 : 0x18), count);
                void *tex = d.values<std::uint8_t>(r.u32(post ? 0x48 : 0x20), count);
                void *centers = d.vectors(r.u32(post ? 0x4c : 0x24), count);
                void *names = d.names(r.u32(post ? 0x44 : 0x1c), count);
                if (post) {
                    h.mInfoTable2Offset = table;
                    h.field_0x40 = ids;
                    h.mNameTab2Offset = names;
                    h.field_0x48 = tex;
                    h.field_0x4c = centers;
                    h.field_0x50 = scale;
                    h.field_0x54 = rot;
                    h.field_0x58 = trans;
                } else {
                    h.mTableOffset = table;
                    h.mUpdateMatIDOffset = ids;
                    h.mNameTab1Offset = names;
                    h.mUpdateTexMtxIDOffset = tex;
                    h.unkOffset = centers;
                    h.mScaleValOffset = scale;
                    h.mRotValOffset = rot;
                    h.mTransValOffset = trans;
                }
            };
            group(false);
            group(true);
            return d.finish();
        }
        std::unique_ptr<NativeBlock> decode_tev(Reader r) {
            require_range(r.bytes.size(), 0, 0x58);
            Decoder<J3DAnmTevRegKeyData> d(r);
            auto &h = d.builder.header;
            h.field_0x9 = r.u8(9);
            h.mFrameMax = r.scalar<std::int16_t>(0xa);
            h.mCRegUpdateMaterialNum = r.u16(0xc);
            h.mKRegUpdateMaterialNum = r.u16(0xe);
            h.field_0x10 = r.u16(0x10);
            h.field_0x12 = r.u16(0x12);
            h.field_0x14 = r.u16(0x14);
            h.field_0x16 = r.u16(0x16);
            h.field_0x18 = r.u16(0x18);
            h.field_0x1a = r.u16(0x1a);
            h.field_0x1c = r.u16(0x1c);
            h.field_0x1e = r.u16(0x1e);
            const auto group = [&]<class Table>(bool konst) {
                const auto count = r.u16(konst ? 0xe : 0xc);
                auto tables = read_tables<Table>(r, r.u32(konst ? 0x24 : 0x20), count, 28, [&](auto at) {
                    Table t{};
                    t.mRTable = r.key(at);
                    t.mGTable = r.key(at + 6);
                    t.mBTable = r.key(at + 12);
                    t.mATable = r.key(at + 18);
                    t.mColorId = r.u8(at + 24);
                    std::copy_n(r.bytes.begin() + at + 25, 3, t.padding);
                    return t;
                });
                std::array<void **, 4> fields =
                    konst ? std::array<void **, 4>{&h.mKRValuesOffset, &h.mKGValuesOffset, &h.mKBValuesOffset, &h.mKAValuesOffset} :
                            std::array<void **, 4>{&h.mCRValuesOffset, &h.mCGValuesOffset, &h.mCBValuesOffset, &h.mCAValuesOffset};
                for (std::size_t c = 0; c < 4; ++c) {
                    auto values = r.values<std::int16_t>(r.u32((konst ? 0x48 : 0x38) + c * 4), r.u16((konst ? 0x18 : 0x10) + c * 2));
                    for (const auto &t : tables)
                        validate_key(std::array{t.mRTable, t.mGTable, t.mBTable, t.mATable}[c], values);
                    *fields[c] = d.append(values);
                }
                void *table = d.append(tables);
                void *ids = d.values<std::uint16_t>(r.u32(konst ? 0x2c : 0x28), count);
                void *names = d.names(r.u32(konst ? 0x34 : 0x30), count);
                if (konst) {
                    h.mKRegTableOffset = table;
                    h.mKRegUpdateMaterialIDOffset = ids;
                    h.mKRegNameTabOffset = names;
                } else {
                    h.mCRegTableOffset = table;
                    h.mCRegUpdateMaterialIDOffset = ids;
                    h.mCRegNameTabOffset = names;
                }
            };
            group.template operator()<J3DAnmCRegKeyTable>(false);
            group.template operator()<J3DAnmKRegKeyTable>(true);
            return d.finish();
        }

        template <bool Keyed>
        std::unique_ptr<NativeBlock> decode_vertex(Reader r) {
            using Header = std::conditional_t<Keyed, J3DAnmVtxColorKeyData, J3DAnmVtxColorFullData>;
            using Value = std::conditional_t<Keyed, std::int16_t, std::uint8_t>;
            require_range(r.bytes.size(), 0, 0x40);
            Decoder<Header> d(r);
            auto &h = d.builder.header;
            h.field_0x9 = r.u8(9);
            h.mFrameMax = r.scalar<std::int16_t>(0xa);
            h.mAnmTableNum[0] = r.u16(0xc);
            h.mAnmTableNum[1] = r.u16(0xe);
            std::copy_n(r.bytes.begin() + 0x10, 8, h.field_0x10);
            const auto tables = std::array{[&] {
                                               if constexpr (Keyed)
                                                   return key_color(r, r.u32(0x18), h.mAnmTableNum[0]);
                                               else
                                                   return full_color(r, r.u32(0x18), h.mAnmTableNum[0]);
                                           }(),
                                           [&] {
                                               if constexpr (Keyed)
                                                   return key_color(r, r.u32(0x1c), h.mAnmTableNum[1]);
                                               else
                                                   return full_color(r, r.u32(0x1c), h.mAnmTableNum[1]);
                                           }()};
            std::array<void **, 4> value_fields;
            if constexpr (Keyed)
                value_fields = {&h.mRValOffset, &h.mGValOffset, &h.mBValOffset, &h.mAValOffset};
            else
                value_fields = {&h.mRValuesOffset, &h.mGValuesOffset, &h.mBValuesOffset, &h.mAValuesOffset};
            for (std::size_t c = 0; c < 4; ++c) {
                std::size_t count = 0;
                if constexpr (Keyed)
                    count = r.u16(0x10 + c * 2);
                else
                    for (const auto &group : tables)
                        for (const auto &t : group) {
                            auto [o, n] = channels(t)[c];
                            count = std::max(count, full_extent(o, n));
                        }
                auto values = r.values<Value>(r.u32(0x30 + c * 4), count);
                if constexpr (Keyed)
                    for (const auto &group : tables)
                        for (const auto &t : group)
                            validate_key(channels(t)[c], values);
                *value_fields[c] = d.append(values);
            }
            for (std::size_t group = 0; group < 2; ++group) {
                h.mTableOffsets[group] = d.append(tables[group]);
                auto indices = read_tables<J3DAnmVtxColorIndexData>(r, r.u32(0x20 + group * 4), h.mAnmTableNum[group], 8, [&](auto at) {
                    return J3DAnmVtxColorIndexData{r.u16(at), reinterpret_cast<void *>(std::uintptr_t(r.u32(at + 4)))};
                });
                std::size_t extent = 0;
                for (const auto &i : indices)
                    extent = std::max(extent, reinterpret_cast<std::uintptr_t>(i.mpData) + i.mNum);
                void *index_data = d.append(indices);
                void *vertices = d.template values<std::uint16_t>(r.u32(0x28 + group * 4), extent);
                if constexpr (Keyed) {
                    h.mVtxColoIndexDataOffset[group] = index_data;
                    h.mVtxColoIndexPointerOffset[group] = vertices;
                } else {
                    h.mVtxColorIndexDataOffsets[group] = index_data;
                    h.mVtxColorIndexPointerOffsets[group] = vertices;
                }
            }
            return d.finish();
        }

        void put32(std::vector<std::uint8_t> &data, std::size_t at, std::uint32_t value) {
            data[at] = value >> 24;
            data[at + 1] = value >> 16;
            data[at + 2] = value >> 8;
            data[at + 3] = value;
        }
        std::unique_ptr<NativeBlock> decode_transform(Reader r, bool keyed) {
            // Reuse the complete existing BCK/BCA decoder and its exact
            // validation rules. This temporary standalone wrapper contains the
            // same block bytes; no second transform interpretation is added.
            std::vector<std::uint8_t> file(0x20 + r.bytes.size());
            put32(file, 0, J3D1);
            put32(file, 4, keyed ? tag('b', 'c', 'k', '1') : tag('b', 'c', 'a', '1'));
            put32(file, 8, static_cast<std::uint32_t>(file.size()));
            put32(file, 12, 1);
            std::copy(r.bytes.begin(), r.bytes.end(), file.begin() + 0x20);
            auto decoded = load_j3d_transform_animation(file);
            const auto count = std::size_t(decoded->field_0x1e) * 3;
            if (keyed) {
                auto &source = static_cast<J3DAnmTransformKey &>(*decoded);
                Decoder<J3DAnmTransformKeyData> d(r);
                auto &h = d.builder.header;
                h.field_0x9 = r.u8(9);
                h.mFrameMax = source.mFrameMax;
                h.field_0xc = source.field_0x1e;
                h.field_0x10 = r.scalar<std::int32_t>(0x10);
                h.mTableOffset = d.template append<J3DAnmTransformKeyTable>({source.mAnmTable, count});
                h.field_0x18 = d.template append<float>({source.mScaleData, r.u16(0xe)});
                h.field_0x1c = d.template append<std::int16_t>({source.mRotData, r.u16(0x10)});
                h.field_0x20 = d.template append<float>({source.mTransData, r.u16(0x12)});
                return d.finish();
            }
            auto &source = static_cast<J3DAnmTransformFull &>(*decoded);
            Decoder<J3DAnmTransformFullData> d(r);
            auto &h = d.builder.header;
            h.field_0x9 = r.u8(9);
            h.mFrameMax = source.mFrameMax;
            h.field_0xc = source.field_0x1e;
            std::copy_n(r.bytes.begin() + 0xe, 6, h.field_0xe);
            std::size_t scale = 0, rotation = 0, translation = 0;
            for (std::size_t i = 0; i < count; ++i) {
                const auto &t = source.mAnmTable[i];
                scale = std::max(scale, full_extent(t.mScaleOffset, t.mScaleMaxFrame));
                rotation = std::max(rotation, full_extent(t.mRotationOffset, t.mRotationMaxFrame));
                translation = std::max(translation, full_extent(t.mTranslateOffset, t.mTranslateMaxFrame));
            }
            h.mTableOffset = d.template append<J3DAnmTransformFullTable>({source.mAnmTable, count});
            h.mScaleValOffset = d.template append<float>({source.mScaleData, scale});
            h.mRotValOffset = d.template append<std::int16_t>({source.mRotData, rotation});
            h.mTransValOffset = d.template append<float>({source.mTransData, translation});
            return d.finish();
        }

        std::uint32_t expected_block(std::uint32_t type) {
            switch (type) {
            case tag('b', 'c', 'k', '1'):
                return ANK1;
            case tag('b', 'c', 'a', '1'):
                return ANF1;
            case tag('b', 'p', 'k', '1'):
                return PAK1;
            case tag('b', 'p', 'a', '1'):
                return PAF1;
            case tag('b', 't', 'k', '1'):
                return TTK1;
            case tag('b', 'r', 'k', '1'):
                return TRK1;
            case tag('b', 't', 'p', '1'):
                return TPT1;
            case tag('b', 'v', 'a', '1'):
                return VAF1;
            case tag('b', 'l', 'k', '1'):
                return CLK1;
            case tag('b', 'l', 'a', '1'):
                return CLF1;
            case tag('b', 'x', 'k', '1'):
                return VCK1;
            case tag('b', 'x', 'a', '1'):
                return VCF1;
            default:
                return 0;
            }
        }
        bool is_animation_block(std::uint32_t type) {
            constexpr std::array known{ANK1, ANF1, PAK1, PAF1, TTK1, TRK1, TPT1, VAF1, CLK1, CLF1, VCK1, VCF1};
            return std::find(known.begin(), known.end(), type) != known.end();
        }
        std::unique_ptr<NativeBlock> decode_block(Reader r) {
            switch (r.u32(0)) {
            case ANK1:
                return decode_transform(r, true);
            case ANF1:
                return decode_transform(r, false);
            case PAK1:
                return decode_color(r, true);
            case PAF1:
                return decode_color(r, false);
            case TTK1:
                return decode_texture_srt(r);
            case TRK1:
                return decode_tev(r);
            case TPT1:
                return decode_pattern(r);
            case VAF1:
                return decode_visibility(r);
            case CLK1:
                return decode_cluster(r, true);
            case CLF1:
                return decode_cluster(r, false);
            case VCK1:
                return decode_vertex<true>(r);
            case VCF1:
                return decode_vertex<false>(r);
            default: {
                typename J3dNativeBlock<JUTDataBlockHeader>::Builder b;
                b.header = {r.u32(0), r.u32(4)};
                return std::make_unique<Block<JUTDataBlockHeader>>(std::move(b).finish());
            }
            }
        }
        struct LoadedData {
            std::shared_ptr<compat::JkrAllocationDomain> domain;
            JUTDataFileHeader file{};
            std::vector<std::unique_ptr<NativeBlock>> blocks;
            std::unique_ptr<J3DAnmBase> animation;
            ~LoadedData() {
                compat::JkrHostAllocationScope host;
                animation.reset();
                blocks.clear();
                domain.reset();
            }
            explicit LoadedData(std::span<const std::uint8_t> bytes) {
                Reader r{bytes};
                file.mMagic = r.u32(0);
                if (file.mMagic != J3D1)
                    return;
                file.mType = r.u32(4);
                const auto expected = expected_block(file.mType);
                if (expected == 0)
                    return;
                require_range(bytes.size(), 0, 0x20);
                file.mFileSize = r.u32(8);
                file.mBlockNum = r.u32(12);
                std::copy_n(bytes.begin() + 0x10, 12, file._10);
                file.mSeAnmOffset = r.u32(0x1c);
                require_range(bytes.size(), 0, file.mFileSize);
                require_range(file.mFileSize, 0, 0x20);
                r.bytes = bytes.first(file.mFileSize);
                std::size_t at = 0x20;
                bool found = false;
                for (std::uint32_t i = 0; i < file.mBlockNum; ++i) {
                    const auto type = r.u32(at), size = r.u32(at + 4);
                    require_range(size, 0, 8);
                    // Retail uses mSize to locate the next block, while every
                    // table offset is relative to this header in the retained
                    // file. The final next pointer is never dereferenced.
                    if (i + 1 < file.mBlockNum)
                        require_range(r.bytes.size(), at, size);
                    if (is_animation_block(type) && type != expected)
                        throw std::runtime_error("J3D animation block does not match its declared animation family");
                    found |= type == expected;
                    blocks.push_back(decode_block({r.bytes.subspan(at)}));
                    at += size;
                }
                if (!found)
                    throw std::runtime_error("J3D animation has no matching data block");
                if (!blocks.empty())
                    file.mFirstBlock = *blocks.front()->header();
            }
        };
        thread_local const LoadedData *current_load = nullptr;
        class LoadScope {
            const LoadedData *previous;

        public:
            explicit LoadScope(const LoadedData &data) : previous(current_load) {
                current_load = &data;
            }
            ~LoadScope() {
                current_load = previous;
            }
        };
        struct Registry {
            std::mutex mutex;
            struct Entry {
                std::weak_ptr<void> owner;
                std::uint64_t generation;
                std::size_t references;
            };
            std::map<const void *, Entry> resources;
            std::uint64_t next_generation = 1;
        };
        Registry &registry() {
            static Registry value;
            return value;
        }
    }  // namespace

    struct J3dAnimationResource::Storage {
        std::vector<std::uint8_t> source;
        std::mutex mutex;
        std::uint64_t generation = 0;
        std::vector<std::unique_ptr<LoadedData>> loads;
        explicit Storage(std::span<const std::uint8_t> bytes) : source(bytes.begin(), bytes.end()) {
            if (source.empty())
                throw std::runtime_error("J3D animation source is empty");
        }
        ~Storage() {
            compat::JkrHostAllocationScope host;
            {
                auto &r = registry();
                std::lock_guard lock(r.mutex);
                const auto entry = r.resources.find(source.data());
                if (entry != r.resources.end() && entry->second.generation == generation)
                    r.resources.erase(entry);
            }
            loads.clear();
        }
        J3DAnmBase *load(J3DAnmLoaderDataBaseFlag flag) {
            auto domain = compat::current_jkr_allocation_domain();
            compat::JkrHostAllocationScope host;
            auto data = std::make_unique<LoadedData>(source);
            data->domain = std::move(domain);
            LoadScope scope(*data);
            if (data->domain) {
                compat::JkrAllocationScope original(data->domain);
                data->animation.reset(detail::load_native_animation(&data->file, flag));
            } else {
                data->animation.reset(detail::load_native_animation(&data->file, flag));
            }
            auto *result = data->animation.get();
            if (result) {
                std::lock_guard lock(mutex);
                loads.push_back(std::move(data));
            }
            return result;
        }
    };
    J3dAnimationResource::J3dAnimationResource(std::span<const std::uint8_t> bytes) {
        compat::JkrHostAllocationScope host;
        _storage = std::make_shared<Storage>(bytes);
        auto &r = registry();
        std::lock_guard lock(r.mutex);
        _storage->generation = r.next_generation++;
        if (_storage->generation == 0)
            throw std::overflow_error("J3D animation registration identity exhausted");
        r.resources.emplace(_storage->source.data(), Registry::Entry{_storage, _storage->generation, 1});
    }
    struct J3dAnimationSourceRegistration::State {
        std::shared_ptr<void> owner;
        const void *identity;
        std::uint64_t generation;
        State(std::shared_ptr<void> resource, const void *key, std::uint64_t id) : owner(std::move(resource)), identity(key), generation(id) {
        }
        ~State() {
            compat::JkrHostAllocationScope host;
            auto &r = registry();
            std::lock_guard lock(r.mutex);
            const auto entry = r.resources.find(identity);
            if (entry != r.resources.end() && entry->second.generation == generation && --entry->second.references == 0)
                r.resources.erase(entry);
        }
    };
    J3dAnimationSourceRegistration::J3dAnimationSourceRegistration(std::unique_ptr<State> state) : _state(std::move(state)) {
    }
    J3dAnimationSourceRegistration::~J3dAnimationSourceRegistration() = default;
    J3dAnimationSourceRegistration::J3dAnimationSourceRegistration(J3dAnimationSourceRegistration &&) noexcept = default;
    J3dAnimationSourceRegistration &J3dAnimationSourceRegistration::operator=(J3dAnimationSourceRegistration &&) noexcept = default;
    J3dAnimationSourceRegistration J3dAnimationResource::register_source(std::span<const std::uint8_t> alias) {
        compat::JkrHostAllocationScope host;
        if (!_storage || alias.size() != _storage->source.size() || !std::equal(alias.begin(), alias.end(), _storage->source.begin()))
            throw std::invalid_argument("J3D animation alias does not match the complete retained source");
        auto &r = registry();
        std::lock_guard lock(r.mutex);
        const auto found = r.resources.find(alias.data());
        std::uint64_t generation;
        if (found != r.resources.end()) {
            if (found->second.owner.lock().get() != _storage.get())
                throw std::logic_error("J3D animation source identity is registered to a different owner");
            generation = found->second.generation;
            ++found->second.references;
        } else {
            generation = r.next_generation++;
            if (generation == 0)
                throw std::overflow_error("J3D animation registration identity exhausted");
            r.resources.emplace(alias.data(), Registry::Entry{_storage, generation, 1});
        }
        try {
            return J3dAnimationSourceRegistration(std::make_unique<J3dAnimationSourceRegistration::State>(_storage, alias.data(), generation));
        } catch (...) {
            auto entry = r.resources.find(alias.data());
            if (--entry->second.references == 0)
                r.resources.erase(entry);
            throw;
        }
    }
    J3dAnimationResource::~J3dAnimationResource() = default;
    const void *J3dAnimationResource::data() const noexcept {
        return _storage ? _storage->source.data() : nullptr;
    }
    std::span<const std::uint8_t> J3dAnimationResource::bytes() const noexcept {
        return _storage ? std::span<const std::uint8_t>(_storage->source) : std::span<const std::uint8_t>{};
    }
    J3DAnmBase *J3dAnimationResource::load(J3DAnmLoaderDataBaseFlag flag) {
        return J3DAnmLoaderDataBase::load(data(), flag);
    }
    J3DAnmBase *load_registered_j3d_animation(const void *data, J3DAnmLoaderDataBaseFlag flag) {
        compat::JkrHostAllocationScope host;
        if (!data)
            return nullptr;
        std::shared_ptr<J3dAnimationResource::Storage> owner;
        {
            auto &r = registry();
            std::lock_guard lock(r.mutex);
            const auto it = r.resources.find(data);
            if (it != r.resources.end())
                owner = std::static_pointer_cast<J3dAnimationResource::Storage>(it->second.owner.lock());
        }
        if (!owner)
            throw std::runtime_error("J3D animation load requires a registered bounded resource owner");
        return owner->load(flag);
    }
    namespace detail {
        const JUTDataBlockHeader *first_animation_block(const void *file) {
            if (!current_load || file != &current_load->file)
                throw std::logic_error("J3D animation block traversal has no native data scope");
            return current_load->blocks.empty() ? nullptr : current_load->blocks.front()->header();
        }
        const JUTDataBlockHeader *next_animation_block(const void *file, const JUTDataBlockHeader *block) {
            if (!current_load || file != &current_load->file)
                throw std::logic_error("J3D animation block traversal has no native data scope");
            for (std::size_t i = 0; i < current_load->blocks.size(); ++i)
                if (current_load->blocks[i]->header() == block)
                    return i + 1 < current_load->blocks.size() ? current_load->blocks[i + 1]->header() : nullptr;
            throw std::logic_error("J3D animation block is not retained by its data scope");
        }
    }  // namespace detail
}  // namespace smgpc::resource
