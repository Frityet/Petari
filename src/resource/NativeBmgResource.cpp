#include "resource/NativeBmgResource.hpp"
#include "resource/BmgMessageArchive.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include <aurora/exception.hpp>
#include <aurora/endian.hpp>

#include <cstring>
#include <array>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace smgpc::resource {
namespace {
    using aurora::endian::read_big;
    using Bytes = std::span<const std::uint8_t>;
    void require_range(Bytes bytes, std::size_t offset, std::size_t count) {
        if (offset > bytes.size() || count > bytes.size() - offset)
            aurora::throw_host_exception<std::invalid_argument>("Native BMG field extends outside its retained block");
    }
    template<class T> void store(std::vector<std::uint8_t>& bytes, std::size_t offset, T value) {
        if (offset > bytes.size() || sizeof(T) > bytes.size() - offset)
            aurora::throw_host_exception<std::out_of_range>("Native BMG output field extends outside its allocation");
        std::memcpy(bytes.data() + offset, &value, sizeof(value));
    }
    std::uint32_t size32(std::size_t size) {
        if (size > std::numeric_limits<std::uint32_t>::max())
            aurora::throw_host_exception<std::length_error>("Native BMG exceeds its original 32-bit offset range");
        return static_cast<std::uint32_t>(size);
    }
    std::size_t align4(std::size_t size) { return (size + 3U) & ~std::size_t{3}; }
}

struct NativeBmgResource::Storage {
    std::vector<std::uint8_t> bytes;
    std::vector<std::uint16_t> text_utf16;
    std::size_t text_begin = 0;
    std::vector<std::uint32_t> text_offsets;

    Storage(Bytes bmg, Bytes ids) {
        const auto parsed = BmgMessageArchive::from_bytes(bmg, ids);
        static_assert(sizeof(wchar_t) == 2 || sizeof(wchar_t) == 4);
        constexpr std::array typed_blocks{"INF1", "DAT1", "FLW1", "FLI1"};
        std::array<bool, typed_blocks.size()> seen{};
        for (const auto& block : parsed.blocks()) {
            for (std::size_t i = 0; i < typed_blocks.size(); ++i) {
                if (block.magic != typed_blocks[i]) continue;
                if (seen[i])
                    aurora::throw_host_exception<std::invalid_argument>("BMG contains a duplicate typed block identity");
                seen[i] = true;
            }
        }
        bytes.assign(bmg.begin(), bmg.begin() + 0x20);
        text_offsets.reserve(parsed.message_count());
        for (const auto& message : parsed.messages()) {
            if (message.info.text_offset % 2 != 0)
                aurora::throw_host_exception<std::invalid_argument>("BMG UTF-16 text offset is not code-unit aligned");
            // Validate tag structure including odd lengths before original readers run.
            (void)bmg_control_tags(message.raw_text);
            text_offsets.push_back(size32(std::size_t(message.info.text_offset / 2) * sizeof(wchar_t)));
        }
        store(bytes, 0x0c, size32(parsed.blocks().size()));
        for (const auto& block : parsed.blocks()) {
            const auto source = bmg.subspan(block.offset, block.available_size);
            const auto start = bytes.size();
            auto output_size = source.size();
            if (block.magic == "DAT1") {
                if ((source.size() - 8U) % 2U != 0)
                    aurora::throw_host_exception<std::invalid_argument>("BMG DAT1 has a partial UTF-16 code unit");
                output_size = 8U + (source.size() - 8U) / 2U * sizeof(wchar_t);
            }
            output_size = align4(output_size);
            (void)size32(start + output_size);
            bytes.resize(start + output_size);
            if (block.magic != "DAT1")
                std::memcpy(bytes.data() + start, source.data(), source.size());
            store(bytes, start, read_big<std::uint32_t>(source, 0));
            store(bytes, start + 4, size32(output_size));

            if (block.magic == "INF1") {
                const auto count = read_big<std::uint16_t>(source, 8);
                const auto item_size = read_big<std::uint16_t>(source, 10);
                if (item_size < 12 || item_size % 4 != 0 || count != text_offsets.size())
                    aurora::throw_host_exception<std::invalid_argument>("BMG INF1 cannot be addressed by original aligned information records");
                require_range(source, 16, std::size_t(count) * item_size);
                store(bytes, start + 8, count);
                store(bytes, start + 10, item_size);
                store(bytes, start + 12, read_big<std::uint32_t>(source, 12));
                for (std::size_t i = 0; i < count; ++i) {
                    const auto offset = 16U + i * item_size;
                    store(bytes, start + offset, text_offsets[i]);
                    store(bytes, start + offset + 4, read_big<std::uint16_t>(source, offset + 4));
                }
            } else if (block.magic == "DAT1") {
                text_begin = start + 8;
                // Both views retain the complete authored DAT1 payload. Original
                // callers may copy through terminators or share interior offsets.
                text_utf16.resize((source.size() - 8U) / 2U);
                for (std::size_t i = 0; i < text_utf16.size(); ++i) {
                    const auto unit = read_big<std::uint16_t>(source, 8 + i * 2);
                    text_utf16[i] = unit;
                    store(bytes, text_begin + i * sizeof(wchar_t), static_cast<wchar_t>(unit));
                }
            } else if (block.magic == "FLW1") {
                const auto count = read_big<std::uint16_t>(source, 8);
                const auto branches = read_big<std::uint16_t>(source, 10);
                require_range(source, 16, std::size_t(count) * 8U + std::size_t(branches) * 2U);
                store(bytes, start + 8, count);
                store(bytes, start + 10, branches);
                store(bytes, start + 12, read_big<std::uint32_t>(source, 12));
                for (std::size_t i = 0; i < count; ++i) {
                    const auto offset = 16U + i * 8U;
                    store(bytes, start + offset + 2, read_big<std::uint16_t>(source, offset + 2));
                    if (source[offset] == 3) {
                        // Event nodes expose one u32 argument, other nodes two u16 fields.
                        store(bytes, start + offset + 4, read_big<std::uint32_t>(source, offset + 4));
                    } else {
                        store(bytes, start + offset + 4, read_big<std::uint16_t>(source, offset + 4));
                        store(bytes, start + offset + 6, read_big<std::uint16_t>(source, offset + 6));
                    }
                }
                for (std::size_t i = 0; i < branches; ++i) {
                    const auto offset = 16U + std::size_t(count) * 8U + i * 2U;
                    const auto index = read_big<std::uint16_t>(source, offset);
                    if (index != 0xffff && index >= count)
                        aurora::throw_host_exception<std::invalid_argument>("BMG branch points outside its original node array");
                    store(bytes, start + offset, index);
                }
            }
            // FLI1 and unknown payloads are retained literally. Original MessageData
            // exposes their block pointer but does not interpret their payload fields.
        }
        store(bytes, 8, size32(bytes.size()));
    }
};

