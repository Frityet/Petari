#pragma once

#include <cstdint>

#include "TitleLayoutActor.hpp"
#include "runtime/NerveExecutor.hpp"
#include "runtime/TriggerChecker.hpp"

namespace smgpc::game::title {

class TitleSequenceProduct final : public runtime::NerveExecutor {
public:
    static constexpr std::int32_t PRESS_AB_APPEAR_FRAME = 25;

    TitleSequenceProduct(TitleLayoutActor *logo_layout, TitleLayoutActor *press_start_layout);

    void appear();
    void kill();
    [[nodiscard]] bool is_active() const;
    void update();

    void update_button_reaction(runtime::TriggerChecker *button_checker, const char *animation_name);
    void update_press_start_reaction();

    void exe_display_encourage_pal60_window();
    void exe_bgm_prepare();
    void exe_logo_fadein();
    void exe_logo_wait();
    void exe_logo_display();
    void exe_decide();
    void exe_dead();

    [[nodiscard]] const TitleLayoutActor *logo_layout() const;
    [[nodiscard]] const TitleLayoutActor *press_start_layout() const;

private:
    TitleLayoutActor *_logo_layout {};
    TitleLayoutActor *_press_start_layout {};
    runtime::TriggerChecker _a_button_checker {};
    runtime::TriggerChecker _b_button_checker {};
    bool _is_display_encourage_pal60_window {};
};

}  // namespace smgpc::game::title
