#include "resource/JMapResource.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include <algorithm>
#include <map>
#include <mutex>
#include <stdexcept>
#include <vector>
namespace {
    struct DeferredJMap {
        std::shared_ptr<const void> source_owner;
        std::span<const std::uint8_t> bytes;
        std::once_flag decode_once;
        std::shared_ptr<const JMapInfo> table;

        std::shared_ptr<const JMapInfo> load() {
            std::call_once(decode_once, [&] {
                table = std::make_shared<JMapInfo>(JMapInfo::from_bcsv(bytes));
            });
            return table;
        }
    };
    struct Registry {
        std::mutex mutex;
        struct Entry {
            std::weak_ptr<void> owner;
            std::weak_ptr<const JMapInfo> table;
            std::uint64_t generation;
            std::size_t references;
            std::weak_ptr<DeferredJMap> deferred;
        };
        std::map<const void *, Entry> tables;
        std::uint64_t next_generation = 1;
    };
    Registry &registry() {
        static Registry value;
        return value;
    }
}  // namespace
namespace smgpc::resource {
    struct JMapResource::Storage {
        std::vector<std::uint8_t> bytes;
        std::shared_ptr<const JMapInfo> table;
        std::uint64_t generation = 0;
        explicit Storage(std::span<const std::uint8_t> source)
            : bytes(source.begin(), source.end()), table(std::make_shared<JMapInfo>(JMapInfo::from_bcsv(bytes))) {
        }
        ~Storage() {
            compat::JkrHostAllocationScope host;
            auto &owners = registry();
            const std::lock_guard lock(owners.mutex);
            const auto entry = owners.tables.find(bytes.data());
            if (entry != owners.tables.end() && entry->second.generation == generation)
                owners.tables.erase(entry);
        }
    };
    struct JMapSourceRegistration::State {
        std::shared_ptr<void> owner;
        const void *identity;
        std::uint64_t generation;
        State(std::shared_ptr<void> resource, const void *key, std::uint64_t id)
            : owner(std::move(resource)), identity(key), generation(id) {
        }
        ~State() {
            compat::JkrHostAllocationScope host;
            auto &owners = registry();
            const std::lock_guard lock(owners.mutex);
            const auto entry = owners.tables.find(identity);
            if (entry != owners.tables.end() && entry->second.generation == generation && --entry->second.references == 0)
                owners.tables.erase(entry);
        }
    };
    JMapSourceRegistration::JMapSourceRegistration(std::unique_ptr<State> state) : _state(std::move(state)) {
    }
    JMapSourceRegistration::~JMapSourceRegistration() = default;
    JMapSourceRegistration::JMapSourceRegistration(JMapSourceRegistration &&) noexcept = default;
    JMapSourceRegistration &JMapSourceRegistration::operator=(JMapSourceRegistration &&) noexcept = default;
    JMapSourceRegistration register_jmap_source(std::span<const std::uint8_t> bytes,
                                               std::shared_ptr<const void> source_owner) {
        compat::JkrHostAllocationScope host;
        if (bytes.empty() || !source_owner)
            throw std::invalid_argument("Deferred JMap source requires a nonempty retained byte range");
        auto source = std::make_shared<DeferredJMap>();
        source->source_owner = std::move(source_owner);
        source->bytes = bytes;
        auto &owners = registry();
        const std::lock_guard lock(owners.mutex);
        const auto found = owners.tables.find(bytes.data());
        std::uint64_t generation;
        if (found != owners.tables.end()) {
            auto existing = found->second.deferred.lock();
            if (!existing || existing->source_owner.get() != source->source_owner.get() ||
                existing->source_owner.owner_before(source->source_owner) ||
                source->source_owner.owner_before(existing->source_owner) || existing->bytes.size() != bytes.size())
                throw std::logic_error("JMap source identity belongs to a different retained byte range");
            source = std::move(existing);
            generation = found->second.generation;
            ++found->second.references;
        } else {
            generation = owners.next_generation++;
            if (generation == 0)
                throw std::overflow_error("JMap registration identity exhausted");
            owners.tables.emplace(bytes.data(), Registry::Entry{source, {}, generation, 1, source});
        }
        try {
            return JMapSourceRegistration(std::make_unique<JMapSourceRegistration::State>(source, bytes.data(), generation));
        } catch (...) {
            auto entry = owners.tables.find(bytes.data());
            if (--entry->second.references == 0)
                owners.tables.erase(entry);
            throw;
        }
    }
    JMapResource::JMapResource(std::span<const std::uint8_t> source) {
        compat::JkrHostAllocationScope host;
        _storage = std::make_shared<Storage>(source);
        auto &owners = registry();
        const std::lock_guard lock(owners.mutex);
        _storage->generation = owners.next_generation++;
        if (_storage->generation == 0)
            throw std::overflow_error("JMap registration identity exhausted");
        owners.tables.emplace(_storage->bytes.data(), Registry::Entry{_storage, _storage->table, _storage->generation, 1});
    }
    JMapSourceRegistration JMapResource::register_source(std::span<const std::uint8_t> alias) {
        compat::JkrHostAllocationScope host;
        if (alias.size() != _storage->bytes.size() || !std::equal(alias.begin(), alias.end(), _storage->bytes.begin()))
            throw std::invalid_argument("JMap alias does not match the complete retained source");
        auto &owners = registry();
        const std::lock_guard lock(owners.mutex);
        const auto found = owners.tables.find(alias.data());
        std::uint64_t generation;
        if (found != owners.tables.end()) {
            if (found->second.owner.lock().get() != _storage.get())
                throw std::logic_error("JMap source identity belongs to a different resource owner");
            generation = found->second.generation;
            ++found->second.references;
        } else {
            generation = owners.next_generation++;
            if (generation == 0)
                throw std::overflow_error("JMap registration identity exhausted");
            owners.tables.emplace(alias.data(), Registry::Entry{_storage, _storage->table, generation, 1});
        }
        try {
            return JMapSourceRegistration(std::make_unique<JMapSourceRegistration::State>(_storage, alias.data(), generation));
        } catch (...) {
            auto entry = owners.tables.find(alias.data());
            if (--entry->second.references == 0)
                owners.tables.erase(entry);
            throw;
        }
    }
    const void *JMapResource::data() const {
        return _storage->bytes.data();
    }
    std::span<const std::uint8_t> JMapResource::bytes() const {
        return _storage->bytes;
    }
    std::shared_ptr<const JMapInfo> find_jmap_resource(const void *data) {
        compat::JkrHostAllocationScope host;
        auto &owners = registry();
        std::shared_ptr<DeferredJMap> deferred;
        {
            const std::lock_guard lock(owners.mutex);
            const auto found = owners.tables.find(data);
            if (found == owners.tables.end())
                return nullptr;
            if (auto table = found->second.table.lock())
                return table;
            deferred = found->second.deferred.lock();
        }
        // Parsing and final source release can allocate or retire owners;
        // neither operation runs while holding the identity registry lock.
        return deferred ? deferred->load() : nullptr;
    }
}  // namespace smgpc::resource
