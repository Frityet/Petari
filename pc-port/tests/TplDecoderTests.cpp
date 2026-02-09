#include "render/TplDecoder.hpp"
#include "tests/TestCommon.hpp"
#include "tests/TestHarness.hpp"

#include <filesystem>

namespace {

int DecodeDirectoryTplCount(const std::filesystem::path& dir) {
    int decodedCount = 0;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".tpl") {
            continue;
        }

        const pcport::ImageRGBA image = pcport::TplDecoder::DecodeFile(entry.path());
        $pc_port_require(image.width > 0);
        $pc_port_require(image.height > 0);
        $pc_port_require(!image.pixels.empty());
        ++decodedCount;
    }
    return decodedCount;
}

$pc_port_test(TplDecoderDecodesFileSelectTextures) {
    const pcport::PreparedMenuAssets& assets = pcport::test::GetPreparedAssets();
    const int decodedCount = DecodeDirectoryTplCount(assets.fileSelectDir / "timg");
    $pc_port_require(decodedCount > 0);
}

$pc_port_test(TplDecoderDecodesTitleLogoTextures) {
    const pcport::PreparedMenuAssets& assets = pcport::test::GetPreparedAssets();
    const int decodedCount = DecodeDirectoryTplCount(assets.titleLogoDir / "timg");
    $pc_port_require(decodedCount > 0);
}

$pc_port_test(TplDecoderDecodesAllMenuTextures) {
    const pcport::PreparedMenuAssets& assets = pcport::test::GetPreparedAssets();
    const int decodedCount = DecodeDirectoryTplCount(assets.fileSelectDir / "timg") + DecodeDirectoryTplCount(assets.titleLogoDir / "timg");
    $pc_port_require(decodedCount > 0);
}

}  // namespace
