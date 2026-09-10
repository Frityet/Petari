#include "compat/SaveChunkEncoding.hpp"

#include "Game/System/BinaryDataChunkHolder.hpp"

#include <aurora/allocation.hpp>
#include <aurora/endian.hpp>
#include <aurora/exception.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <limits>
#include <span>
#include <string_view>
#include <stdexcept>
#include <vector>

namespace smgpc::compat {
namespace {
constexpr u32 cPlay = 0x504c4159;
constexpr u32 cFlags = 0x464c4731;
constexpr u32 cPieces = 0x50434531;
constexpr u32 cPaths = 0x53504e31;
constexpr u32 cValues = 0x564c4531;
constexpr u32 cGalaxies = 0x47414c41;

enum class Order { Native, Wii };

u16 read_u16(std::span<const u8> bytes, std::size_t offset, Order order) noexcept {
    if (order == Order::Wii) return aurora::endian::read_u16(bytes.data() + offset);
    u16 value;
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}

constexpr u16 attribute_hash(std::string_view name) noexcept {
    u32 hash = 0;
    for (const auto byte : name) hash = static_cast<u8>(byte) + hash * 31U;
    return static_cast<u16>(hash);
}

bool contains(std::size_t size, std::size_t offset, std::size_t length) noexcept {
    return offset <= size && length <= size - offset;
}

// Only identify serialized tags and widths; the original deserializer still
// decides which galaxy, scenario and driver receives each value.
bool validate_path_tokens(std::span<const u8> bytes, std::size_t begin, std::size_t end) noexcept {
    bool has_zone = false;
    unsigned entries = 0;
    unsigned steps = 0;
    while (begin < end && ++steps <= 100) {
        const auto tag = bytes[begin++];
        if (tag == 0xff) return begin == end;
        if ((tag & 0xc0) == 0xc0) {
            has_zone = true;
            continue;
        }
        if (!has_zone || ++entries > 16) return false;
        if ((tag & 0x80) != 0) {
            if (begin == end) return false;
            ++begin;
        }
    }
    return false;
}

template <typename Visit>
bool walk_paths(std::span<const u8> bytes, Order order, Visit visit) noexcept {
    if (bytes.empty()) return false;
    std::size_t position = 1;
    for (unsigned galaxy = 0; galaxy < bytes[0]; ++galaxy) {
        if (!contains(bytes.size(), position, 6)) return false;
        const auto block_size = read_u16(bytes, position + 2, order);
        const auto scenarios = bytes[position + 4];
        if (block_size < 6 || !contains(bytes.size(), position, block_size)) return false;
        const auto end = position + block_size;
        visit(position, 2);
        visit(position + 2, 2);
        position += 6;
        for (unsigned scenario = 0; scenario < scenarios; ++scenario) {
            if (!contains(end, position, 3)) return false;
            const auto scenario_size = read_u16(bytes, position, order);
            if (scenario_size < 3 || !contains(end, position, scenario_size)) return false;
            if (!validate_path_tokens(bytes, position + 2, position + scenario_size)) return false;
            visit(position, 2);
            position += scenario_size;
        }
        // Later versions may append opaque bytes within a galaxy block.
        position = end;
    }
    return true;
}

template <typename Visit>
bool walk_galaxies(std::span<const u8> bytes, Order order, Visit visit) noexcept {
    if (bytes.size() < 6) return false;
    const auto records = read_u16(bytes, 0, order);
    const auto attributes = read_u16(bytes, 2, order);
    const auto record_size = read_u16(bytes, 4, order);
    const std::size_t header_size = 6 + std::size_t(attributes) * 4;
    if (header_size > bytes.size() || (records != 0 && record_size == 0)) return false;
    if (std::size_t(records) * record_size > bytes.size() - header_size) return false;

    struct Attribute {
        u16 hash;
        std::size_t width;
        std::size_t offset = std::numeric_limits<std::size_t>::max();
    };
    std::array fields{
        Attribute{attribute_hash("mGalaxyName"), 2},
        Attribute{attribute_hash("mPowerStarFlag"), 1},
        Attribute{attribute_hash("mFirstPlayFlag"), 1},
        Attribute{attribute_hash("mMaxCoinNum"), 16},
    };
    visit(0, 2);
    visit(2, 2);
    visit(4, 2);
    for (unsigned i = 0; i < attributes; ++i) {
        const std::size_t position = 6 + std::size_t(i) * 4;
        const auto hash = read_u16(bytes, position, order);
        const auto offset = read_u16(bytes, position + 2, order);
        if (offset > record_size) return false;
        for (auto& field : fields) {
            // BinaryDataContentAccessor uses the first matching descriptor.
            if (field.hash == hash && field.offset == std::numeric_limits<std::size_t>::max()) {
                if (!contains(record_size, offset, field.width)) return false;
                field.offset = offset;
            }
        }
        visit(position, 2);
        visit(position + 2, 2);
    }
    if (records != 0 && fields[0].offset == std::numeric_limits<std::size_t>::max()) return false;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (fields[i].offset == std::numeric_limits<std::size_t>::max()) continue;
        for (std::size_t j = 0; j < i; ++j) {
            if (fields[j].offset == std::numeric_limits<std::size_t>::max()) continue;
            if (fields[i].offset < fields[j].offset + fields[j].width &&
                fields[j].offset < fields[i].offset + fields[i].width) return false;
        }
    }
    for (unsigned record = 0; record < records; ++record) {
        const auto base = header_size + std::size_t(record) * record_size;
        visit(base + fields[0].offset, 2);
        if (fields[3].offset != std::numeric_limits<std::size_t>::max()) {
            for (unsigned coin = 0; coin < 8; ++coin) visit(base + fields[3].offset + coin * 2, 2);
        }
    }
    return true;
}

template <typename Visit>
bool walk_payload(u32 signature, std::span<const u8> bytes, Order order, Visit visit) noexcept {
    switch (signature) {
    case cPlay:
        if (bytes.size() < 7) return false;
        visit(1, 4);
        visit(5, 2);
        return true;
    case cPieces:
        if (bytes.size() < 32) return false;
        for (std::size_t offset = 0; offset < 32; offset += 2) visit(offset, 2);
        return true;
    case cFlags:
    case cValues:
        if (bytes.size() % (signature == cFlags ? 2 : 4) != 0) return false;
        for (std::size_t offset = 0; offset < bytes.size(); offset += 2) visit(offset, 2);
        return true;
    case cPaths:
        return walk_paths(bytes, order, visit);
    case cGalaxies:
        return walk_galaxies(bytes, order, visit);
    default:
        return true;
    }
}

bool transform(u32 signature, std::span<u8> payload, Order source) noexcept {
    // Validate before touching bytes so a failure cannot leave a mixed order.
    if (!walk_payload(signature, payload, source, [](std::size_t, std::size_t) {})) return false;
    if constexpr (std::endian::native == std::endian::little) {
        return walk_payload(signature, payload, source, [&](std::size_t offset, std::size_t width) {
            std::reverse(payload.begin() + offset, payload.begin() + offset + width);
        });
    }
    return true;
}

std::vector<u8> make_buffer(std::size_t size) {
    const aurora::allocation::HostAllocationScope host;
    return std::vector<u8>(size);
}
} // namespace

