#pragma once

#include "Game/Util/JMapInfo.hpp"

#include <cstdint>
#include <memory>
#include <span>

namespace smgpc::resource {

    // A bounded binary resource for the original unsized JMapInfo::attach API.
    // Copies share the retained bytes and decoded table identity.
    class JMapResource final {
    public:
        explicit JMapResource(std::span<const std::uint8_t> bytes);

        [[nodiscard]] const void* data() const;
        [[nodiscard]] std::span<const std::uint8_t> bytes() const;

    private:
        struct Storage;
        std::shared_ptr<Storage> _storage;
    };

    [[nodiscard]] std::shared_ptr<const JMapInfo> find_jmap_resource(const void* data);

}  // namespace smgpc::resource
