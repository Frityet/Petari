#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace smgpc::resource {

    // A native metadata block keeps the SDK's header-relative offset contract.
    // Records have real C++ lifetimes and native alignment; source command bytes
    // can remain unchanged alongside them. No pointer is exposed before finish.
    template <typename Header>
    class J3dNativeBlock final {
        static_assert(std::is_trivially_destructible_v<Header>);
        static constexpr std::size_t storage_alignment = 32;
        static_assert(alignof(Header) <= storage_alignment);

        struct DeleteStorage {
            void operator()(std::byte* pointer) const noexcept {
                ::operator delete[](pointer, std::align_val_t(storage_alignment));
            }
        };

        std::unique_ptr<std::byte[], DeleteStorage> _bytes;
        std::size_t _size;

        explicit J3dNativeBlock(std::size_t size)
            : _bytes(static_cast<std::byte*>(::operator new[](size, std::align_val_t(storage_alignment)))),
              _size(size) {
            std::fill_n(_bytes.get(), size, std::byte{});
        }

    public:
        J3dNativeBlock(const J3dNativeBlock&) = delete;
        J3dNativeBlock& operator=(const J3dNativeBlock&) = delete;

        [[nodiscard]] Header& header() noexcept {
            return *std::launder(reinterpret_cast<Header*>(_bytes.get()));
        }
        [[nodiscard]] const Header& header() const noexcept {
            return *std::launder(reinterpret_cast<const Header*>(_bytes.get()));
        }
        [[nodiscard]] std::span<std::byte> bytes() noexcept { return {_bytes.get(), _size}; }
        [[nodiscard]] std::span<const std::byte> bytes() const noexcept { return {_bytes.get(), _size}; }

        class Builder {
            struct Part {
                std::size_t offset;
                explicit Part(std::size_t value) : offset(value) {}
                virtual ~Part() = default;
                virtual void construct(std::byte*) const = 0;
            };
            template <typename T>
            struct Array final : Part {
                std::vector<T> values;
                Array(std::size_t offset, std::span<const T> source) : Part(offset), values(source.begin(), source.end()) {}
                void construct(std::byte* storage) const override {
                    for (std::size_t i = 0; i < values.size(); ++i) {
                        std::construct_at(reinterpret_cast<T*>(storage + this->offset + i * sizeof(T)), values[i]);
                    }
                }
            };
            struct SourceRange {
                std::size_t source;
                std::size_t size;
                std::size_t native;
            };
            std::size_t _size = sizeof(Header);
            std::vector<std::unique_ptr<Part>> _parts;
            std::vector<SourceRange> _sources;
            bool _finished = false;

            void require_open() const {
                if (_finished) throw std::logic_error("J3D native block builder has already been finalized");
            }

        public:
            Header header{};

            template <typename T>
            [[nodiscard]] std::size_t append(std::span<const T> values, std::size_t alignment = alignof(T)) {
                static_assert(std::is_trivially_destructible_v<T>, "Metadata records cannot own external resources");
                require_open();
                if (alignment < alignof(T) || alignment > storage_alignment || (alignment & (alignment - 1)) != 0) {
                    throw std::invalid_argument("Unsupported J3D native table alignment");
                }
                constexpr auto limit = static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max());
                if (_size > limit - (alignment - 1)) throw std::length_error("J3D native block is too large");
                const auto offset = (_size + alignment - 1) & ~(alignment - 1);
                if (values.size() > (limit - offset) / sizeof(T)) throw std::length_error("J3D native table is too large");
                auto part = std::make_unique<Array<T>>(offset, values);
                _parts.push_back(std::move(part));
                _size = offset + values.size_bytes();
                return offset;
            }

            [[nodiscard]] std::size_t append_bytes(std::span<const std::uint8_t> bytes, std::size_t alignment = 1) {
                return append<std::uint8_t>(bytes, alignment);
            }

            // Resolve relative references after the destination tables have
            // offsets. The array extent stays fixed until final construction.
            template <typename T>
            [[nodiscard]] std::span<T> edit_array(std::size_t offset) {
                require_open();
                for (const auto& part : _parts) {
                    if (part->offset == offset) {
                        if (auto* array = dynamic_cast<Array<T>*>(part.get())) return array->values;
                    }
                }
                throw std::invalid_argument("J3D native offset does not identify the requested table type");
            }

            // Register only spans whose source/native byte strides agree. A
            // widened header needs individual field relocation, not this map.
            void map_source_range(std::size_t source, std::size_t size, std::size_t native) {
                require_open();
                if (source > std::numeric_limits<std::size_t>::max() - size || native > _size || size > _size - native) {
                    throw std::out_of_range("J3D native source mapping exceeds its allocation");
                }
                for (const auto& previous : _sources) {
                    const auto overlap = std::max(source, previous.source);
                    if (overlap < source + size && overlap < previous.source + previous.size &&
                        native + (overlap - source) != previous.native + (overlap - previous.source)) {
                        throw std::invalid_argument("Overlapping J3D source ranges require a shared native representation");
                    }
                }
                _sources.push_back({source, size, native});
            }

            [[nodiscard]] std::optional<std::size_t> find_source_offset(std::size_t source, std::size_t size = 1) const {
                for (const auto& range : _sources) {
                    if (source >= range.source && source - range.source <= range.size && size <= range.size - (source - range.source)) {
                        return range.native + (source - range.source);
                    }
                }
                return std::nullopt;
            }

            [[nodiscard]] static void* pointer_offset(std::size_t offset) noexcept {
                return reinterpret_cast<void*>(static_cast<std::uintptr_t>(offset));
            }

            [[nodiscard]] std::unique_ptr<J3dNativeBlock> finish() && {
                require_open();
                auto result = std::unique_ptr<J3dNativeBlock>(new J3dNativeBlock(_size));
                std::construct_at(reinterpret_cast<Header*>(result->_bytes.get()), header);
                for (const auto& part : _parts) part->construct(result->_bytes.get());
                _finished = true;
                return result;
            }
        };
    };
}
