#include "resource/KCollisionResource.hpp"

#include "resource/JMapResource.hpp"

#include <bit>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>
#include <map>
#include <mutex>
#include <new>
#include <optional>
#include <set>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace {
    using Bytes = std::span<const std::uint8_t>;

    void require(bool condition, const char* message) {
        if (!condition) {
            throw std::invalid_argument(message);
        }
    }

    void require_extent(Bytes bytes, std::size_t offset, std::size_t size) {
        require(offset <= bytes.size() && size <= bytes.size() - offset, "KCL resource extent is out of bounds.");
    }

    std::uint16_t be16(Bytes bytes, std::size_t offset) {
        require_extent(bytes, offset, 2);
        return (static_cast<std::uint16_t>(bytes[offset]) << 8) | bytes[offset + 1];
    }

    std::uint32_t be32(Bytes bytes, std::size_t offset) {
        require_extent(bytes, offset, 4);
        return (static_cast<std::uint32_t>(bytes[offset]) << 24) |
               (static_cast<std::uint32_t>(bytes[offset + 1]) << 16) |
               (static_cast<std::uint32_t>(bytes[offset + 2]) << 8) | bytes[offset + 3];
    }

    float bef32(Bytes bytes, std::size_t offset) {
        return std::bit_cast<float>(be32(bytes, offset));
    }

    TVec3f vector_at(Bytes bytes, std::size_t offset) {
        TVec3f vector(bef32(bytes, offset), bef32(bytes, offset + 4), bef32(bytes, offset + 8));
        require(std::isfinite(vector.x) && std::isfinite(vector.y) && std::isfinite(vector.z),
                "KCL vector contains a non-finite component.");
        return vector;
    }

    KC_PrismData prism_at(Bytes bytes, std::size_t offset) {
        return {bef32(bytes, offset), be16(bytes, offset + 4), be16(bytes, offset + 6),
                {be16(bytes, offset + 8), be16(bytes, offset + 10), be16(bytes, offset + 12)}, be16(bytes, offset + 14)};
    }

    struct NativeAllocationDeleter {
        void operator()(std::byte* data) const {
            ::operator delete(data, std::align_val_t(alignof(std::max_align_t)));
        }
    };

    struct Registry {
        std::mutex mutex;
        std::map<const void*, std::weak_ptr<void>> files;
    };

    Registry& registry() {
        static Registry value;
        return value;
    }

    class OctreeDecoder {
    public:
        OctreeDecoder(Bytes source, std::uint8_t* destination, const KCLFile& file, std::size_t prism_count)
            : _source(source), _destination(destination), _file(file), _prism_count(prism_count),
              _types((source.size() + 1) / 2) {
        }

        void decode() {
            const auto width = _file.mBlockWidthShift;
            require(width >= 0 && width < 32, "KCL block width shift is invalid.");
            const bool single_root = _file.mBlockXShift == -1 && _file.mBlockXYShift == -1;
            require(single_root || (_file.mBlockXShift >= 0 && _file.mBlockXShift < 32 &&
                                    _file.mBlockXYShift >= 0 && _file.mBlockXYShift < 32),
                    "KCL block axis shifts are invalid.");
            const auto allowed_x = ~static_cast<std::uint32_t>(_file.mXMask);
            const auto allowed_y = ~static_cast<std::uint32_t>(_file.mYMask);
            const auto allowed_z = ~static_cast<std::uint32_t>(_file.mZMask);
            const std::uint32_t roots = single_root ? 0U :
                ((allowed_x >> width) | ((allowed_y >> width) << _file.mBlockXShift) |
                 ((allowed_z >> width) << _file.mBlockXYShift));
            require_extent(_source, static_cast<std::size_t>(roots) * 4, 4);
            subsets(roots, [&](std::uint32_t index) { node(0, static_cast<std::size_t>(index) * 4, width); });

            // searchBlock returns a pointer one u16 before the first index.
            // That unused prefix may overlap a node word or another leaf list.
            for (const auto prefix : _prefixes) {
                if (_types[prefix / 2] == 0) {
                    std::construct_at(reinterpret_cast<std::uint16_t*>(_destination + prefix), be16(_source, prefix));
                }
            }
        }

    private:
        template<class Function>
        static void subsets(std::uint32_t mask, Function function) {
            std::uint32_t value = 0;
            do {
                function(value);
                value = (value - mask) & mask;
            } while (value != 0);
        }

        void node(std::size_t base, std::size_t offset, s32 remaining) {
            require(base <= _source.size() && offset <= _source.size() - base, "KCL octree node offset is invalid.");
            const auto address = base + offset;
            require((address & 3U) == 0, "KCL octree node is not word aligned.");
            require_extent(_source, address, 4);
            if (!_visited.emplace(base, offset, remaining).second) {
                return;
            }
            require(_types[address / 2] != 2 && _types[address / 2 + 1] != 2,
                    "KCL octree node overlaps a live leaf index.");
            _types[address / 2] = _types[address / 2 + 1] = 1;
            const auto value = be32(_source, address);
            std::construct_at(reinterpret_cast<s32*>(_destination + address), std::bit_cast<s32>(value));
            const auto relative = static_cast<std::size_t>(value & 0x7fffffffU);
            require(relative <= _source.size() - base, "KCL octree child offset is out of bounds.");
            const auto child = base + relative;
            if ((value & 0x80000000U) != 0) {
                leaf(child);
                return;
            }
            require(remaining > 0, "KCL octree subdivision exhausts the coordinate bits.");
            const auto shift = remaining - 1;
            const std::uint32_t children =
                (((~static_cast<std::uint32_t>(_file.mXMask)) >> shift) & 1U) |
                ((((~static_cast<std::uint32_t>(_file.mYMask)) >> shift) & 1U) << 1) |
                ((((~static_cast<std::uint32_t>(_file.mZMask)) >> shift) & 1U) << 2);
            subsets(children, [&](std::uint32_t index) { node(child, index * 4U, shift); });
        }

        void leaf(std::size_t prefix) {
            require((prefix & 1U) == 0, "KCL octree leaf is not halfword aligned.");
            require_extent(_source, prefix, 2);
            _prefixes.insert(prefix);
            for (auto address = prefix + 2;; address += 2) {
                const auto index = be16(_source, address);
                require(_types[address / 2] != 1, "KCL octree leaf index overlaps a node word.");
                _types[address / 2] = 2;
                require(index <= _prism_count, "KCL octree leaf references a missing prism.");
                std::construct_at(reinterpret_cast<std::uint16_t*>(_destination + address), index);
                if (index == 0) {
                    return;
                }
            }
        }

        Bytes _source;
        std::uint8_t* _destination;
        const KCLFile& _file;
        std::size_t _prism_count;
        std::vector<std::uint8_t> _types;
        std::set<std::tuple<std::size_t, std::size_t, s32>> _visited;
        std::set<std::size_t> _prefixes;
    };
}

