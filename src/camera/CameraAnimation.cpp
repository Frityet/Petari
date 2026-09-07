#include "camera/CameraAnimation.hpp"

#include "Game/Camera/CameraAnim.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <new>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace smgpc::camera {
    struct NativeCameraAnimationData::Storage {
        explicit Storage(std::size_t byte_count)
            : data(::operator new(byte_count,
                                  std::align_val_t{alignof(std::max_align_t)})),
              size(byte_count) {}

        ~Storage() {
            ::operator delete(data,
                              std::align_val_t{alignof(std::max_align_t)});
        }

        Storage(const Storage &) = delete;
        Storage &operator=(const Storage &) = delete;

        void *data;
        std::size_t size;
    };

    namespace {

        static_assert(sizeof(CanmFileHeader) == 0x20U);
        static_assert(sizeof(CanmFrameInfo) == 8U * 8U);
        static_assert(sizeof(CanmKeyFrameInfo) == 8U * 12U);
        static_assert(alignof(CanmFileHeader) == alignof(std::uint32_t));
        static_assert(alignof(CanmFrameInfo) == alignof(std::uint32_t));
        static_assert(alignof(CanmKeyFrameInfo) == alignof(std::uint32_t));
        static_assert(sizeof(float) == 4U);
        static_assert(alignof(float) == alignof(std::uint32_t));

        [[nodiscard]] std::uint32_t read_be_u32(
            std::span<const std::uint8_t> bytes, std::size_t offset) {
            if (offset + 4U > bytes.size()) {
                throw std::runtime_error(
                    "Camera animation field is outside the CANM resource.");
            }
            return (static_cast<std::uint32_t>(bytes[offset]) << 24U) |
                   (static_cast<std::uint32_t>(bytes[offset + 1U]) << 16U) |
                   (static_cast<std::uint32_t>(bytes[offset + 2U]) << 8U) |
                   static_cast<std::uint32_t>(bytes[offset + 3U]);
        }

        [[nodiscard]] float read_be_float(
            std::span<const std::uint8_t> bytes, std::size_t offset) {
            return std::bit_cast<float>(read_be_u32(bytes, offset));
        }

        [[nodiscard]] bool has_tag(std::span<const std::uint8_t> bytes,
                                   std::size_t offset,
                                   std::string_view tag) {
            return offset + tag.size() <= bytes.size() &&
                   std::equal(tag.begin(), tag.end(), bytes.begin() + offset);
        }

    }  // namespace

    std::span<const std::uint8_t> NativeCameraAnimationData::bytes() const noexcept {
        if (_storage == nullptr) {
            return {};
        }
        return {static_cast<const std::uint8_t *>(_storage->data), _storage->size};
    }

    CameraAnimation CameraAnimation::from_bytes(
        std::span<const std::uint8_t> bytes) {
        constexpr auto cHeaderSize = std::size_t{0x20U};
        if (bytes.size() < cHeaderSize || !has_tag(bytes, 0U, "ANDO")) {
            throw std::runtime_error(
                "Camera animation resource does not have an ANDO header.");
        }

        auto result = CameraAnimation{};
        struct Component {
            std::uint32_t count = 0U;
            std::uint32_t offset = 0U;
            std::uint32_t type = 0U;
        };
        auto components = std::array<Component, 8U>{};
        auto values = std::vector<float>{};
        auto component_size = std::size_t{};
        if (has_tag(bytes, 4U, "CANM")) {
            result._format = CameraAnimationFormat::Canm;
            component_size = 8U;
        } else if (has_tag(bytes, 4U, "CKAN")) {
            result._format = CameraAnimationFormat::Ckan;
            component_size = 12U;
        } else {
            throw std::runtime_error(
                "Camera animation resource is neither CANM nor CKAN.");
        }
        if (read_be_u32(bytes, 0x08U) == 0U) {
            throw std::runtime_error(
                "Camera animation resource has an unsupported zero version.");
        }

        result._frame_count = read_be_u32(bytes, 0x18U);
        if (result._frame_count == 0U) {
            throw std::runtime_error(
                "Camera animation has no frames for the original camera reader.");
        }
        const auto value_offset =
            cHeaderSize + static_cast<std::size_t>(read_be_u32(bytes, 0x1cU));
        const auto component_table_size =
            components.size() * component_size;
        if ((value_offset % alignof(std::uint32_t)) != 0U ||
            value_offset < cHeaderSize + component_table_size) {
            throw std::runtime_error(
                "Camera animation value table is unaligned or overlaps its components.");
        }
        if (cHeaderSize + component_table_size > bytes.size() ||
            value_offset + 4U > bytes.size()) {
            throw std::runtime_error(
                "Camera animation component or value table is truncated.");
        }

        const auto value_byte_count =
            static_cast<std::size_t>(read_be_u32(bytes, value_offset));
        if ((value_byte_count % sizeof(float)) != 0U ||
            value_offset + 4U + value_byte_count > bytes.size()) {
            throw std::runtime_error(
                "Camera animation float table has an invalid extent.");
        }

        for (auto index = std::size_t{}; index < components.size();
             ++index) {
            const auto offset = cHeaderSize + index * component_size;
            auto &component = components[index];
            component.count = read_be_u32(bytes, offset);
            component.offset = read_be_u32(bytes, offset + 4U);
            if (result._format == CameraAnimationFormat::Ckan) {
                component.type = read_be_u32(bytes, offset + 8U);
            }
        }

        values.reserve(value_byte_count / sizeof(float));
        for (auto offset = value_offset + 4U;
             offset < value_offset + 4U + value_byte_count; offset += 4U) {
            const auto value = read_be_float(bytes, offset);
            if (!std::isfinite(value)) {
                throw std::runtime_error(
                    "Camera animation contains a non-finite authored value.");
            }
            values.push_back(value);
        }

        const auto terminal_frame = static_cast<float>(result._frame_count - 1U);
        if (result._format == CameraAnimationFormat::Canm &&
            !(terminal_frame < 4294967296.0F)) {
            throw std::runtime_error(
                "CANM terminal frame exceeds the original unsigned frame conversion.");
        }
        for (auto component_index = std::size_t{};
             component_index < components.size(); ++component_index) {
            const auto &component = components[component_index];
            if (component.count == 0U) {
                throw std::runtime_error(
                    "Camera animation component has no authored samples.");
            }
            const auto stride = result._format == CameraAnimationFormat::Canm ?
                                    std::size_t{1U} :
                                    component.type == 0U ? std::size_t{3U} :
                                                           std::size_t{4U};
            // A final key used only as lookahead needs time/value/incoming
            // tangent; get4f never reads its outgoing tangent.
            const auto required = component.count == 1U ? std::size_t{1U} :
                                  result._format == CameraAnimationFormat::Canm ?
                                      static_cast<std::size_t>(component.count) :
                                      (component.count - 1U) * stride + 3U;
            if (static_cast<std::size_t>(component.offset) + required >
                values.size()) {
                throw std::runtime_error(
                    "Camera animation component exceeds its float table.");
            }

            if (result._format == CameraAnimationFormat::Ckan &&
                component.count > 1U) {
                auto previous = values[component.offset];
                if (previous > 0.0F) {
                    throw std::runtime_error(
                        "CKAN first key would underflow the original key search at frame zero.");
                }
                for (auto key = std::uint32_t{1U}; key < component.count; ++key) {
                    const auto next = values[
                        static_cast<std::size_t>(component.offset) + key * stride];
                    if (previous > next) {
                        throw std::runtime_error(
                            "CKAN key times are not in nondecreasing order.");
                    }
                    previous = next;
                }
                // Original get3f/get4f unconditionally read the following key.
                // calc samples active frames below mNrFrames and, once ended,
                // samples twist/FOV at float(mNrFrames - 1).
                const auto final_key_is_reachable =
                    previous < static_cast<float>(result._frame_count) ||
                    (component_index >= 6U && !(terminal_frame < previous));
                if (final_key_is_reachable) {
                    // The search count need not include a stored lookahead
                    // record. Preserve the original count and validate the
                    // three following values that get3f/get4f actually load.
                    const auto following =
                        static_cast<std::size_t>(component.offset) +
                        component.count * stride;
                    if (following + 3U > values.size()) {
                        throw std::runtime_error(
                            "CKAN final searched key has no readable following record.");
                    }
                    const auto boundary = values[following];
                    // This record is outside the binary search. Later frames
                    // retain this final segment and extrapolate in the original
                    // Hermite routine; its time does not clamp playback.
                    if (!(previous < boundary)) {
                        throw std::runtime_error(
                            "CKAN following record does not advance beyond the last searched key.");
                    }
                }
            }
        }

        auto storage = std::make_shared<NativeCameraAnimationData::Storage>(bytes.size());
        auto *native = static_cast<std::uint8_t *>(storage->data);
        std::memcpy(native, bytes.data(), bytes.size());

        auto *header = std::construct_at(reinterpret_cast<CanmFileHeader *>(native));
        std::memcpy(header->mMagic, bytes.data(), sizeof(header->mMagic));
        std::memcpy(header->mType, bytes.data() + 4U, sizeof(header->mType));
        header->_8 = std::bit_cast<std::int32_t>(read_be_u32(bytes, 0x08U));
        header->_C = std::bit_cast<std::int32_t>(read_be_u32(bytes, 0x0cU));
        header->_10 = std::bit_cast<std::int32_t>(read_be_u32(bytes, 0x10U));
        header->_14 = std::bit_cast<std::int32_t>(read_be_u32(bytes, 0x14U));
        header->mNrFrames = result._frame_count;
        header->mValueOffset = read_be_u32(bytes, 0x1cU);

        if (result._format == CameraAnimationFormat::Canm) {
            auto *info = std::construct_at(
                reinterpret_cast<CanmFrameInfo *>(native + cHeaderSize));
            const auto native_components = std::array{
                &info->mPosX, &info->mPosY, &info->mPosZ,
                &info->mWatchPosX, &info->mWatchPosY, &info->mWatchPosZ,
                &info->mTwist, &info->mFovy};
            for (auto index = std::size_t{}; index < native_components.size(); ++index) {
                native_components[index]->mCount = components[index].count;
                native_components[index]->mOffset = components[index].offset;
            }
        } else {
            auto *info = std::construct_at(
                reinterpret_cast<CanmKeyFrameInfo *>(native + cHeaderSize));
            const auto native_components = std::array{
                &info->mPosX, &info->mPosY, &info->mPosZ,
                &info->mWatchPosX, &info->mWatchPosY, &info->mWatchPosZ,
                &info->mTwist, &info->mFovy};
            for (auto index = std::size_t{}; index < native_components.size(); ++index) {
                native_components[index]->mCount = components[index].count;
                native_components[index]->mOffset = components[index].offset;
                native_components[index]->mType = components[index].type;
            }
        }

        std::construct_at(reinterpret_cast<std::uint32_t *>(native + value_offset),
                          static_cast<std::uint32_t>(value_byte_count));
        auto *native_values = ::new (static_cast<void *>(native + value_offset + 4U))
            float[values.size()];
        std::copy(values.begin(), values.end(), native_values);
        result._native_data._storage = std::move(storage);
        return result;
    }

    CameraAnimationFormat CameraAnimation::format() const noexcept {
        return _format;
    }

    std::uint32_t CameraAnimation::frame_count() const noexcept {
        return _frame_count;
    }

    NativeCameraAnimationData CameraAnimation::native_data() const noexcept {
        return _native_data;
    }

    CameraAnimationSample CameraAnimation::sample(float frame) const {
        if (!std::isfinite(frame) || frame < 0.0F ||
            !(frame < static_cast<float>(_frame_count))) {
            throw std::invalid_argument(
                "Camera animation sampling requires a finite frame within playback.");
        }
        const auto native = _native_data.bytes();
        if (native.empty()) {
            throw std::logic_error("Camera animation has no native data owner.");
        }
        const auto *header = reinterpret_cast<const CanmFileHeader *>(native.data());
        // The original set/accessor signatures predate const-correctness. They
        // retain these pointers and only read the immutable native block.
        auto *entry = const_cast<std::uint8_t *>(native.data() + sizeof(CanmFileHeader));
        const auto read_sample = [&](auto &accessor) {
            accessor.set(entry, entry + header->mValueOffset + 4U);
            auto eye = TVec3f{};
            auto watch = TVec3f{};
            accessor.getPos(&eye, frame);
            accessor.getWatchPos(&watch, frame);
            return CameraAnimationSample{
                .eye = {eye.x, eye.y, eye.z},
                .watch = {watch.x, watch.y, watch.z},
                .twist_degrees = accessor.getTwist(frame),
                .fovy_degrees = accessor.getFovy(frame),
            };
        };
        if (_format == CameraAnimationFormat::Canm) {
            auto accessor = CamAnmDataAccessor{};
            return read_sample(accessor);
        }
        auto accessor = KeyCamAnmDataAccessor{};
        return read_sample(accessor);
    }

}  // namespace smgpc::camera
