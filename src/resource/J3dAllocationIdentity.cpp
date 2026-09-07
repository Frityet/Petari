#include "J3dAllocationIdentity.hpp"
#include "compat/JkrAllocationDomain.hpp"

#include <iterator>
#include <map>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace smgpc::resource {
    struct J3dAllocationIdentity::State {
        std::mutex mutex;
        // Original cached addresses have their top two bits set to10. Shifting
        // by four, as the model loader does, leaves both material state bits
        // clear. The unshifted material-table loader retains its original bits.
        std::map<std::uint32_t, std::uint32_t> free{{0x80000000U, 0x40000000U}};
        std::map<std::uint32_t, std::uint32_t> allocated;

        std::uint32_t allocate(std::uint32_t size) {
            std::lock_guard lock(mutex);
            for (auto it = free.begin(); it != free.end(); ++it) {
                if (it->second < size) continue;
                const auto base = it->first;
                const auto remainder = it->second - size;
                allocated.emplace(base, size);
                try {
                    if (remainder != 0) free.emplace(base + size, remainder);
                } catch (...) {
                    allocated.erase(base);
                    throw;
                }
                free.erase(it);
                return base;
            }
            throw std::length_error("Original J3D allocation identity space is exhausted");
        }

        void release(std::uint32_t base, std::uint32_t size) {
            std::lock_guard lock(mutex);
            // Reuse the reservation's existing map node: retirement must not
            // allocate memory while unwinding a failed resource construction.
            auto node = allocated.extract(base);
            auto next = free.lower_bound(base);
            if (next != free.begin()) {
                auto previous = std::prev(next);
                if (previous->first + previous->second == base) {
                    base = previous->first;
                    size += previous->second;
                    free.erase(previous);
                }
            }
            if (next != free.end() && base + size == next->first) {
                size += next->second;
                free.erase(next);
            }
            node.key() = base;
            node.mapped() = size;
            free.insert(std::move(node));
        }
    };

    J3dAllocationIdentity::J3dAllocationIdentity(std::size_t original_extent) : _extent(original_extent) {
        compat::JkrHostAllocationScope host_allocations;
        if (original_extent == 0 || original_extent > 0x40000000U) {
            throw std::length_error("J3D allocation identity extent is outside its original address range");
        }
        static auto state = std::make_shared<State>();
        _state = state;
        _reservation = static_cast<std::uint32_t>((original_extent + 31U) & ~std::size_t{31});
        _base = _state->allocate(_reservation);
    }

    J3dAllocationIdentity::~J3dAllocationIdentity() {
        if (_base != 0) _state->release(_base, _reservation);
    }

    J3dAllocationIdentity::J3dAllocationIdentity(J3dAllocationIdentity&& other) noexcept
        : _state(std::move(other._state)), _base(std::exchange(other._base, 0)),
          _extent(std::exchange(other._extent, 0)), _reservation(std::exchange(other._reservation, 0)) {}

    std::uint32_t J3dAllocationIdentity::address(std::size_t original_offset) const {
        if (_base == 0 || original_offset >= _extent) throw std::out_of_range("J3D allocation identity offset is outside its extent");
        return _base + static_cast<std::uint32_t>(original_offset);
    }
}