namespace smgpc::resource {
    struct KCollisionResource::Storage {
        KCLFile file{};
        std::vector<std::uint8_t> source;
        std::array<std::uint32_t, 4> offsets{};
        std::vector<TVec3f> positions;
        std::vector<TVec3f> normals;
        std::unique_ptr<std::byte[], NativeAllocationDeleter> geometry;
        std::size_t octree_size = 0;
        std::optional<JMapResource> attributes;

        Storage(Bytes bytes, Bytes pa) : source(bytes.begin(), bytes.end()) {
            require_extent(bytes, 0, 0x38);
            for (std::size_t i = 0; i < offsets.size(); ++i) {
                offsets[i] = be32(bytes, i * 4);
            }
            const std::size_t position_offset = offsets[0];
            const std::size_t normal_offset = offsets[1];
            const std::size_t dummy_offset = offsets[2];
            const std::size_t prism_offset = dummy_offset + 16;
            const std::size_t octree_offset = offsets[3];
            require(position_offset >= 0x38 && normal_offset >= position_offset &&
                    prism_offset >= normal_offset && octree_offset >= prism_offset && octree_offset <= bytes.size(),
                    "KCL array offsets are inconsistent.");
            require((position_offset & 3U) == 0 && (normal_offset & 3U) == 0 &&
                    (dummy_offset & 3U) == 0 && (octree_offset & 3U) == 0,
                    "KCL arrays are not word aligned.");
            require((normal_offset - position_offset) % 12 == 0 && (prism_offset - normal_offset) % 12 == 0 &&
                    (octree_offset - prism_offset) % 16 == 0, "KCL array has a partial record.");
            const auto prism_count = (octree_offset - prism_offset) / 16;
            require(prism_count <= static_cast<std::size_t>(std::numeric_limits<s32>::max()), "KCL prism count exceeds its signed API.");
            file.mThickness = bef32(bytes, 0x10);
            require(std::isfinite(file.mThickness), "KCL thickness is non-finite.");
            file.mMin = vector_at(bytes, 0x14);
            file.mXMask = std::bit_cast<s32>(be32(bytes, 0x20));
            file.mYMask = std::bit_cast<s32>(be32(bytes, 0x24));
            file.mZMask = std::bit_cast<s32>(be32(bytes, 0x28));
            file.mBlockWidthShift = std::bit_cast<s32>(be32(bytes, 0x2c));
            file.mBlockXShift = std::bit_cast<s32>(be32(bytes, 0x30));
            file.mBlockXYShift = std::bit_cast<s32>(be32(bytes, 0x34));
            for (auto offset = position_offset; offset < normal_offset; offset += 12) {
                positions.push_back(vector_at(bytes, offset));
            }
            for (auto offset = normal_offset; offset < prism_offset; offset += 12) {
                normals.push_back(vector_at(bytes, offset));
            }
            if (!pa.empty()) {
                attributes.emplace(pa);
            }
            const auto prism_bytes = (prism_count + 1) * sizeof(KC_PrismData);
            octree_size = bytes.size() - octree_offset;
            geometry.reset(static_cast<std::byte*>(::operator new(prism_bytes + octree_size,
                           std::align_val_t(alignof(std::max_align_t)))));
            file.mPos = positions.data();
            file.mNorms = normals.data();
            file.mPrisms = reinterpret_cast<KC_PrismData*>(geometry.get());
            file.mOctree = geometry.get() + prism_bytes;
            for (std::size_t index = 0; index <= prism_count; ++index) {
                const auto prism = prism_at(bytes, dummy_offset + index * 16);
                if (index != 0) {
                    require(std::isfinite(prism.mHeight) && prism.mPositionIndex < positions.size() &&
                            prism.mNormalIndex < normals.size() && prism.mEdgeIndices[0] < normals.size() &&
                            prism.mEdgeIndices[1] < normals.size() && prism.mEdgeIndices[2] < normals.size(),
                            "KCL prism references an invalid vector or has non-finite height.");
                }
                std::construct_at(file.mPrisms + index, prism);
            }
            const auto raw_octree = bytes.subspan(octree_offset);
            std::memcpy(file.mOctree, raw_octree.data(), octree_size);
            OctreeDecoder(raw_octree, static_cast<std::uint8_t*>(file.mOctree), file, prism_count).decode();
        }

