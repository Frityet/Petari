#include "J3dTransformAnimation.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace smgpc::resource {
    namespace {

        constexpr std::uint32_t J3D1 = 0x4a334431U;
        constexpr std::uint32_t BCK1 = 0x62636b31U;
        constexpr std::uint32_t BCA1 = 0x62636131U;
        constexpr std::uint32_t ANK1 = 0x414e4b31U;
        constexpr std::uint32_t ANF1 = 0x414e4631U;
        constexpr std::size_t TRANSFORM_HEADER_SIZE = 0x24U;

        void require_range(std::size_t size, std::size_t offset, std::size_t count) {
            if (offset > size || count > size - offset) {
                throw std::runtime_error("J3D transform animation range outside its containing block");
            }
        }

        std::uint16_t read_u16(std::span<const std::uint8_t> data, std::size_t offset) {
            require_range(data.size(), offset, 2U);
            return static_cast<std::uint16_t>((std::uint16_t{data[offset]} << 8U) | data[offset + 1U]);
        }

        std::uint32_t read_u32(std::span<const std::uint8_t> data, std::size_t offset) {
            require_range(data.size(), offset, 4U);
            return (std::uint32_t{data[offset]} << 24U) | (std::uint32_t{data[offset + 1U]} << 16U) |
                   (std::uint32_t{data[offset + 2U]} << 8U) | data[offset + 3U];
        }

        template<class T>
        std::vector<T> read_values(std::span<const std::uint8_t> block, std::size_t offset,
                                   std::size_t count) {
            // A zero-length array is never dereferenced by a valid track. Its
            // stored offset is consequently irrelevant, as on the original.
            if (count == 0U) {
                return {};
            }
            require_range(block.size(), offset, count * sizeof(T));
            std::vector<T> values;
            values.reserve(count);
            for (std::size_t i = 0; i < count; ++i) {
                if constexpr (sizeof(T) == 4U) {
                    values.push_back(std::bit_cast<T>(read_u32(block, offset + i * sizeof(T))));
                } else {
                    values.push_back(std::bit_cast<T>(read_u16(block, offset + i * sizeof(T))));
                }
            }
            return values;
        }

        template<class Animation, class Table>
        class OwnedTransform final : public Animation {
        public:
            std::vector<Table> tables;
            std::vector<float> scales;
            std::vector<std::int16_t> rotations;
            std::vector<float> translations;

            void load_metadata(std::span<const std::uint8_t> block) {
                this->mAttribute = block[8U];
                this->mFrameMax = std::bit_cast<std::int16_t>(read_u16(block, 0x0aU));
                this->mFrame = 0.0F;
                this->field_0x1e = read_u16(block, 0x0cU);
            }

            void load_values(std::span<const std::uint8_t> block, std::size_t scale_count,
                              std::size_t rotation_count, std::size_t translation_count) {
                scales = read_values<float>(block, read_u32(block, 0x18U), scale_count);
                rotations = read_values<std::int16_t>(block, read_u32(block, 0x1cU), rotation_count);
                translations = read_values<float>(block, read_u32(block, 0x20U), translation_count);
                this->mScaleData = scales.data();
                this->mRotData = rotations.data();
                this->mTransData = translations.data();
            }
        };

        J3DAnmKeyTableBase read_key(std::span<const std::uint8_t> block, std::size_t offset) {
            return {read_u16(block, offset), read_u16(block, offset + 2U), read_u16(block, offset + 4U)};
        }

        template<class T>
        void validate_key(const J3DAnmKeyTableBase& key, const std::vector<T>& values) {
            if (key.mMaxFrame == 0U) {
                return;
            }
            if (key.mMaxFrame == 1U) {
                require_range(values.size(), key.mOffset, 1U);
                return;
            }
            // J3D uses a shared tangent for type zero and separate incoming /
            // outgoing tangents for every nonzero type.
            const std::size_t stride = key.mType == 0U ? 3U : 4U;
            require_range(values.size(), key.mOffset, key.mMaxFrame * stride);
            float previous = static_cast<float>(values[key.mOffset]);
            if (!std::isfinite(previous)) {
                throw std::runtime_error("J3D transform key time is not finite");
            }
            for (std::size_t i = 1U; i < key.mMaxFrame; ++i) {
                const float time = static_cast<float>(values[key.mOffset + i * stride]);
                // Equal times are valid: the original upper-bound key search
                // selects the last equal interior key before interpolation.
                if (!std::isfinite(time) || time < previous) {
                    throw std::runtime_error("J3D transform key times are not ordered");
                }
                previous = time;
            }
        }

        std::unique_ptr<J3DAnmTransform> load_key(std::span<const std::uint8_t> block) {
            require_range(block.size(), 0U, TRANSFORM_HEADER_SIZE);
            auto animation = std::make_unique<OwnedTransform<J3DAnmTransformKey, J3DAnmTransformKeyTable>>();
            animation->load_metadata(block);
            animation->load_values(block, read_u16(block, 0x0eU), read_u16(block, 0x10U), read_u16(block, 0x12U));
            animation->mDecShift = block[9U];
            const auto table_offset = read_u32(block, 0x14U);
            const auto table_count = std::size_t{animation->field_0x1e} * 3U;
            if (table_count != 0U) {
                require_range(block.size(), table_offset, table_count * 0x12U);
            }
            animation->tables.reserve(table_count);
            for (std::size_t i = 0; i < table_count; ++i) {
                const auto offset = table_offset + i * 0x12U;
                J3DAnmTransformKeyTable table{read_key(block, offset), read_key(block, offset + 6U),
                                             read_key(block, offset + 12U)};
                validate_key(table.mScaleInfo, animation->scales);
                validate_key(table.mRotationInfo, animation->rotations);
                validate_key(table.mTranslateInfo, animation->translations);
                animation->tables.push_back(table);
            }
            animation->mAnmTable = animation->tables.data();
            return animation;
        }

        std::size_t full_extent(std::uint16_t offset, std::uint16_t count) {
            // Unlike keyed animations, the original full sampler always reads
            // a value, including on negative frames and past the channel end.
            if (count == 0U) {
                throw std::runtime_error("J3D full transform channel contains no samples");
            }
            return std::size_t{offset} + count;
        }

        template<class Animation>
        std::unique_ptr<J3DAnmTransform> load_full(std::span<const std::uint8_t> block) {
            require_range(block.size(), 0U, TRANSFORM_HEADER_SIZE);
            auto animation = std::make_unique<OwnedTransform<Animation, J3DAnmTransformFullTable>>();
            animation->load_metadata(block);
            const auto table_offset = read_u32(block, 0x14U);
            const auto table_count = std::size_t{animation->field_0x1e} * 3U;
            if (table_count != 0U) {
                require_range(block.size(), table_offset, table_count * 0x0cU);
            }
            animation->tables.reserve(table_count);
            std::size_t scale_count = 0U, rotation_count = 0U, translation_count = 0U;
            for (std::size_t i = 0; i < table_count; ++i) {
                const auto offset = table_offset + i * 0x0cU;
                J3DAnmTransformFullTable table{read_u16(block, offset), read_u16(block, offset + 2U),
                                              read_u16(block, offset + 4U), read_u16(block, offset + 6U),
                                              read_u16(block, offset + 8U), read_u16(block, offset + 10U)};
                scale_count = std::max(scale_count, full_extent(table.mScaleOffset, table.mScaleMaxFrame));
                rotation_count = std::max(rotation_count, full_extent(table.mRotationOffset, table.mRotationMaxFrame));
                translation_count = std::max(translation_count, full_extent(table.mTranslateOffset, table.mTranslateMaxFrame));
                animation->tables.push_back(table);
            }
            // The original Full loader ignores header bytes 0x0e..0x13. Copy
            // every value its descriptors can read, bounded by this block,
            // without imposing a contract on those unused metadata fields.
            animation->load_values(block, scale_count, rotation_count, translation_count);
            animation->mAnmTable = animation->tables.data();
            return animation;
        }

    }  // namespace

    std::unique_ptr<J3DAnmTransform> load_j3d_transform_animation(std::span<const std::uint8_t> data,
                                                                bool interpolate_full) {
        require_range(data.size(), 0U, 0x20U);
        if (read_u32(data, 0U) != J3D1) {
            throw std::runtime_error("Not a J3D1 transform animation");
        }
        const auto type = read_u32(data, 4U);
        if (type != BCK1 && type != BCA1) {
            throw std::runtime_error("J3D animation is not BCK or BCA");
        }
        const auto file_size = read_u32(data, 8U);
        require_range(data.size(), 0U, file_size);
        require_range(file_size, 0U, 0x20U);
        data = data.first(file_size);
        const auto block_count = read_u32(data, 0x0cU);
        std::size_t offset = 0x20U;
        std::unique_ptr<J3DAnmTransform> animation;
        for (std::uint32_t i = 0; i < block_count; ++i) {
            require_range(data.size(), offset, 8U);
            const auto tag = read_u32(data, offset);
            const auto size = read_u32(data, offset + 4U);
            require_range(size, 0U, 8U);
            require_range(data.size(), offset, size);
            const auto block = data.subspan(offset, size);
            if (type == BCK1 && tag == ANK1) {
                animation = load_key(block);
            } else if (type == BCA1 && tag == ANF1) {
                animation = interpolate_full ? load_full<J3DAnmTransformFullWithLerp>(block)
                                             : load_full<J3DAnmTransformFull>(block);
            }
            offset += size;
        }
        if (!animation) {
            throw std::runtime_error("J3D transform animation has no matching transform block");
        }
        return animation;
    }

}  // namespace smgpc::resource