NativeBmgResource::NativeBmgResource(Bytes bmg, Bytes ids) {
    compat::JkrHostAllocationScope host;
    _storage = std::make_unique<Storage>(bmg, ids);
}
NativeBmgResource::~NativeBmgResource() {
    compat::JkrHostAllocationScope host;
    _storage.reset();
}
std::uint8_t* NativeBmgResource::data() const noexcept { return _storage->bytes.data(); }
std::size_t NativeBmgResource::size() const noexcept { return _storage->bytes.size(); }
std::size_t NativeBmgResource::message_count() const noexcept { return _storage->text_offsets.size(); }
const wchar_t* NativeBmgResource::message(std::size_t index) const {
    return reinterpret_cast<const wchar_t*>(_storage->bytes.data() + _storage->text_begin + _storage->text_offsets.at(index));
}
const std::uint16_t* NativeBmgResource::message_utf16(std::size_t index) const {
    if (index >= message_count())
        aurora::throw_host_exception<std::out_of_range>("BMG message index is outside its original information table");
    return _storage->text_utf16.data() + _storage->text_offsets[index] / sizeof(wchar_t);
}
std::optional<std::size_t> NativeBmgResource::message_index(const wchar_t* pointer) const noexcept {
    if (!pointer) return std::nullopt;
    for (std::size_t i = 0; i < message_count(); ++i)
        if (message(i) == pointer) return i;
    return std::nullopt;
}
std::unique_ptr<NativeBmgResource> make_native_bmg_resource(
    const void* bmg, std::size_t bmg_size, const void* ids, std::size_t ids_size) {
    compat::JkrHostAllocationScope host;
    if (!bmg || !ids)
        aurora::throw_host_exception<std::invalid_argument>("Native BMG initialization requires actual message and identifier resources");
    return std::make_unique<NativeBmgResource>(Bytes{static_cast<const std::uint8_t*>(bmg), bmg_size},
                                              Bytes{static_cast<const std::uint8_t*>(ids), ids_size});
}
}
