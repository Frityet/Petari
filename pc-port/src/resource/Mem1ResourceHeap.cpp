#include "Mem1ResourceHeap.hpp"

#include <dolphin/os.h>
#include <dolphin/gx/GXAurora.h>

#include <limits>
#include <stdexcept>
#include <utility>

namespace smgpc::resource {
    namespace {
        std::mutex startup_mutex;
    }

    std::shared_ptr<Mem1ResourceHeap> Mem1ResourceHeap::create(std::size_t byte_budget) {
        return std::shared_ptr<Mem1ResourceHeap>(new Mem1ResourceHeap(byte_budget));
    }

    Mem1ResourceHeap::Mem1ResourceHeap(std::size_t byte_budget) {
        std::lock_guard lock(startup_mutex);
        if (AuroraOSIsAllocatorInitialized()) {
            throw std::logic_error("Mapped resource heap cannot replace an initialized OS allocator");
        }
        if (byte_budget < 128 || byte_budget > std::numeric_limits<std::int32_t>::max() - 31U) {
            throw std::invalid_argument("Mapped resource heap budget is outside the OS heap range");
        }
        const auto low = reinterpret_cast<std::uintptr_t>(OSGetArenaLo());
        const auto high = reinterpret_cast<std::uintptr_t>(OSGetArenaHi());
        const auto aligned_high = high & ~std::uintptr_t{31};
        _reserved = (byte_budget + 31U) & ~std::size_t{31};
        if (OSBaseAddress == 0 || low < OSBaseAddress || high < low ||
            high - OSBaseAddress > OSGetPhysicalMemSize()) {
            throw std::logic_error("Mapped resource heap requires completed OSInit and a valid MEM1 arena");
        }
        if (aligned_high < low || _reserved > aligned_high - low) {
            throw std::bad_alloc();
        }
        auto* region = OSAllocFromArenaHi(static_cast<u32>(_reserved), 32);
        auto* end = reinterpret_cast<void*>(aligned_high);
        auto* heap_start = OSInitAlloc(region, end, 1);
        if (heap_start == nullptr || (_handle = OSCreateHeap(heap_start, end)) < 0) {
            throw std::runtime_error("Failed to initialize the reserved mapped resource heap");
        }
    }

    Mem1ResourceHeap::~Mem1ResourceHeap() {
        OSDestroyHeap(_handle);
        // OSInitAlloc's descriptor table still names this reservation. Its
        // lifetime is the process's OS memory lifetime, not an arena rollback.
    }

    Mem1ResourceHeap::Allocation Mem1ResourceHeap::allocate(std::size_t size) {
        if (size == 0 || size > std::numeric_limits<std::int32_t>::max() - 63U) {
            throw std::length_error("Mapped resource allocation exceeds the OS heap range");
        }
        auto owner = shared_from_this();
        std::lock_guard lock(_mutex);
        auto* memory = static_cast<std::byte*>(OSAllocFromHeap(_handle, static_cast<u32>(size)));
        if (memory == nullptr) throw std::bad_alloc();
        return Allocation(std::move(owner), memory, size);
    }

    void Mem1ResourceHeap::release(void* memory) noexcept {
        // Command bytes do not retain source objects. Finish queued CPU reads
        // before returning their mapped storage to the heap for reuse.
        AuroraDrainGXCommands();
        std::lock_guard lock(_mutex);
        OSFreeToHeap(_handle, memory);
    }

    std::size_t Mem1ResourceHeap::available_bytes() const {
        std::lock_guard lock(_mutex);
        const auto bytes = OSCheckHeap(_handle);
        if (bytes < 0) throw std::runtime_error("Mapped resource heap is invalid");
        return static_cast<std::size_t>(bytes);
    }

    Mem1ResourceHeap::Allocation::Allocation(std::shared_ptr<Mem1ResourceHeap> heap, std::byte* data, std::size_t size)
        : _heap(std::move(heap)), _data(data), _size(size) {}
    Mem1ResourceHeap::Allocation::~Allocation() { reset(); }
    Mem1ResourceHeap::Allocation::Allocation(Allocation&& other) noexcept
        : _heap(std::move(other._heap)), _data(std::exchange(other._data, nullptr)), _size(std::exchange(other._size, 0)) {}
    Mem1ResourceHeap::Allocation& Mem1ResourceHeap::Allocation::operator=(Allocation&& other) noexcept {
        if (this != &other) {
            reset();
            _heap = std::move(other._heap);
            _data = std::exchange(other._data, nullptr);
            _size = std::exchange(other._size, 0);
        }
        return *this;
    }
    void Mem1ResourceHeap::Allocation::reset() noexcept {
        if (_data != nullptr) _heap->release(_data);
        _data = nullptr;
        _size = 0;
        _heap.reset();
    }
}
