#include "game/layout/LayoutArchiveLoader.hpp"

#include "tests/TestHarness.hpp"

#include <string>

$test("LayoutArchiveLoader font lookup keys fall back from Korean BRFNT suffix") {
    const auto keys = smgpc::game::layout::LayoutArchiveLoader::make_font_name_lookup_keys("MessageFont26kor.brfnt");

    $pc_port_require(keys.size() >= 2U);
    $pc_port_require_eq(keys[0U], std::string("messagefont26kor"));
    $pc_port_require_eq(keys[1U], std::string("messagefont26"));
}

$test("LayoutArchiveLoader applies Korean localized pane visibility") {
    smgpc::assets::layout::LayoutDefinition layout {};
    layout.panes.push_back(smgpc::assets::layout::PaneDefinition {.name = "TxtPro01", .visible = true});
    layout.panes.push_back(smgpc::assets::layout::PaneDefinition {.name = "TxtPro01JpJa", .visible = true});
    layout.panes.push_back(smgpc::assets::layout::PaneDefinition {.name = "TxtPro01KrKo", .visible = true});
    layout.panes.push_back(smgpc::assets::layout::PaneDefinition {.name = "TxtPro01CnSi", .visible = true});

    smgpc::game::layout::LayoutArchiveLoader::apply_language_pane_visibility("KrKorean", &layout);

    $pc_port_require(!layout.panes[0U].visible);
    $pc_port_require(!layout.panes[1U].visible);
    $pc_port_require(layout.panes[2U].visible);
    $pc_port_require(!layout.panes[3U].visible);
}

$test("LayoutArchiveLoader keeps base pane visible when selected localization is absent") {
    smgpc::assets::layout::LayoutDefinition layout {};
    layout.panes.push_back(smgpc::assets::layout::PaneDefinition {.name = "TxtPro01", .visible = true});
    layout.panes.push_back(smgpc::assets::layout::PaneDefinition {.name = "TxtPro01JpJa", .visible = true});
    layout.panes.push_back(smgpc::assets::layout::PaneDefinition {.name = "TxtPro01CnSi", .visible = true});

    smgpc::game::layout::LayoutArchiveLoader::apply_language_pane_visibility("KrKorean", &layout);

    $pc_port_require(layout.panes[0U].visible);
    $pc_port_require(!layout.panes[1U].visible);
    $pc_port_require(!layout.panes[2U].visible);
}

$test("LayoutArchiveLoader font lookup keys fall back from simplified Chinese BRFNT suffix") {
    const auto keys = smgpc::game::layout::LayoutArchiveLoader::make_font_name_lookup_keys("MessageFont26sch.brfnt");

    $pc_port_require(keys.size() >= 2U);
    $pc_port_require_eq(keys[0U], std::string("messagefont26sch"));
    $pc_port_require_eq(keys[1U], std::string("messagefont26"));
}
