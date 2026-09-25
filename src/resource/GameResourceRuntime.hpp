#pragma once

#include <JSystem/JKernel/JKRHeap.hpp>
#include <aurora/allocation.hpp>
#include "resource/Mem1ResourceHeap.hpp"

#include <cstddef>
#include <memory>

namespace smgpc::resource {
    class EmbeddedGameTables;
    struct GameResourceBudget {
        std::size_t host_heap_bytes = 128U * 1024U * 1024U;
        std::size_t cohort_bytes = 64U * 1024U * 1024U;
        std::size_t mem1_bytes = 16U * 1024U * 1024U;
        std::size_t scenario_catalog_bytes = 8U * 1024U * 1024U;
        std::size_t particle_resource_bytes = 2U * 1024U * 1024U;
        std::size_t save_data_bytes = 1024U * 1024U;
        std::size_t message_resource_bytes = 2U * 1024U * 1024U;
    };

    // Explicit process startup after Aurora configuration. Reuse this owner
    // across RuntimeContexts; no resource request reinitializes OS memory.
    class GameResourceRuntime final {
    public:
        explicit GameResourceRuntime(GameResourceBudget budget = {});
        ~GameResourceRuntime();
        GameResourceRuntime(const GameResourceRuntime&) = delete;
        GameResourceRuntime& operator=(const GameResourceRuntime&) = delete;
        [[nodiscard]] static GameResourceRuntime* active() noexcept;
        [[nodiscard]] const std::shared_ptr<Mem1ResourceHeap>& mem1_heap() const noexcept;
        [[nodiscard]] const JKRHeap::Handle& root_heap() const noexcept;
        void prepare_mem2_arena(std::size_t bytes);
        [[nodiscard]] const std::shared_ptr<void>& mem2_storage() const noexcept;
        [[nodiscard]] const GameResourceBudget& budget() const noexcept;

    private:
        GameResourceBudget _budget;
        std::shared_ptr<void> _mem2Storage;
        JKRHeap::Handle _rootHeap;
        std::shared_ptr<Mem1ResourceHeap> _mem1;
        std::unique_ptr<EmbeddedGameTables> _embedded_tables;
    };
}
