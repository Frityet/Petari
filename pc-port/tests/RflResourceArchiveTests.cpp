#include "resource/RarcArchive.hpp"

#include <aurora/rfl/ResourceArchive.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void writeBe16(std::span<std::uint8_t> bytes, std::size_t offset, std::uint16_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value);
    }

    void writeBe32(std::span<std::uint8_t> bytes, std::size_t offset, std::uint32_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 24U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 16U);
        bytes[offset + 2U] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 3U] = static_cast<std::uint8_t>(value);
    }

    [[nodiscard]] std::vector<std::uint8_t> makeMinimalArchive() {
        constexpr auto header_size = std::size_t{4U} +
                                     aurora::rfl::ResourceArchive::archive_count * sizeof(std::uint32_t);
        constexpr auto section_size = std::size_t{13U};
        auto bytes = std::vector<std::uint8_t>(
            header_size + aurora::rfl::ResourceArchive::archive_count * section_size);
        writeBe16(bytes, 0U, static_cast<std::uint16_t>(aurora::rfl::ResourceArchive::archive_count));
        writeBe16(bytes, 2U, 1U);

        auto section_offset = header_size;
        for (auto index = std::size_t{}; index < aurora::rfl::ResourceArchive::archive_count; ++index) {
            writeBe32(bytes, 4U + index * sizeof(std::uint32_t), static_cast<std::uint32_t>(section_offset));
            writeBe16(bytes, section_offset, 1U);
            writeBe16(bytes, section_offset + 2U, 1U);
            writeBe32(bytes, section_offset + 4U, 0U);
            writeBe32(bytes, section_offset + 8U, 1U);
            bytes[section_offset + 12U] = static_cast<std::uint8_t>(index);
            section_offset += section_size;
        }
        return bytes;
    }

    void requireError(std::span<const std::uint8_t> bytes, aurora::rfl::ResourceArchiveError expected,
                      std::string_view message) {
        const auto parsed = aurora::rfl::ResourceArchive::copy_from(bytes);
        require(!parsed.valid() && parsed.error() == expected, message);
    }

    void testFormatValidation() {
        constexpr auto header_size = std::size_t{4U} +
                                     aurora::rfl::ResourceArchive::archive_count * sizeof(std::uint32_t);
        constexpr auto section_size = std::size_t{13U};
        const auto valid_bytes = makeMinimalArchive();
        const auto valid = aurora::rfl::ResourceArchive::copy_from(valid_bytes);
        require(valid.valid() && valid.version() == 1U,
                "the minimal structurally valid RFL resource must parse");
        const auto first_file = valid.file(aurora::rfl::ResourceArchiveId::Beard, 0U);
        require(first_file.has_value() && first_file->size() == 1U && (*first_file)[0] == 0U,
                "the minimal resource must expose its owned file payload");

        const auto too_small_bytes = std::array<std::uint8_t, 4U>{1U, 2U, 3U, 4U};
        requireError(too_small_bytes, aurora::rfl::ResourceArchiveError::HeaderTooSmall,
                     "a truncated RFL resource must retain its precise parse error");

        auto wrong_archive_count = valid_bytes;
        writeBe16(wrong_archive_count, 0U, 17U);
        requireError(wrong_archive_count, aurora::rfl::ResourceArchiveError::WrongArchiveCount,
                     "the parser must reject a non-retail archive count");

        auto missing_version = valid_bytes;
        writeBe16(missing_version, 2U, 0U);
        requireError(missing_version, aurora::rfl::ResourceArchiveError::MissingVersion,
                     "the parser must reject a missing resource version");

        auto invalid_section_offset = valid_bytes;
        writeBe32(invalid_section_offset, 4U, static_cast<std::uint32_t>(header_size - 1U));
        requireError(invalid_section_offset, aurora::rfl::ResourceArchiveError::SectionOffsetOutOfRange,
                     "the parser must reject a section overlapping the header");

        auto empty_section = valid_bytes;
        writeBe16(empty_section, header_size, 0U);
        requireError(empty_section, aurora::rfl::ResourceArchiveError::EmptySection,
                     "the parser must reject an empty subarchive");

        auto invalid_table = valid_bytes;
        const auto last_section = header_size +
                                  (aurora::rfl::ResourceArchive::archive_count - 1U) * section_size;
        writeBe16(invalid_table, last_section, 0xFFFFU);
        requireError(invalid_table, aurora::rfl::ResourceArchiveError::OffsetTableOutOfRange,
                     "the parser must reject an offset table extending beyond the resource");

        auto nonzero_first_offset = valid_bytes;
        writeBe32(nonzero_first_offset, header_size + 4U, 1U);
        requireError(nonzero_first_offset, aurora::rfl::ResourceArchiveError::NonZeroFirstFileOffset,
                     "the parser must reject a nonzero first file offset");

        auto nonmonotonic_offsets = valid_bytes;
        writeBe16(nonmonotonic_offsets, header_size, 2U);
        writeBe32(nonmonotonic_offsets, header_size + 8U, 2U);
        writeBe32(nonmonotonic_offsets, header_size + 12U, 1U);
        requireError(nonmonotonic_offsets, aurora::rfl::ResourceArchiveError::NonMonotonicFileOffsets,
                     "the parser must reject decreasing file offsets");

        auto wrong_largest_file = valid_bytes;
        writeBe16(wrong_largest_file, header_size + 2U, 2U);
        requireError(wrong_largest_file, aurora::rfl::ResourceArchiveError::IncorrectLargestFileSize,
                     "the parser must verify the advertised largest file size");

        auto data_out_of_range = valid_bytes;
        writeBe16(data_out_of_range, last_section + 2U, 2U);
        writeBe32(data_out_of_range, last_section + 8U, 2U);
        requireError(data_out_of_range, aurora::rfl::ResourceArchiveError::FileDataOutOfRange,
                     "the parser must reject file data extending past the resource");
    }

    [[nodiscard]] std::optional<std::filesystem::path> findRealMiiFaceArchive() {
        for (auto root = std::filesystem::current_path(); !root.empty(); root = root.parent_path()) {
            const std::filesystem::path candidates[]{
                root / "orig/RMGK02/files/ObjectData/MiiFaceDatabase.arc",
                root / "container/orig/RMGK01/files/ObjectData/MiiFaceDatabase.arc",
                root / "pc-port/container/orig/RMGK01/files/ObjectData/MiiFaceDatabase.arc",
            };
            for (const auto &candidate : candidates) {
                auto error = std::error_code{};
                if (std::filesystem::is_regular_file(candidate, error)) {
                    return candidate;
                }
            }
            if (root == root.root_path()) {
                break;
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] std::uint64_t fnv1a(std::span<const std::uint8_t> bytes) {
        auto hash = std::uint64_t{14695981039346656037ULL};
        for (const auto byte : bytes) {
            hash ^= byte;
            hash *= 1099511628211ULL;
        }
        return hash;
    }

}  // namespace

int main() {
    try {
        testFormatValidation();

        const auto archive_path = findRealMiiFaceArchive();
        if (!archive_path.has_value()) {
            std::cout << "[skip] real MiiFaceDatabase.arc resource check\n";
            return 0;
        }

        const auto rarc = smgpc::resource::RarcArchive::from_file(*archive_path);
        const auto resource = rarc.resource_data("/RFL_Res.dat");
        require(!resource.empty(), "the retail Mii face archive must contain RFL_Res.dat");

        const auto parsed = aurora::rfl::ResourceArchive::copy_from(resource);
        require(parsed.valid() && parsed.version() != 0U && parsed.bytes().size() == resource.size(),
                "the generalized parser must retain a valid owned retail RFL resource");
        require(parsed.bytes().data() != resource.data(),
                "the parsed archive must not borrow its caller's resource lifetime");

        auto total_files = std::size_t{};
        for (auto archive_index = std::size_t{};
             archive_index < aurora::rfl::ResourceArchive::archive_count; ++archive_index) {
            const auto archive_id = static_cast<aurora::rfl::ResourceArchiveId>(archive_index);
            const auto &section = parsed.section(archive_id);
            const auto first_file = parsed.file(archive_id, 0U);
            require(section.file_count != 0U && section.largest_file_size != 0U &&
                        first_file.has_value() && !first_file->empty(),
                    "every retail RFL subarchive must expose its first real file");
            require(!parsed.file(archive_id, section.file_count).has_value(),
                    "an out-of-range retail RFL file index must remain absent");
            total_files += section.file_count;
            std::cout << "archive[" << archive_index << "] files=" << section.file_count
                      << " largest=" << section.largest_file_size << " first=" << first_file->size() << '\n';
        }

        std::cout << "RFL resource archive passed: version=" << parsed.version()
                  << " bytes=" << parsed.bytes().size() << " files=" << total_files
                  << " fnv1a=0x" << std::hex << fnv1a(parsed.bytes()) << std::dec << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "RFL resource archive test failed: " << error.what() << '\n';
        return 1;
    }
}
