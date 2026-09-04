#pragma once

#include "compat/JkrAllocationDomain.hpp"
#include "resource/Mem1ResourceHeap.hpp"

#include <cstddef>
#include <memory>

namespace smgpc::compat { class JutTextureAllocationService; }

namespace smgpc::resource {
    class EmbeddedGameTables;
    struct GameResourceBudget {
        std::size_t host_heap_bytes = 128U * 1024U * 1024U;
        std::size_t cohort_bytes = 64U * 1024U * 1024U;
        std::size_t mem1_bytes = 16U * 1024U * 1024U;
        std::size_t scenario_catalog_bytes = 8U * 1024U * 1024U;
        std::size_t particle_resource_bytes = 2U * 1024U * 1024U;
    };

    // Explicit process startup after Aurora configuration. Reuse this owner
    // across RuntimeContexts; no resource request reinitializes OS memory.
    class GameResourceRuntime final {
    public:
        explicit GameResourceRuntime(GameResourceBudget budget = {});
        ~GameResourceRuntime();
        GameResourceRuntime(const GameResourceRuntime&) = delete;
        GameResourceRuntime& operator=(const GameResourceRuntime&) = delete;
        [[nodiscard]] std::shared_ptr<compat::JkrAllocationDomain> create_cohort() const;
        [[nodiscard]] const std::shared_ptr<Mem1ResourceHeap>& mem1_heap() const noexcept;
        [[nodiscard]] const std::shared_ptr<compat::JkrHeapRuntime>& host_heaps() const noexcept;
        [[nodiscard]] const GameResourceBudget& budget() const noexcept;

    private:
        GameResourceBudget _budget;
        std::shared_ptr<compat::JkrHeapRuntime> _heaps;
        std::shared_ptr<Mem1ResourceHeap> _mem1;
        std::unique_ptr<compat::JutTextureAllocationService> _textures;
        std::unique_ptr<EmbeddedGameTables> _embedded_tables;
    };
}
