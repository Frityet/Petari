#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>

namespace smgpc::resource {

    // Explicit process-startup owner. OSInit must have completed before create.
    // The caller chooses one budget and retains/passes this shared heap to all
    // mapped resources. It never replaces an existing OS allocator.
    class Mem1ResourceHeap final : public std::enable_shared_from_this<Mem1ResourceHeap> {
    public:
        class Allocation final {
        public:
            Allocation() noexcept = default;
            ~Allocation();
            Allocation(Allocation&&) noexcept;
            Allocation& operator=(Allocation&&) noexcept;
            Allocation(const Allocation&) = delete;
            Allocation& operator=(const Allocation&) = delete;
            [[nodiscard]] std::span<std::byte> bytes() noexcept { return {_data, _size}; }
            [[nodiscard]] std::span<const std::byte> bytes() const noexcept { return {_data, _size}; }

        private:
            friend class Mem1ResourceHeap;
            Allocation(std::shared_ptr<Mem1ResourceHeap>, std::byte*, std::size_t);
            void reset() noexcept;
            std::shared_ptr<Mem1ResourceHeap> _heap;
            std::byte* _data = nullptr;
            std::size_t _size = 0;
        };

        [[nodiscard]] static std::shared_ptr<Mem1ResourceHeap> create(std::size_t byte_budget);
        ~Mem1ResourceHeap();
        Mem1ResourceHeap(const Mem1ResourceHeap&) = delete;
        Mem1ResourceHeap& operator=(const Mem1ResourceHeap&) = delete;

        [[nodiscard]] Allocation allocate(std::size_t size);
        [[nodiscard]] std::size_t available_bytes() const;
        [[nodiscard]] std::size_t reserved_bytes() const noexcept { return _reserved; }

    private:
        explicit Mem1ResourceHeap(std::size_t byte_budget);
        void release(void*) noexcept;
        mutable std::mutex _mutex;
        int _handle = -1;
        std::size_t _reserved = 0;
    };
}
