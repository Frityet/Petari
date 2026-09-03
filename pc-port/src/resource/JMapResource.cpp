#include "resource/JMapResource.hpp"

#include <map>
#include <mutex>
#include <vector>

namespace {
    struct Registry {
        std::mutex mutex;
        std::map<const void*, std::weak_ptr<const JMapInfo>> tables;
    };

    Registry& registry() {
        static Registry value;
        return value;
    }
}

namespace smgpc::resource {
    struct JMapResource::Storage {
        std::vector<std::uint8_t> bytes;
        std::shared_ptr<const JMapInfo> table;

        explicit Storage(std::span<const std::uint8_t> source)
            : bytes(source.begin(), source.end()),
              table(std::make_shared<JMapInfo>(JMapInfo::from_bcsv(bytes))) {
            auto& owners = registry();
            const std::lock_guard lock(owners.mutex);
            owners.tables.emplace(bytes.data(), table);
        }

        ~Storage() {
            auto& owners = registry();
            const std::lock_guard lock(owners.mutex);
            owners.tables.erase(bytes.data());
        }
    };

    JMapResource::JMapResource(std::span<const std::uint8_t> bytes)
        : _storage(std::make_shared<Storage>(bytes)) {
    }

    const void* JMapResource::data() const {
        return _storage->bytes.data();
    }

    std::span<const std::uint8_t> JMapResource::bytes() const {
        return _storage->bytes;
    }

    std::shared_ptr<const JMapInfo> find_jmap_resource(const void* data) {
        auto& owners = registry();
        const std::lock_guard lock(owners.mutex);
        const auto found = owners.tables.find(data);
        return found != owners.tables.end() ? found->second.lock() : nullptr;
    }
}