        ~Storage() {
            auto& owners = registry();
            const std::lock_guard lock(owners.mutex);
            owners.files.erase(&file);
        }
    };

    KCollisionResource::KCollisionResource(Bytes bytes, Bytes attributes)
        : _storage(std::make_shared<Storage>(bytes, attributes)) {
        auto& owners = registry();
        const std::lock_guard lock(owners.mutex);
        owners.files.emplace(&_storage->file, _storage);
    }

    KCLFile* KCollisionResource::native_file() const { return &_storage->file; }
    const void* KCollisionResource::attributes_data() const { return _storage->attributes ? _storage->attributes->data() : nullptr; }
    Bytes KCollisionResource::source_bytes() const { return _storage->source; }
    Bytes KCollisionResource::native_octree() const {
        return {static_cast<const std::uint8_t*>(_storage->file.mOctree), _storage->octree_size};
    }
    const std::array<std::uint32_t, 4>& KCollisionResource::source_offsets() const { return _storage->offsets; }

    bool is_native_kcollision_file(const void* data) {
        auto& owners = registry();
        const std::lock_guard lock(owners.mutex);
        const auto found = owners.files.find(data);
        return found != owners.files.end() && !found->second.expired();
    }

    KCLFile* require_native_kcollision_file(void* data) {
        require(is_native_kcollision_file(data), "KCollisionServer requires a retained, decoded native KCL resource.");
        return static_cast<KCLFile*>(data);
    }

    OwnedKCollisionServer::OwnedKCollisionServer(KCollisionResource resource)
        : _resource(std::move(resource)), _server(), _map_info(_server.mapInfo) {
        _server.init(_resource.native_file(), _resource.attributes_data());
    }
    OwnedKCollisionServer::~OwnedKCollisionServer() = default;
    KCollisionServer& OwnedKCollisionServer::server() { return _server; }
    const KCollisionServer& OwnedKCollisionServer::server() const { return _server; }
}
