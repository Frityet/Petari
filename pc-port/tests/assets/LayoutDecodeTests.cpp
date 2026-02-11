#include "assets/AssetServices.hpp"
#include "assets/PackedAsset.hpp"
#include "assets/layout/Yaz0.hpp"
#include "tests/TestHarness.hpp"

#include <cstddef>
#include <vector>

namespace {

[[nodiscard]] std::vector<std::byte> make_bytes(std::initializer_list<unsigned char> values) {
    std::vector<std::byte> bytes {};
    bytes.reserve(values.size());
    for (const auto value : values) {
        bytes.push_back(static_cast<std::byte>(value));
    }
    return bytes;
}

}  // namespace

$test("PackedAsset unpack returns original payload bytes") {
    smgpc::assets::PackedAssetConverter converter {};
    smgpc::assets::LoadedAsset source {
        .id = smgpc::assets::AssetId {.logical_path = "LayoutData/TitleLogo.arc"},
        .source_path = "/tmp/source.arc",
        .bytes = make_bytes({0x01, 0x23, 0x45, 0x67, 0x89}),
    };

    const auto converted = converter.convert(source);
    $pc_port_require(converted);

    const auto unpacked = smgpc::assets::unpack_packed_asset(converted->bytes);
    $pc_port_require(unpacked);
    $pc_port_require_eq(unpacked->size(), source.bytes.size());
    $pc_port_require(*unpacked == source.bytes);
}

$test("Yaz0 decode expands literal stream") {
    const auto encoded = make_bytes({
        0x59, 0x61, 0x7A, 0x30,  // Yaz0
        0x00, 0x00, 0x00, 0x04,  // decompressed size
        0x00, 0x00, 0x00, 0x00,  // reserved
        0x00, 0x00, 0x00, 0x00,  // reserved
        0xF0,                    // four literals
        0x41, 0x42, 0x43, 0x44,  // A B C D
    });

    const auto decoded = smgpc::assets::layout::decode_yaz0(encoded);
    $pc_port_require(decoded);
    $pc_port_require_eq(decoded->size(), static_cast<std::size_t>(4));
    $pc_port_require((*decoded)[0] == static_cast<std::byte>(0x41));
    $pc_port_require((*decoded)[1] == static_cast<std::byte>(0x42));
    $pc_port_require((*decoded)[2] == static_cast<std::byte>(0x43));
    $pc_port_require((*decoded)[3] == static_cast<std::byte>(0x44));
}
