#include "layout/NativeLayoutResource.hpp"
#include "layout/BrlytLayout.hpp"
#include "nw4r/lyt/resources.h"
#include <aurora/allocation.hpp>
#include <aurora/endian.hpp>
#include <aurora/exception.hpp>
#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <unordered_set>

namespace smgpc::layout {
    namespace {
        using Bytes = std::span<const std::uint8_t>;
        using Buffer = std::vector<std::uint8_t>;
        using aurora::endian::read_big;

        void require_range(Bytes bytes, std::size_t offset, std::size_t size) {
            if (offset > bytes.size() || size > bytes.size() - offset)
                aurora::throw_host_exception<std::runtime_error>("NW4R resource record exceeds its containing block");
        }

        template<typename T> void put(Buffer& out, std::size_t offset, T value) {
            require_range(out, offset, sizeof(T));
            std::memcpy(out.data() + offset, &value, sizeof(T));
        }

        template<typename T> void scalar(Buffer& out, Bytes source, std::size_t offset) {
            put(out, offset, read_big<T>(source, offset));
        }

        void words(Buffer& out, Bytes source, std::size_t offset, std::size_t count) {
            require_range(source, offset, count * 4);
            for (std::size_t i = 0; i < count; ++i) scalar<std::uint32_t>(out, source, offset + i * 4);
        }

        void string_at(Bytes source, std::size_t offset) {
            require_range(source, offset, 1);
            if (std::find(source.begin() + offset, source.end(), 0) == source.end())
                aurora::throw_host_exception<std::runtime_error>("NW4R resource string has no terminator");
        }

        void name_list(Buffer& out, Bytes source) {
            const auto count = read_big<std::uint16_t>(source, 8);
            scalar<std::uint16_t>(out, source, 8);
            require_range(source, 12, count * 8U);
            for (std::size_t i = 0; i < count; ++i) {
                const auto offset = 12 + i * 8;
                string_at(source, 12 + read_big<std::uint32_t>(source, offset));
                scalar<std::uint32_t>(out, source, offset);
            }
        }

        void window_content(Buffer& out, Bytes source, std::size_t offset) {
            require_range(source, offset, 20);
            words(out, source, offset, 4);
            scalar<std::uint16_t>(out, source, offset + 16);
            words(out, source, offset + 20, source[offset + 18] * 8U);
        }

        Buffer layout_block(Bytes source, const BrlytLayout& parsed) {
            Buffer out(source.begin(), source.end());
            const auto kind = std::string_view(reinterpret_cast<const char*>(source.data()), 4);
            scalar<std::uint32_t>(out, source, 4);
            if (kind == "lyt1") {
                words(out, source, 12, 2);
            } else if (kind == "txl1" || kind == "fnl1") {
                name_list(out, source);
            } else if (kind == "mat1") {
                const auto count = read_big<std::uint16_t>(source, 8);
                if (count != parsed.materials.size())
                    aurora::throw_host_exception<std::runtime_error>("NW4R layout has conflicting material tables");
                scalar<std::uint16_t>(out, source, 8);
                for (std::size_t i = 0; i < count; ++i) {
                    const auto offset = read_big<std::uint32_t>(source, 12 + i * 4);
                    scalar<std::uint32_t>(out, source, 12 + i * 4);
                    const auto& material = parsed.materials[i].native_resource;
                    require_range(source, offset, material.size());
                    std::memcpy(out.data() + offset, material.data(), material.size());
                }
            } else if (kind == "grp1") {
                const auto count = read_big<std::uint16_t>(source, 24);
                require_range(source, 28, count * 16U);
                scalar<std::uint16_t>(out, source, 24);
            } else if (kind == "pan1" || kind == "bnd1" || kind == "pic1" || kind == "txt1" || kind == "wnd1") {
                // Float words are copied by representation, preserving NaN,
                // signed zero and the exact original local transform values.
                words(out, source, 36, 10);
                if (kind == "pic1") {
                    window_content(out, source, 76);
                } else if (kind == "txt1") {
                    using nw4r::lyt::res::TextBox;
                    require_range(source, 0, 116);
                    const auto buffer_bytes = read_big<std::uint16_t>(source, 76);
                    const auto string_bytes = read_big<std::uint16_t>(source, 78);
                    const auto text_offset = read_big<std::uint32_t>(source, 88);
                    if ((buffer_bytes | string_bytes) & 1U)
                        aurora::throw_host_exception<std::runtime_error>("BRLYT text byte counts must contain complete UTF-16 units");
                    require_range(source, text_offset, string_bytes);
                    TextBox text{};
                    std::memcpy(static_cast<nw4r::lyt::res::Pane*>(&text), out.data(), 76);
                    text.textBufBytes = (buffer_bytes / 2U) * sizeof(wchar_t);
                    text.textStrBytes = (string_bytes / 2U) * sizeof(wchar_t);
                    text.materialIdx = read_big<std::uint16_t>(source, 80);
                    text.fontIdx = read_big<std::uint16_t>(source, 82);
                    text.textPosition = source[84];
                    text.textAlignment = source[85];
                    text.textStrOffset = sizeof(TextBox);
                    text.textCols[0] = read_big<std::uint32_t>(source, 92);
                    text.textCols[1] = read_big<std::uint32_t>(source, 96);
                    text.fontSize.width = read_big<float>(source, 100);
                    text.fontSize.height = read_big<float>(source, 104);
                    text.charSpace = read_big<float>(source, 108);
                    text.lineSpace = read_big<float>(source, 112);
                    if (text.materialIdx >= parsed.materials.size() || text.fontIdx >= parsed.font_names.size())
                        aurora::throw_host_exception<std::runtime_error>("BRLYT text references an absent material or font");
                    out.resize(sizeof(TextBox) + text.textStrBytes);
                    text.blockHeader.size = static_cast<std::uint32_t>(out.size());
                    std::memcpy(out.data(), &text, sizeof(text));
                    for (std::size_t i = 0; i < string_bytes / 2U; ++i)
                        put(out, sizeof(TextBox) + i * sizeof(wchar_t), static_cast<wchar_t>(read_big<std::uint16_t>(source, text_offset + i * 2)));
                } else if (kind == "wnd1") {
                    words(out, source, 76, 4);
                    require_range(source, 92, 12);
                    const auto content = read_big<std::uint32_t>(source, 96);
                    const auto frames = read_big<std::uint32_t>(source, 100);
                    words(out, source, 96, 2);
                    window_content(out, source, content);
                    const auto frame_count = source[92];
                    require_range(source, frames, frame_count * 4U);
                    for (std::size_t i = 0; i < frame_count; ++i) {
                        const auto offset = read_big<std::uint32_t>(source, frames + i * 4);
                        scalar<std::uint32_t>(out, source, frames + i * 4);
                        require_range(source, offset, 4);
                        scalar<std::uint16_t>(out, source, offset);
                    }
                }
            }
            return out;
        }

