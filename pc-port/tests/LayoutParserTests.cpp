#include "layout/BrlytRuntime.hpp"
#include "tests/TestCommon.hpp"
#include "tests/TestHarness.hpp"

namespace {

$pc_port_test(LayoutParserPressStartLoads) {
    const pcport::PreparedMenuAssets& assets = pcport::test::GetPreparedAssets();
    const pcport::BrlytLayout pressStart = pcport::BrlytLayout::LoadFromDirectory(assets.pressStartDir);
    $pc_port_require(!pressStart.GetPanes().empty());
}

$pc_port_test(LayoutParserTitleLogoLoads) {
    const pcport::PreparedMenuAssets& assets = pcport::test::GetPreparedAssets();
    const pcport::BrlytLayout titleLogo = pcport::BrlytLayout::LoadFromDirectory(assets.titleLogoDir);
    $pc_port_require(!titleLogo.GetPanes().empty());
}

$pc_port_test(LayoutParserFileSelectLoads) {
    const pcport::PreparedMenuAssets& assets = pcport::test::GetPreparedAssets();
    const pcport::BrlytLayout fileSelect = pcport::BrlytLayout::LoadFromDirectory(assets.fileSelectDir);
    $pc_port_require(!fileSelect.GetPanes().empty());
}

$pc_port_test(LayoutParserPressStartRequiredPanes) {
    const pcport::PreparedMenuAssets& assets = pcport::test::GetPreparedAssets();
    const pcport::BrlytLayout pressStart = pcport::BrlytLayout::LoadFromDirectory(assets.pressStartDir);
    $pc_port_require(pressStart.FindPane("PressAB") != nullptr);
    $pc_port_require(pressStart.FindPane("TxtStart") != nullptr);
}

$pc_port_test(LayoutParserTitleLogoRequiredPanes) {
    const pcport::PreparedMenuAssets& assets = pcport::test::GetPreparedAssets();
    const pcport::BrlytLayout titleLogo = pcport::BrlytLayout::LoadFromDirectory(assets.titleLogoDir);
    $pc_port_require(titleLogo.FindPane("SMGTitleLogo") != nullptr);
    $pc_port_require(titleLogo.FindPane("PicLogoGalaxy") != nullptr);
}

$pc_port_test(LayoutParserFileSelectRequiredPanes) {
    const pcport::PreparedMenuAssets& assets = pcport::test::GetPreparedAssets();
    const pcport::BrlytLayout fileSelect = pcport::BrlytLayout::LoadFromDirectory(assets.fileSelectDir);
    $pc_port_require(fileSelect.FindPane("SButton") != nullptr);
    $pc_port_require(fileSelect.FindPane("TxtStart") != nullptr);
}

}  // namespace
