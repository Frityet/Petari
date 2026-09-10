#pragma once

#include <aurora/allocation.hpp>

#include <cstddef>
#include <memory>

class JKRHeap;

namespace smgpc::compat {

    // Explicit process owner for the original reclaiming root heap, backed by
    // a caller-budgeted host arena. It does not reinitialize the OS allocator
    // or consume the separately reserved MEM1 texture heap.
    class JkrHeapRuntime final {
    public:
        [[nodiscard]] static std::shared_ptr<JkrHeapRuntime> create(std::size_t host_byte_budget);
        ~JkrHeapRuntime();
        JkrHeapRuntime(const JkrHeapRuntime&) = delete;
        JkrHeapRuntime& operator=(const JkrHeapRuntime&) = delete;
        [[nodiscard]] JKRHeap& root_heap() const noexcept;
        // Call before original HeapMemoryWatcher::createRootHeap. The budget
        // includes the original reserved MEM2 prefix and remains caller-owned.
        void prepare_mem2_arena(std::size_t byte_budget);

    private:
        friend class JkrAllocationDomain;
        explicit JkrHeapRuntime(std::size_t host_byte_budget);
        struct Storage;
        std::unique_ptr<Storage> _storage;
    };

    // Retained actual JKRSolidHeap child. Its final release disposes the
    // original heap and returns the child allocation to the original root.
    // Typed SDK/Game owners must run required destructors before this release.
    class JkrAllocationDomain final {
    public:
        [[nodiscard]] static std::shared_ptr<JkrAllocationDomain>
        create(std::shared_ptr<JkrHeapRuntime> runtime, std::size_t byte_budget);
        [[nodiscard]] static std::shared_ptr<JkrAllocationDomain>
        create(std::shared_ptr<JkrAllocationDomain> parent, std::size_t byte_budget);
        // Bind an actual Game-owned heap. The supplied owner must destroy that
        // heap only after the final retained domain/resource has been released.
        [[nodiscard]] static std::shared_ptr<JkrAllocationDomain>
        retain_heap(std::shared_ptr<JkrHeapRuntime> runtime, JKRHeap& heap,
                    std::shared_ptr<void> heap_owner);
        [[nodiscard]] static std::shared_ptr<JkrAllocationDomain>
        retain_heap(std::shared_ptr<JkrAllocationDomain> parent, JKRHeap& heap);
        ~JkrAllocationDomain();
        JkrAllocationDomain(const JkrAllocationDomain&) = delete;
        JkrAllocationDomain& operator=(const JkrAllocationDomain&) = delete;
        [[nodiscard]] JKRHeap& heap() const noexcept;

    private:
        JkrAllocationDomain(std::shared_ptr<JkrHeapRuntime> runtime, std::size_t byte_budget);
        JkrAllocationDomain(std::shared_ptr<JkrAllocationDomain> parent, std::size_t byte_budget);
        JkrAllocationDomain(std::shared_ptr<JkrHeapRuntime>, JKRHeap&, std::shared_ptr<void>);
        static void register_owner(const std::shared_ptr<JkrAllocationDomain>&);
        struct Storage;
        std::unique_ptr<Storage> _storage;
    };

    // Enter around an original constructor/factory/method that allocates.
    // Retains the domain, selects its original current heap under the original
    // recursive restoration scope, and enables native global-new routing.
    class JkrAllocationScope final {
    public:
        explicit JkrAllocationScope(std::shared_ptr<JkrAllocationDomain> domain);
        ~JkrAllocationScope();
        JkrAllocationScope(const JkrAllocationScope&) = delete;
        JkrAllocationScope& operator=(const JkrAllocationScope&) = delete;

    private:
        struct Storage;
        std::unique_ptr<Storage> _storage;
    };

    // Keep native parsers, registries, caches, loggers and STL control blocks
    // independent from Game heap teardown. A nested JkrAllocationScope may
    // re-enter an explicit original domain while this escape remains alive.
    using JkrHostAllocationScope = aurora::allocation::HostAllocationScope;

    // Observe the domain of the selected original heap within the current
    // Game scope, including inside a host-allocation escape. Returns null
    // outside such a scope. Native retained owners use this to retain the
    // allocations made by original constructors they invoke subsequently.
    [[nodiscard]] std::shared_ptr<JkrAllocationDomain> current_jkr_allocation_domain() noexcept;

} // namespace smgpc::compat
