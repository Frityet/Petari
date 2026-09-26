#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

namespace smgpc::resource {
    // Material difference words combine original-width allocation addresses
    // with two state bits. This owns a disjoint, stable original address range;
    // it does not map memory or encode a truncated host pointer.
    class J3dAllocationIdentity final {
    public:
        // Select the retained address range while original SDK code converts
        // an allocation pointer to its Wii address. Nested loads restore the
        // enclosing range, and the range survives this temporary binding.
        class Scope final {
        public:
            explicit Scope(J3dAllocationIdentity&);
            ~Scope();
            Scope(const Scope&) = delete;
            Scope& operator=(const Scope&) = delete;
        private:
            J3dAllocationIdentity* _previous;
        };

        explicit J3dAllocationIdentity(std::size_t original_extent);
        ~J3dAllocationIdentity();
        J3dAllocationIdentity(J3dAllocationIdentity&&) noexcept;
        J3dAllocationIdentity& operator=(J3dAllocationIdentity&&) = delete;
        J3dAllocationIdentity(const J3dAllocationIdentity&) = delete;
        J3dAllocationIdentity& operator=(const J3dAllocationIdentity&) = delete;

        [[nodiscard]] std::uint32_t address(std::size_t original_offset = 0) const;
        [[nodiscard]] static std::uint32_t original_address(const void* allocation, std::size_t original_offset = 0);

    private:
        struct State;
        std::shared_ptr<State> _state;
        std::uint32_t _base = 0;
        std::size_t _extent = 0;
        std::uint32_t _reservation = 0;
        const void* _native_allocation = nullptr;
    };
}
