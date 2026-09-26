#include "BinaryChunkFile.hpp"

#include <aurora/endian.hpp>
#include <limits>

namespace smgpc::common {

    bool has_bounded_binary_chunks(std::span<const std::uint8_t> bytes) {
        if (bytes.size() < 4 || bytes.size() > std::numeric_limits<std::int32_t>::max()) {
            return false;
        }

        std::size_t offset = 4;
        for (unsigned index = 0; index < bytes[1]; ++index) {
            if (bytes.size() - offset < 12) {
                return false;
            }
            const auto size = aurora::endian::read_u32(bytes.data() + offset + 8);
            if (size < 12 || size > bytes.size() - offset) {
                return false;
            }
            offset += size;
        }
        return true;
    }

}  // namespace smgpc::common
