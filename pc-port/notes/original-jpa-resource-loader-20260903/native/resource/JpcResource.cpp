#include "resource/JpcResource.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include <JSystem/JParticle/JPABaseShape.hpp>
#include <JSystem/JParticle/JPAChildShape.hpp>
#include <JSystem/JParticle/JPADynamicsBlock.hpp>
#include <JSystem/JParticle/JPAExTexShape.hpp>
#include <JSystem/JParticle/JPAExtraShape.hpp>
#include <JSystem/JParticle/JPAFieldBlock.hpp>
#include <JSystem/JParticle/JPATexture.hpp>
#include <algorithm>
#include <bit>
#include <cstring>
#include <map>
#include <mutex>
#include <new>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace smgpc::resource {
namespace {
using Bytes = std::span<const std::uint8_t>;
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(std::string("JPC: ") + message);
}
void range(Bytes bytes, std::size_t offset, std::size_t size) {
    require(offset <= bytes.size() && size <= bytes.size() - offset, "range outside block");
}
std::uint16_t be16(Bytes bytes, std::size_t offset) {
    range(bytes, offset, 2);
    return (std::uint16_t(bytes[offset]) << 8) | bytes[offset + 1];
}
std::uint32_t be32(Bytes bytes, std::size_t offset) {
    range(bytes, offset, 4);
    return (std::uint32_t(bytes[offset]) << 24) | (std::uint32_t(bytes[offset + 1]) << 16) |
           (std::uint32_t(bytes[offset + 2]) << 8) | bytes[offset + 3];
}
constexpr std::uint32_t tag(char a, char b, char c, char d) {
    return (std::uint32_t(a) << 24) | (std::uint32_t(b) << 16) | (std::uint32_t(c) << 8) | std::uint32_t(d);
}
constexpr auto BEM = tag('B','E','M','1'), BSP = tag('B','S','P','1'), ESP = tag('E','S','P','1');
constexpr auto SSP = tag('S','S','P','1'), ETX = tag('E','T','X','1'), FLD = tag('F','L','D','1');
constexpr auto KFA = tag('K','F','A','1'), TDB = tag('T','D','B','1'), TEX = tag('T','E','X','1');
static_assert(sizeof(JPADynamicsBlockData) == 0x7c && sizeof(JPABaseShapeData) == 0x34);
static_assert(sizeof(JPAExtraShapeData) == 0x60 && sizeof(JPAChildShapeData) == 0x48);
static_assert(sizeof(JPAExTexShapeData) == 0x28 && sizeof(JPAFieldBlockData) == 0x44);
static_assert(sizeof(JPATextureData) == 0x40 && sizeof(ResTIMG) == 0x20);
struct BlockWriter {
    Bytes source;
    std::shared_ptr<void> owner;
    std::uint8_t* data;
    explicit BlockWriter(Bytes input) : source(input) {
        data = static_cast<std::uint8_t*>(::operator new(input.size(), std::align_val_t(16)));
        owner = std::shared_ptr<void>(data, [](void* p) { ::operator delete(p, std::align_val_t(16)); });
        std::memcpy(data, input.data(), input.size());
    }
    template<class T> void header() {
        static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>);
        range(source, 0, sizeof(T));
        std::construct_at(reinterpret_cast<T*>(data));
        std::memcpy(data, source.data(), sizeof(T));
    }
    void u16(std::size_t offset) {
        auto value = be16(source, offset);
        std::memcpy(data + offset, &value, sizeof(value));
    }
    void u32(std::size_t offset) {
        auto value = be32(source, offset);
        std::memcpy(data + offset, &value, sizeof(value));
    }
    void words(std::size_t start, std::size_t end) {
        for (auto offset = start; offset < end; offset += 4) u32(offset);
    }
    void halfwords(std::size_t start, std::size_t end) {
        for (auto offset = start; offset < end; offset += 2) u16(offset);
    }
    // Numeric tails are actual arrays accessed by the original constructors.
    void floats(std::size_t start, std::size_t count) {
        range(source, start, count * sizeof(float));
        require(start % alignof(float) == 0, "unaligned float table");
        for (std::size_t i = 0; i < count; ++i)
            std::construct_at(reinterpret_cast<float*>(data + start) + i, std::bit_cast<float>(be32(source, start + i * 4)));
    }
    void colors(std::size_t start, std::size_t count, std::int16_t max_frame) {
        range(source, start, count * sizeof(JPAClrAnmKeyData));
        require(start >= sizeof(JPABaseShapeData) && start <= 0x7fff && start % 2 == 0 && count != 0 && max_frame >= 0 && max_frame < 0x7fff,
                "invalid color table");
        std::int16_t previous = -1;
        for (std::size_t i = 0; i < count; ++i) {
            const auto offset = start + i * sizeof(JPAClrAnmKeyData);
            const auto frame = std::bit_cast<std::int16_t>(be16(source, offset));
            require(frame > previous, "invalid color key frame");
            auto* key = std::construct_at(reinterpret_cast<JPAClrAnmKeyData*>(data + offset));
            key->index = frame;
            std::memcpy(&key->color, source.data() + offset + 2, sizeof(GXColor));
            previous = frame;
        }

    }
};
JpcBlock decode_block(Bytes source, std::size_t source_offset, std::uint8_t texture_refs) {
    const auto magic = be32(source, 0);
    BlockWriter out(source);
    switch (magic) {
    case BEM:
        out.header<JPADynamicsBlockData>(); out.words(4, 0x68); out.halfwords(0x68, 0x78); break;
    case BSP: {
        out.header<JPABaseShapeData>();
        out.words(4, 0xc); out.halfwords(0xc, 0x10); out.words(0x10, 0x18); out.u16(0x18); out.u16(0x24);
        std::size_t tail = 0x34;
        if (be32(source, 8) & 0x01000000) { out.floats(tail, 10); tail += 0x28; }
        if (source[0x1e] & 1) { range(source, tail, source[0x1f]); require(source[0x1f] != 0, "empty texture animation"); }
        const auto max_frame = std::bit_cast<std::int16_t>(be16(source, 0x24));
        if (source[0x21] & 2) out.colors(be16(source, 0xc), source[0x22], max_frame);
        if (source[0x21] & 8) out.colors(be16(source, 0xe), source[0x23], max_frame);
        break;
    }
    case ESP:
        out.header<JPAExtraShapeData>(); out.words(4, 0x28); out.halfwords(0x28, 0x2c); out.words(0x2c, 0x60); break;
    case SSP:
        out.header<JPAChildShapeData>(); out.words(4, 0x34); out.u32(0x3c); out.halfwords(0x40, 0x44); out.u16(0x46); break;
    case ETX:
        out.header<JPAExTexShapeData>(); out.words(4, 0x24); break;
    case FLD:
        out.header<JPAFieldBlockData>(); out.words(4, 0x40); break;
    case KFA:
        range(source, 0, 0xc); out.u32(4); require(source[9] != 0, "empty key table"); out.floats(0xc, std::size_t(source[9]) * 4); break;
    case TDB:
        out.u32(4); range(source, 8, std::size_t(texture_refs) * 2);
        for (std::size_t i = 0; i < texture_refs; ++i)
            std::construct_at(reinterpret_cast<std::uint16_t*>(out.data + 8) + i, be16(source, 8 + i * 2));
        break;
    case TEX: {
        out.header<JPATextureData>(); out.u32(4); out.u16(0x22); out.u16(0x24); out.u16(0x2a);
        out.u32(0x2c); out.u16(0x3a); out.u32(0x3c);
        require(std::find(source.begin() + 0xc, source.begin() + 0x20, 0) != source.begin() + 0x20, "unterminated texture name");
        require(source[0x30] <= 1 && source[0x31] <= 1 && source[0x32] <= 1, "invalid texture bool");
        const auto image_offset = be32(source, 0x3c);
        std::size_t image_start = 0x20 + (image_offset ? image_offset : 0x20);
        std::size_t width = be16(source, 0x22), height = be16(source, 0x24);
        require(width && height && source[0x38], "empty texture image");
        std::size_t tile_width, tile_height, tile_bytes = 32;
        switch (source[0x20]) {
        case 0: case 8: case 14: tile_width = 8; tile_height = 8; break;
        case 1: case 2: case 9: tile_width = 8; tile_height = 4; break;
        case 3: case 4: case 5: case 10: tile_width = tile_height = 4; break;
        case 6: tile_width = tile_height = 4; tile_bytes = 64; break;
        default: throw std::runtime_error("JPC: unsupported GX texture format");
        }
        for (std::size_t level = 0; level < source[0x38]; ++level) {
            const auto size = ((width + tile_width - 1) / tile_width) * ((height + tile_height - 1) / tile_height) * tile_bytes;
            range(source, image_start, size);
            image_start += size;
            width = std::max<std::size_t>(1, width / 2);
            height = std::max<std::size_t>(1, height / 2);
        }
        const auto palette_count = be16(source, 0x2a);
        if (palette_count) range(source, 0x20 + be32(source, 0x2c), std::size_t(palette_count) * 2);
        break;
    }
    default: out.u32(4); break; // Original loader ignores unknown block kinds.
    }
    return {magic, source_offset, {out.data, source.size()}, std::move(out.owner)};
}
struct RegisteredSource {
    std::span<const std::uint8_t> source;
    std::shared_ptr<const void> source_owner;
    std::shared_ptr<const JpcResource> decoded;
};
struct Registry {
    std::mutex mutex;
    std::map<const void*, std::weak_ptr<RegisteredSource>> entries;
};
Registry& registry() { static Registry value; return value; }
} // namespace

