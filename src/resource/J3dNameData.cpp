#include "J3dNameData.hpp"

#include "JSystem/JUtility/JUTNameTab.hpp"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <stdexcept>

namespace smgpc::resource {

    struct J3dNameData::Storage {
        std::unique_ptr<std::byte[]> bytes;
        std::size_t size;
        std::unique_ptr<JUTNameTab> table;

        explicit Storage(std::span<const std::uint8_t> source)
            : size(std::max(source.size(), sizeof(ResNTAB))) {
            if (source.size() < 4) {
                throw std::runtime_error("J3D name table header is truncated");
            }
            const auto read16 = [&](std::size_t offset) {
                return static_cast<std::uint16_t>((std::uint16_t{source[offset]} << 8U) | source[offset + 1]);
            };
            const auto count = read16(0);
            if (count * 4U > source.size() - 4) {
                throw std::runtime_error("J3D name records exceed their containing block");
            }
            for (std::size_t i = 0; i < count; ++i) {
                const auto offset = read16(6 + i * 4);
                if (offset >= source.size() || std::find(source.begin() + offset, source.end(), 0) == source.end()) {
                    throw std::runtime_error("J3D name is not terminated inside its containing block");
                }
            }
            bytes = std::make_unique<std::byte[]>(size);
            std::memcpy(bytes.get(), source.data(), source.size());
            for (std::size_t i = 0; i < 2U + count * 2U; ++i) {
                const auto value = read16(i * 2);
                std::memcpy(bytes.get() + i * 2, &value, sizeof(value));
            }
            table = std::make_unique<JUTNameTab>(reinterpret_cast<const ResNTAB*>(bytes.get()));
        }
    };

    J3dNameData::J3dNameData() noexcept = default;
    J3dNameData::J3dNameData(std::span<const std::uint8_t> bytes) : _storage(std::make_unique<Storage>(bytes)) {}
    J3dNameData::~J3dNameData() = default;
    J3dNameData::J3dNameData(J3dNameData&&) noexcept = default;
    J3dNameData& J3dNameData::operator=(J3dNameData&&) noexcept = default;

    const ResNTAB* J3dNameData::resource() const noexcept {
        return _storage ? _storage->table->mResource : nullptr;
    }

    JUTNameTab* J3dNameData::table() const noexcept {
        return _storage ? _storage->table.get() : nullptr;
    }

    std::span<const std::uint8_t> J3dNameData::bytes() const noexcept {
        if (!_storage) {
            return {};
        }
        return {reinterpret_cast<const std::uint8_t*>(_storage->bytes.get()), _storage->size};
    }

}  // namespace smgpc::resource
