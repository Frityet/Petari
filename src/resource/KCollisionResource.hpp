#pragma once

#include "Game/Map/KCollision.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <span>

namespace smgpc::resource {

    class KCollisionResource final {
    public:
        explicit KCollisionResource(std::span<const std::uint8_t> kcl,
                                    std::span<const std::uint8_t> attributes = {});

        [[nodiscard]] KCLFile* native_file() const;
        [[nodiscard]] const void* attributes_data() const;
        [[nodiscard]] std::span<const std::uint8_t> source_bytes() const;
        [[nodiscard]] std::span<const std::uint8_t> native_octree() const;
        [[nodiscard]] const std::array<std::uint32_t, 4>& source_offsets() const;

    private:
        struct Storage;
        std::shared_ptr<Storage> _storage;
    };

    // Owns a real original server, its constructor-allocated JMapInfo, and all
    // resources it borrows. It does not create CollisionParts or scene objects.
    class OwnedKCollisionServer final {
    public:
        explicit OwnedKCollisionServer(KCollisionResource resource);
        ~OwnedKCollisionServer();
        OwnedKCollisionServer(const OwnedKCollisionServer&) = delete;
        OwnedKCollisionServer& operator=(const OwnedKCollisionServer&) = delete;

        [[nodiscard]] KCollisionServer& server();
        [[nodiscard]] const KCollisionServer& server() const;

    private:
        KCollisionResource _resource;
        KCollisionServer _server;
        std::unique_ptr<JMapInfo> _map_info;
    };

    // Retains separately allocated, mutable original geometry. The explicit
    // spans include the prism sentinel; no relationship between allocations is
    // inferred. Octree halfwords are authored Wii-order numeric values, as in
    // DynamicCollisionObj, rather than a host-endian array of node words.
    class GeneratedKCollisionResource final {
    public:
        GeneratedKCollisionResource(KCLFile& file, std::span<TVec3f> positions,
                                    std::span<TVec3f> normals, std::span<KC_PrismData> prisms,
                                    std::span<const std::uint16_t> octree_halfwords,
                                    std::shared_ptr<void> allocation_owner);

        [[nodiscard]] KCLFile* native_file() const;
        // Validates mutations before the collision owner publishes them.
        void validate() const;

    private:
        struct Storage;
        std::shared_ptr<Storage> _storage;
    };

    [[nodiscard]] bool is_native_kcollision_file(const void* data);
    [[nodiscard]] KCLFile* require_native_kcollision_file(void* data);
    [[nodiscard]] s32 native_kcollision_triangle_count(const KCLFile* file);

}  // namespace smgpc::resource