JpcResource::JpcResource(std::span<const std::uint8_t> source) {
    compat::JkrHostAllocationScope host;
    require(be32(source, 0) == tag('J','P','A','C') && be32(source, 4) == tag('2','-','1','0'), "expected JPAC2-10");
    range(source, 0, 0x10);
    _source.assign(source.begin(), source.end());
    source = _source;
    const auto resource_count = be16(source, 8), texture_count = be16(source, 0xa);
    const auto texture_offset = std::size_t(be32(source, 0xc));
    require(texture_offset >= 0x10 && texture_offset <= source.size(), "invalid texture table offset");
    std::set<std::uint16_t> ids;
    std::size_t offset = 0x10;
    _resources.reserve(resource_count);
    for (std::size_t i = 0; i < resource_count; ++i) {
        range(source.first(texture_offset), offset, 8);
        JpcResourceRecord record{be16(source, offset), source[offset + 4], source[offset + 5], source[offset + 6], {}};
        require(ids.insert(record.user_index).second, "duplicate resource user index");
        const auto count = be16(source, offset + 2);
        offset += 8;
        std::size_t fields = 0, keys = 0;
        std::set<std::uint32_t> singletons;
        for (std::size_t j = 0; j < count; ++j) {
            const auto size = be32(source.first(texture_offset), offset + 4);
            require(size >= 8, "block smaller than its header");
            range(source.first(texture_offset), offset, size);
            const auto magic = be32(source, offset);
            if (magic == FLD) ++fields;
            else if (magic == KFA) ++keys;
            else if (magic == BEM || magic == BSP || magic == ESP || magic == SSP || magic == ETX || magic == TDB)
                require(singletons.insert(magic).second, "duplicate singleton block");
            require(magic != TEX, "texture inside resource record");
            record.blocks.push_back(decode_block(source.subspan(offset, size), offset, record.texture_reference_count));
            offset += size;
        }
        require(fields == record.field_count && keys == record.key_count, "block counts disagree with resource header");
        require(singletons.contains(BEM) && singletons.contains(BSP), "resource missing dynamics or base shape");
        require(record.texture_reference_count == 0 || singletons.contains(TDB), "resource missing texture references");
        for (const auto& block : record.blocks) if (block.tag == TDB) {
            for (std::size_t j = 0; j < record.texture_reference_count; ++j)
                require(be16(source, block.source_offset + 8 + j * 2) < texture_count, "texture reference outside texture table");
        }
        _resources.push_back(std::move(record));
    }
    offset = texture_offset;
    _textures.reserve(texture_count);
    for (std::size_t i = 0; i < texture_count; ++i) {
        const auto size = be32(source, offset + 4);
        require(size >= 0x40 && be32(source, offset) == TEX, "invalid texture block");
        range(source, offset, size);
        _textures.push_back(decode_block(source.subspan(offset, size), offset, 0));
        offset += size;
    }
}