        Buffer animation_block(Bytes source) {
            Buffer out(source.begin(), source.end());
            const auto kind = std::string_view(reinterpret_cast<const char*>(source.data()), 4);
            scalar<std::uint32_t>(out, source, 4);
            if (kind == "pat1") {
                const auto count = read_big<std::uint16_t>(source, 10);
                string_at(source, read_big<std::uint32_t>(source, 12));
                require_range(source, read_big<std::uint32_t>(source, 16), count * 20U);
                for (auto offset : {8, 10, 20, 22}) scalar<std::uint16_t>(out, source, offset);
                words(out, source, 12, 2);
            } else if (kind == "pah1") {
                require_range(source, read_big<std::uint32_t>(source, 8), read_big<std::uint16_t>(source, 12) * 36U);
                scalar<std::uint32_t>(out, source, 8);
                scalar<std::uint16_t>(out, source, 12);
            } else if (kind == "pai1") {
                require_range(source, 0, 20);
                const auto file_count = read_big<std::uint16_t>(source, 12);
                const auto content_count = read_big<std::uint16_t>(source, 14);
                const auto contents = read_big<std::uint32_t>(source, 16);
                for (auto offset : {8, 12, 14}) scalar<std::uint16_t>(out, source, offset);
                scalar<std::uint32_t>(out, source, 16);
                words(out, source, 20, file_count);
                for (std::size_t i = 0; i < file_count; ++i)
                    string_at(source, 20 + read_big<std::uint32_t>(source, 20 + i * 4));
                words(out, source, contents, content_count);
                // Offset aliases remain aliases. Always read the packed source,
                // never swap a shared target repeatedly in place.
                std::unordered_set<std::size_t> visited_infos;
                std::unordered_set<std::size_t> visited_targets;
                for (std::size_t c = 0; c < content_count; ++c) {
                    const auto content = read_big<std::uint32_t>(source, contents + c * 4);
                    require_range(source, content, 24);
                    const auto info_count = source[content + 20];
                    words(out, source, content + 24, info_count);
                    for (std::size_t i = 0; i < info_count; ++i) {
                        const auto info = content + read_big<std::uint32_t>(source, content + 24 + i * 4);
                        if (!visited_infos.insert(info).second) continue;
                        require_range(source, info, 8);
                        scalar<std::uint32_t>(out, source, info);
                        const auto target_count = source[info + 4];
                        words(out, source, info + 8, target_count);
                        const auto info_kind = read_big<std::uint32_t>(source, info);
                        for (std::size_t t = 0; t < target_count; ++t) {
                            const auto target = info + read_big<std::uint32_t>(source, info + 8 + t * 4);
                            if (!visited_targets.insert(target).second) continue;
                            require_range(source, target, 12);
                            const auto count = read_big<std::uint16_t>(source, target + 4);
                            const auto curve = source[target + 2];
                            const auto keys = target + read_big<std::uint32_t>(source, target + 8);
                            if (count == 0 || (curve != 1 && curve != 2))
                                aurora::throw_host_exception<std::runtime_error>("Unsupported or empty BRLAN key curve");
                            scalar<std::uint16_t>(out, source, target + 4);
                            scalar<std::uint32_t>(out, source, target + 8);
                            const auto stride = curve == 1 ? 8U : 12U;
                            require_range(source, keys, count * stride);
                            for (std::size_t k = 0; k < count; ++k) {
                                const auto key = keys + k * stride;
                                if (curve == 2) {
                                    words(out, source, key, 3);
                                } else {
                                    scalar<float>(out, source, key);
                                    scalar<std::uint16_t>(out, source, key + 4);
                                    scalar<std::uint16_t>(out, source, key + 6);
                                    if (info_kind == nw4r::lyt::res::ANIMATIONTYPE_RLTP && read_big<std::uint16_t>(source, key + 4) >= file_count)
                                        aurora::throw_host_exception<std::runtime_error>("BRLAN texture key references an absent image");
                                }
                            }
                        }
                    }
                }
            }
            return out;
        }
    }