bool is_native_save_chunk(u32 signature) noexcept {
    return signature == cPlay || signature == cFlags || signature == cPieces ||
           signature == cPaths || signature == cValues || signature == cGalaxies;
}

bool validate_save_chunk_payload(u32 signature, const u8* payload, u32 size) noexcept {
    if (size != 0 && payload == nullptr) return false;
    if (signature == cPlay) return true; // Original PLAY has a legacy prefix-load contract.
    return walk_payload(signature, {payload, size}, Order::Wii, [](std::size_t, std::size_t) {});
}

s32 serialize_save_chunk(const BinaryDataChunkBase& chunk, u8* payload, u32 capacity) {
    const auto signature = chunk.getSignature();
    if (!is_native_save_chunk(signature)) return chunk.serialize(payload, capacity);
    if (signature == cPlay) {
        if (payload == nullptr || static_cast<s32>(capacity) <= 0) return 0;
        std::array<u8, 7> native{};
        if (chunk.serialize(native.data(), native.size()) != native.size() || !transform(signature, native, Order::Native)) {
            aurora::throw_host_exception<std::logic_error>("Original PLAY serializer did not produce its fixed payload");
        }
        const auto copied = std::min<std::size_t>(capacity, native.size());
        std::memcpy(payload, native.data(), copied);
        return static_cast<s32>(copied);
    }
    if (payload == nullptr || capacity > static_cast<u32>(std::numeric_limits<s32>::max()) ||
        (signature == cPieces && capacity < 32)) {
        aurora::throw_host_exception<std::length_error>("Original save chunk destination is unavailable or too small");
    }
    auto native = make_buffer(capacity);
    const s32 written = chunk.serialize(native.data(), capacity);
    if (written < 0 || static_cast<u32>(written) > capacity ||
        !transform(signature, {native.data(), static_cast<std::size_t>(written)}, Order::Native)) {
        aurora::throw_host_exception<std::length_error>("Original save chunk serializer produced an invalid payload");
    }
    std::memcpy(payload, native.data(), static_cast<std::size_t>(written));
    return written;
}

s32 deserialize_save_chunk(BinaryDataChunkBase& chunk, const u8* payload, u32 size) {
    const auto signature = chunk.getSignature();
    if (!is_native_save_chunk(signature)) return chunk.deserialize(payload, size);
    if (signature == cPlay) {
        std::array<u8, 7> native{0, 0, 0, 0, 0, 0, 4};
        if (payload != nullptr && static_cast<s32>(size) > 0) {
            std::memcpy(native.data(), payload, std::min<std::size_t>(size, native.size()));
        }
        transform(signature, native, Order::Wii);
        return chunk.deserialize(native.data(), native.size());
    }
    if (!validate_save_chunk_payload(signature, payload, size)) return -1;
    auto native = make_buffer(size);
    if (size != 0) std::memcpy(native.data(), payload, size);
    if (!transform(signature, native, Order::Wii)) return -1;
    return chunk.deserialize(native.data(), size);
}

} // namespace smgpc::compat
