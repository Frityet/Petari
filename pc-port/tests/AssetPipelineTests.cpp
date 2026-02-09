#include "tests/TestCommon.hpp"
#include "tests/TestHarness.hpp"

#include <filesystem>

namespace {

$pc_port_test(AssetPipelineRootExists) {
    const pcport::PreparedMenuAssets& assets = pcport::test::GetPreparedAssets();
    $pc_port_require(std::filesystem::exists(assets.root));
}

$pc_port_test(AssetPipelinePressStartLayoutExists) {
    const pcport::PreparedMenuAssets& assets = pcport::test::GetPreparedAssets();
    $pc_port_require(std::filesystem::exists(assets.pressStartDir / "blyt" / "pressstart.brlyt"));
}

$pc_port_test(AssetPipelineFileSelectLayoutExists) {
    const pcport::PreparedMenuAssets& assets = pcport::test::GetPreparedAssets();
    $pc_port_require(std::filesystem::exists(assets.fileSelectDir / "blyt" / "fileselect.brlyt"));
}

$pc_port_test(AssetPipelineTitleLogoLayoutExists) {
    const pcport::PreparedMenuAssets& assets = pcport::test::GetPreparedAssets();
    $pc_port_require(std::filesystem::exists(assets.titleLogoDir / "blyt" / "titlelogo.brlyt"));
}

$pc_port_test(AssetPipelinePressStartAnimationsExist) {
    const pcport::PreparedMenuAssets& assets = pcport::test::GetPreparedAssets();
    $pc_port_require(std::filesystem::exists(assets.pressStartDir / "anim" / "appear.brlan"));
}

$pc_port_test(AssetPipelineFileSelectAnimationsExist) {
    const pcport::PreparedMenuAssets& assets = pcport::test::GetPreparedAssets();
    $pc_port_require(std::filesystem::exists(assets.fileSelectDir / "anim" / "buttonwait.brlan"));
}

$pc_port_test(AssetPipelineTitleLogoAnimationsExist) {
    const pcport::PreparedMenuAssets& assets = pcport::test::GetPreparedAssets();
    $pc_port_require(std::filesystem::exists(assets.titleLogoDir / "anim" / "wait.brlan"));
}

}  // namespace