    void NativeLayoutResource::validate_archive_span(Bytes bytes) {
        require_range(bytes, 0, 16);
        const auto size = read_big<std::uint32_t>(bytes, 8);
        if (size < 16 || size > bytes.size())
            aurora::throw_host_exception<std::runtime_error>("NW4R resource file size exceeds its archive entry");
    }

    std::shared_ptr<const NativeLayoutResource> NativeLayoutResource::create(const void* resource, std::uint32_t signature) {
        if (resource == nullptr) return {};
        const Bytes header(static_cast<const std::uint8_t*>(resource), 16);
        if (read_big<std::uint32_t>(header, 0) != signature) return {};
        const auto packed = read_big<std::uint16_t>(header, 4) == 0xFEFF;
        const auto* native = static_cast<const nw4r::lyt::res::BinaryFileHeader*>(resource);
        if (!packed && native->byteOrder != 0xFEFF) return {};
        const auto version = packed ? read_big<std::uint16_t>(header, 6) : native->version;
        if (version >> 8 || (version & 255) < 9) return {};
        const auto size = packed ? read_big<std::uint32_t>(header, 8) : native->fileSize;
        if (size < 16) aurora::throw_host_exception<std::runtime_error>("NW4R file header is truncated");
        const aurora::allocation::HostAllocationScope host;
        return std::make_shared<NativeLayoutResource>(Bytes(header.data(), size), resource, packed);
    }

    NativeLayoutResource::NativeLayoutResource(Bytes bytes, const void* source, bool packed) : _source(source) {
        if (!packed) {
            _bytes.assign(bytes.begin(), bytes.end());
            return;
        }
        const auto header_size = read_big<std::uint16_t>(bytes, 12);
        const auto block_count = read_big<std::uint16_t>(bytes, 14);
        if (header_size < 16 || header_size % 4)
            aurora::throw_host_exception<std::runtime_error>("NW4R file header size is invalid");
        require_range(bytes, 0, header_size);
        const bool layout = read_big<std::uint32_t>(bytes, 0) == nw4r::lyt::res::FILESIGNATURE_RLYT;
        const auto parsed = layout ? parse_brlyt_layout(bytes) : BrlytLayout{};
        _bytes.assign(bytes.begin(), bytes.begin() + header_size);
        for (auto offset : {4, 6, 12, 14}) scalar<std::uint16_t>(_bytes, bytes, offset);
        std::size_t offset = header_size;
        for (std::size_t i = 0; i < block_count; ++i) {
            require_range(bytes, offset, 8);
            const auto size = read_big<std::uint32_t>(bytes, offset + 4);
            if (size < 8 || size % 4)
                aurora::throw_host_exception<std::runtime_error>("NW4R block size is invalid");
            require_range(bytes, offset, size);
            const auto block = bytes.subspan(offset, size);
            auto converted = layout ? layout_block(block, parsed) : animation_block(block);
            _bytes.insert(_bytes.end(), converted.begin(), converted.end());
            offset += size;
        }
        if (_bytes.size() > std::numeric_limits<std::uint32_t>::max())
            aurora::throw_host_exception<std::runtime_error>("Native NW4R resource exceeds its offset representation");
        put(_bytes, 8, static_cast<std::uint32_t>(_bytes.size()));
    }
}
