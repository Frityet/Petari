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

    [[nodiscard]] bool is_native_kcollision_file(const void* data);
    [[nodiscard]] KCLFile* require_native_kcollision_file(void* data);

}  // namespace smgpc::resource
