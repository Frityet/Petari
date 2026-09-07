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
        explicit J3dAllocationIdentity(std::size_t original_extent);
        ~J3dAllocationIdentity();
        J3dAllocationIdentity(J3dAllocationIdentity&&) noexcept;
        J3dAllocationIdentity& operator=(J3dAllocationIdentity&&) = delete;
        J3dAllocationIdentity(const J3dAllocationIdentity&) = delete;
        J3dAllocationIdentity& operator=(const J3dAllocationIdentity&) = delete;

        [[nodiscard]] std::uint32_t address(std::size_t original_offset = 0) const;

    private:
        struct State;
        std::shared_ptr<State> _state;
        std::uint32_t _base = 0;
        std::size_t _extent = 0;
        std::uint32_t _reservation = 0;
    };
}
