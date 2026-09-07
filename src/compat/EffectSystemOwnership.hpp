#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

class EffectSystem;
class LiveActor;

namespace smgpc::compat {
    class JkrAllocationDomain;

    // Retains the original scene heap and process particle catalog. NameObj
    // roots and adaptors remain owned by the scene's registration graph.
    class EffectSystemOwnership final {
    public:
        static constexpr std::size_t default_byte_budget = 8U * 1024U * 1024U;
        explicit EffectSystemOwnership(std::size_t byte_budget = default_byte_budget);
        ~EffectSystemOwnership();
        EffectSystemOwnership(const EffectSystemOwnership &) = delete;
        EffectSystemOwnership &operator=(const EffectSystemOwnership &) = delete;
        [[nodiscard]] EffectSystem *construct();
        void entry(std::uint32_t particles, std::uint32_t emitters);
        void retire() noexcept;
        [[nodiscard]] EffectSystem &system() const;
        [[nodiscard]] const std::shared_ptr<JkrAllocationDomain> &allocation_domain() const noexcept;

    private:
        struct Storage;
        std::unique_ptr<Storage> _storage;
    };

    void initialize_actor_effect_keeper(LiveActor *, int capacity, const char *name, bool sort);
    void release_actor_effect_keeper(const LiveActor *) noexcept;
}  // namespace smgpc::compat
