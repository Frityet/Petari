#include "Game/Screen/LayoutActor.hpp"
#include "Game/Screen/LayoutManager.hpp"
#include "layout/LayoutHost.hpp"
#include "layout/LayoutResourceResolver.hpp"
#include "layout/LayoutRuntime.hpp"

#include <cmath>
#include <filesystem>
#include <functional>
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

void require_unavailable(const std::function<void()>& operation, std::string_view message) {
    auto unavailable = false;
    try {
        operation();
    } catch (const std::exception&) {
        unavailable = true;
    }
    require(unavailable, message);
}

[[nodiscard]] std::optional<std::filesystem::path> find_sys_info_window_mini_archive() {
    for (auto root = std::filesystem::current_path(); !root.empty(); root = root.parent_path()) {
        const std::filesystem::path candidates[]{
            root.parent_path() / "orig/RMGK02/files/LayoutData/SysInfoWindowMini.arc",
            root.parent_path() / "orig/RMGK01/files/LayoutData/SysInfoWindowMini.arc",
            root / "orig/RMGK02/files/LayoutData/SysInfoWindowMini.arc",
            root / "orig/RMGK01/files/LayoutData/SysInfoWindowMini.arc",
            root / "container/orig/RMGK02/files/LayoutData/SysInfoWindowMini.arc",
            root / "container/orig/RMGK01/files/LayoutData/SysInfoWindowMini.arc",
            root / "pc-port/container/orig/RMGK02/files/LayoutData/SysInfoWindowMini.arc",
            root / "pc-port/container/orig/RMGK01/files/LayoutData/SysInfoWindowMini.arc",
        };
        for (const auto& candidate : candidates) {
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
}  // namespace

int main() {
    auto passed = 0;

    auto entries = std::vector<smgpc::resource::RarcEntry>{
        {.path = "blyt/unrelated.brlyt"},
        {.path = "anim/notwait.brlan"},
        {.path = "blyt/titlelogo.brlyt"},
        {.path = "anim/wait.brlan"},
    };
    require(smgpc::layout::find_layout_brlyt(entries, "TitleLogo") == &entries[2],
            "BRLYT resolution must select the resource semantically named by the requested layout");
    require(smgpc::layout::find_layout_brlan(entries, "Wait") == &entries[3],
            "BRLAN resolution must select the exact animation path, not a suffix or first entry");
    require(smgpc::layout::find_layout_brlyt(entries, "Missing") == nullptr &&
                smgpc::layout::find_layout_brlan(entries, "missing") == nullptr,
            "missing exact resources must remain absent");
    require(smgpc::layout::find_layout_brlyt(entries, "../titlelogo") == nullptr,
            "path-like layout names must not broaden archive lookup");
    auto duplicate_entries = entries;
    duplicate_entries.push_back({.path = "blyt/titlelogo.brlyt"});
    require_unavailable([&] { (void)smgpc::layout::find_layout_brlyt(duplicate_entries, "TitleLogo"); },
                        "duplicate exact layout resources must fail instead of choosing one arbitrarily");
    ++passed;

    auto runtime = smgpc::layout::LayoutRuntime("missing-layout-host", "DefinitelyMissingLayout", 1U, 0);
    require(!runtime.getArchivePath().has_value() && !runtime.hasPane("") && !runtime.hasPane("InfoWindow") &&
                !runtime.paneIndex("").has_value() && !runtime.paneIndex("InfoWindow").has_value(),
            "a runtime without a real archive must expose no root or named panes");
    require(!runtime.isPaneVisible("") && !runtime.isPaneVisible("InfoWindow") &&
                !runtime.paneBounds("").has_value() && !runtime.paneScale("").has_value(),
            "an absent BRLYT must not manufacture visibility, bounds, or scale");

    Mtx matrix{};
    for (auto& row : matrix) {
        for (auto& value : row) {
            value = 37.0F;
        }
    }
    require(!runtime.copyPaneMatrix("", matrix) && !runtime.copyPaneMatrix("InfoWindow", matrix),
            "an absent root or named pane must not manufacture a matrix");
    for (const auto& row : matrix) {
        for (const auto value : row) {
            require(value == 37.0F, "a failed matrix lookup must preserve caller storage");
        }
    }
    require_unavailable([&] { runtime.setPaneVisible("InfoWindow", true); },
                        "mutating an absent pane must fail explicitly");
    require_unavailable([&] { runtime.startPaneAnim("InfoWindow", "Wait", 0U); },
                        "an absent pane must not acquire synthetic animation state");
    require_unavailable([&] { runtime.setTextBoxNumberRecursive("InfoWindow", 7); },
                        "an absent text box must not accept a number mutation");
    require_unavailable([&] { runtime.setTextBoxHorizontalPosition("InfoWindow", 1U); },
                        "an absent text box must not accept alignment mutation");
    ++passed;

    require_unavailable([&] { runtime.startAnim("Wait", 0U); },
                        "starting an absent BRLAN must fail instead of assigning a default duration");
    require(!runtime.hasActiveAnimation(0U),
            "a failed BRLAN start must be transactional and leave no active animation behind");
    require_unavailable([&] { (void)runtime.getAnimDuration("Appear"); },
                        "an absent BRLAN must not receive a hard-coded duration");
    require_unavailable([&] { (void)runtime.isAnimLooping("Wait"); },
                        "an absent BRLAN must not receive a name-based loop policy");
    require_unavailable([&] { (void)runtime.getAnimFrame(0U); },
                        "a layer without an active BRLAN must not expose a fabricated frame");
    require_unavailable([&] { (void)runtime.getAnimFrameMax(0U); },
                        "a layer without an active BRLAN must not expose a fabricated end frame");
    require_unavailable([&] { (void)runtime.isAnimStopped(0U); },
                        "a layer without an active BRLAN must not report a fabricated stopped state");
    require_unavailable([&] { (void)runtime.hasActiveAnimation(1U); },
                        "an unconfigured animation layer must not alias the last real layer");
    ++passed;

    auto manager = LayoutManager("unbound-layout-manager", true, 1U, 0x100U);
    require(manager.getPaneCtrl("InfoWindow") == nullptr && manager.getPaneMtxRef("InfoWindow") == nullptr,
            "manager lookups must preserve absent controls and matrices");
    require_unavailable([&] { (void)smgpc::layout::is_pane_visible(&manager, "InfoWindow"); },
                        "an unbound manager must not report fabricated pane visibility");
    require_unavailable([&] { (void)manager.createAndAddPaneCtrl("InfoWindow", 1U); },
                        "a manager without a real layout must not create a pane control");
    require_unavailable([&] { manager.createPaneMtxRef("InfoWindow"); },
                        "a manager without a real layout must not create an identity pane matrix");
    require_unavailable([&] { smgpc::layout::set_pane_visible(&manager, "InfoWindow", true, false); },
                        "a manager without a real layout must reject pane mutation");
    require_unavailable([&] { (void)smgpc::layout::pane_animation_frame(&manager, "InfoWindow", 0U); },
                        "a missing pane controller must not expose a zero animation frame");
    require_unavailable([&] { (void)smgpc::layout::is_pane_animation_stopped(&manager, "InfoWindow", 0U); },
                        "a missing pane controller must not report a fabricated stopped state");
    require_unavailable([&] { (void)smgpc::layout::animation_duration(&manager, "Wait"); },
                        "a manager without a real host must not expose a default duration");
    ++passed;

    require_unavailable([&] { (void)smgpc::layout::is_layout_actor_dead(nullptr); },
                        "a null LayoutActor must not be reported as an ordinary dead retail actor");
    require_unavailable([&] { runtime.initEffectKeeper(1, "LayoutEffect", nullptr); },
                        "layout-runtime effect initialization requires an active real RuntimeContext");
    auto ownerless_runtime = smgpc::layout::LayoutRuntime("", "DefinitelyMissingLayout", 1U, 0);
    require_unavailable([&] { ownerless_runtime.initEffectKeeper(1, "LayoutEffect", nullptr); },
                        "layout-runtime effect initialization requires a real named effect owner");
    ++passed;

    auto actor = LayoutActor("uninitialized-layout-actor", true);
    require_unavailable([&] { (void)smgpc::layout::layout_anim_frame(&actor, 0U); },
                        "an uninitialized LayoutActor must not expose a default animation frame");
    require_unavailable([&] { (void)smgpc::layout::is_layout_anim_stopped(&actor, 0U); },
                        "an uninitialized LayoutActor must not report a fabricated stopped state");
    actor.initLayoutManager("DefinitelyMissingLayout", 1U);
    require_unavailable([&] { actor.initEffectKeeper(1, "LayoutEffect", nullptr); },
                        "LayoutActor effect initialization requires an active real RuntimeContext");
    smgpc::layout::release_layout_actor_if_registered(&actor);
    ++passed;

    if (const auto archive = find_sys_info_window_mini_archive()) {
        auto real = smgpc::layout::LayoutRuntime(
            "real-dead-layout-host", "SysInfoWindowMini", 1U, 0, *archive);
        require(real.isDead() && real.hasPane("SaveIconPosition"),
                "the retail SaveIconPosition pane must exist even while its layout host is dead");
        require(real.paneIndex("SaveIconPosition").has_value() && *real.paneIndex("SaveIconPosition") > 0U,
                "a retail pane lookup must expose its parsed BRLYT index rather than a synthetic success index");

        Mtx dead_matrix{};
        require(real.copyPaneMatrix("SaveIconPosition", dead_matrix) && real.isDead(),
                "reading a retail pane matrix must not revive its dead layout host");
        require(std::fabs(dead_matrix[0][3] - 182.25F) < 0.001F &&
                    std::fabs(dead_matrix[1][3] - -60.0F) < 0.001F,
                "the dead-host matrix must preserve the retail location-adjusted SaveIconPosition transform");
        require(!real.isPaneVisible("SaveIconPosition") &&
                    !real.paneBounds("SaveIconPosition").has_value(),
                "a dead layout pane must expose no render or pointer bounds");

        real.appear();
        require(real.isPaneVisible("SaveIconPosition") &&
                    real.paneBounds("SaveIconPosition").has_value(),
                "an appearing retail pane must expose its real render and pointer bounds");
        real.setPaneVisible("SaveIconPosition", false);
        Mtx hidden_matrix{};
        require(real.hasPane("SaveIconPosition") &&
                    real.copyPaneMatrix("SaveIconPosition", hidden_matrix) &&
                    !real.isPaneVisible("SaveIconPosition") &&
                    !real.paneBounds("SaveIconPosition").has_value(),
                "a hidden real pane must keep its transform without acquiring pointer bounds");
        require(std::fabs(hidden_matrix[0][3] - dead_matrix[0][3]) < 0.001F &&
                    std::fabs(hidden_matrix[1][3] - dead_matrix[1][3]) < 0.001F,
                "hiding a pane must not replace its retail transform with an identity matrix");
        ++passed;
    }

    std::cout << "Layout real-or-absent tests passed: " << passed << "/"
              << (find_sys_info_window_mini_archive().has_value() ? 7 : 6) << "\n";
    return 0;
}
