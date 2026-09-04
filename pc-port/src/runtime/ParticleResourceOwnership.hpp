#pragma once

#include <cstddef>
#include <memory>

class ParticleResourceHolder;
namespace smgpc::compat { class JkrHeapRuntime; }

namespace smgpc::runtime {
    class ArchiveMountService;

    // Process owner of the original particle catalog. Scene effect owners must
    // retain this owner until every emitter using its resources has retired.
    // The process ArchiveMountService must outlive this owner.
    class ParticleResourceOwnership final {
    public:
        static constexpr std::size_t default_byte_budget = 2U * 1024U * 1024U;
        ParticleResourceOwnership(std::shared_ptr<compat::JkrHeapRuntime>,
                                  std::size_t byte_budget, ArchiveMountService&);
        ~ParticleResourceOwnership();
        ParticleResourceOwnership(const ParticleResourceOwnership&) = delete;
        ParticleResourceOwnership& operator=(const ParticleResourceOwnership&) = delete;
        [[nodiscard]] ParticleResourceHolder& holder() const noexcept;
        [[nodiscard]] std::size_t construction_bytes() const noexcept;
        [[nodiscard]] static ParticleResourceOwnership* active() noexcept;
    private:
        struct Storage;
        std::unique_ptr<Storage> _storage;
    };
}