struct JpcSourceRegistration::State {
    std::shared_ptr<RegisteredSource> source;
    ~State() {
        if (!source) return;
        compat::JkrHostAllocationScope host;
        auto& owners = registry();
        const std::lock_guard lock(owners.mutex);
        const auto identity = source->source.data();
        source.reset();
        auto entry = owners.entries.find(identity);
        if (entry != owners.entries.end() && entry->second.expired()) owners.entries.erase(entry);
    }
};
JpcSourceRegistration::JpcSourceRegistration(std::unique_ptr<State> state) : _state(std::move(state)) {}
JpcSourceRegistration::~JpcSourceRegistration() = default;
JpcSourceRegistration::JpcSourceRegistration(JpcSourceRegistration&&) noexcept = default;
JpcSourceRegistration& JpcSourceRegistration::operator=(JpcSourceRegistration&&) noexcept = default;
JpcSourceRegistration register_jpc_source(std::span<const std::uint8_t> bytes, std::shared_ptr<const void> source_owner) {
    compat::JkrHostAllocationScope host;
    require(!bytes.empty() && bool(source_owner), "registration requires retained bounded source bytes");
    auto& owners = registry();
    const std::lock_guard lock(owners.mutex);
    const auto found = owners.entries.find(bytes.data());
    auto source = found == owners.entries.end() ? std::shared_ptr<RegisteredSource>{} : found->second.lock();
    if (source) {
        require(source->source.size() == bytes.size() && source->source_owner.get() == source_owner.get() &&
                !source->source_owner.owner_before(source_owner) && !source_owner.owner_before(source->source_owner),
                "identity belongs to another owner");
    } else {
        source = std::make_shared<RegisteredSource>();
        source->source = bytes;
        source->source_owner = std::move(source_owner);
        source->decoded = std::make_shared<JpcResource>(bytes);
    }
    auto state = std::make_unique<JpcSourceRegistration::State>();
    // No operation after publication can throw; State retires the weak entry.
    owners.entries.insert_or_assign(bytes.data(), source);
    state->source = std::move(source);
    return JpcSourceRegistration(std::move(state));
}
std::shared_ptr<const JpcResource> resolve_jpc_source(const void* identity) {
    compat::JkrHostAllocationScope host;
    auto& owners = registry();
    const std::lock_guard lock(owners.mutex);
    const auto found = owners.entries.find(identity);
    require(found != owners.entries.end(), "unsized SDK source has no registered byte range");
    auto source = found->second.lock();
    require(bool(source), "registered source expired");
    return source->decoded;
}
} // namespace smgpc::resource
