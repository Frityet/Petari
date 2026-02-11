#include "TitleSequenceProduct.hpp"

#include <stdexcept>

#include "TitleRuntimeMR.hpp"
#include "runtime/Nerve.hpp"
#include "runtime/NerveUtil.hpp"
#include "runtime/Spine.hpp"

namespace smgpc::game::title {
namespace {

class NrvTitleSequenceProductDisplayEncouragePal60Window final : public runtime::Nerve {
public:
    static NrvTitleSequenceProductDisplayEncouragePal60Window s_instance;

    void execute(runtime::Spine *spine) const override {
        static_cast<TitleSequenceProduct *>(spine->executor())->exe_display_encourage_pal60_window();
    }
};

class NrvTitleSequenceProductBgmPrepare final : public runtime::Nerve {
public:
    static NrvTitleSequenceProductBgmPrepare s_instance;

    void execute(runtime::Spine *spine) const override {
        static_cast<TitleSequenceProduct *>(spine->executor())->exe_bgm_prepare();
    }
};

class NrvTitleSequenceProductLogoFadein final : public runtime::Nerve {
public:
    static NrvTitleSequenceProductLogoFadein s_instance;

    void execute(runtime::Spine *spine) const override {
        static_cast<TitleSequenceProduct *>(spine->executor())->exe_logo_fadein();
    }
};

class NrvTitleSequenceProductLogoWait final : public runtime::Nerve {
public:
    static NrvTitleSequenceProductLogoWait s_instance;

    void execute(runtime::Spine *spine) const override {
        static_cast<TitleSequenceProduct *>(spine->executor())->exe_logo_wait();
    }
};

class NrvTitleSequenceProductLogoDisplay final : public runtime::Nerve {
public:
    static NrvTitleSequenceProductLogoDisplay s_instance;

    void execute(runtime::Spine *spine) const override {
        static_cast<TitleSequenceProduct *>(spine->executor())->exe_logo_display();
    }
};

class NrvTitleSequenceProductDecide final : public runtime::Nerve {
public:
    static NrvTitleSequenceProductDecide s_instance;

    void execute(runtime::Spine *spine) const override {
        static_cast<TitleSequenceProduct *>(spine->executor())->exe_decide();
    }
};

class NrvTitleSequenceProductDead final : public runtime::Nerve {
public:
    static NrvTitleSequenceProductDead s_instance;

    void execute(runtime::Spine *spine) const override {
        static_cast<TitleSequenceProduct *>(spine->executor())->exe_dead();
    }
};

NrvTitleSequenceProductDisplayEncouragePal60Window NrvTitleSequenceProductDisplayEncouragePal60Window::s_instance {};
NrvTitleSequenceProductBgmPrepare NrvTitleSequenceProductBgmPrepare::s_instance {};
NrvTitleSequenceProductLogoFadein NrvTitleSequenceProductLogoFadein::s_instance {};
NrvTitleSequenceProductLogoWait NrvTitleSequenceProductLogoWait::s_instance {};
NrvTitleSequenceProductLogoDisplay NrvTitleSequenceProductLogoDisplay::s_instance {};
NrvTitleSequenceProductDecide NrvTitleSequenceProductDecide::s_instance {};
NrvTitleSequenceProductDead NrvTitleSequenceProductDead::s_instance {};

}  // namespace

TitleSequenceProduct::TitleSequenceProduct(TitleLayoutActor *logo_layout, TitleLayoutActor *press_start_layout)
    : _logo_layout(logo_layout), _press_start_layout(press_start_layout) {
    if (_logo_layout == nullptr || _press_start_layout == nullptr) {
        throw std::invalid_argument("TitleSequenceProduct requires non-null logo and press-start layouts.");
    }

    if (MR::is_display_encourage_pal60_window()) {
        init_nerve(&NrvTitleSequenceProductDisplayEncouragePal60Window::s_instance);
    } else {
        init_nerve(&NrvTitleSequenceProductBgmPrepare::s_instance);
    }

    _is_display_encourage_pal60_window = MR::is_display_encourage_pal60_window();
    kill();
}

void TitleSequenceProduct::appear() {
    if (_is_display_encourage_pal60_window) {
        set_nerve(&NrvTitleSequenceProductDisplayEncouragePal60Window::s_instance);
    } else {
        set_nerve(&NrvTitleSequenceProductBgmPrepare::s_instance);
    }
}

void TitleSequenceProduct::kill() {
    set_nerve(&NrvTitleSequenceProductDead::s_instance);
}

bool TitleSequenceProduct::is_active() const {
    return not is_nerve(&NrvTitleSequenceProductDead::s_instance);
}

void TitleSequenceProduct::update() {
    update_nerve();
    _logo_layout->update(1.0F);
    _press_start_layout->update(1.0F);
}

void TitleSequenceProduct::update_button_reaction(runtime::TriggerChecker *button_checker, const char *animation_name) {
    if (button_checker == nullptr || animation_name == nullptr) {
        return;
    }

    if (button_checker->on_trigger()) {
        MR::start_anim(_logo_layout, animation_name, 1U);
        MR::set_anim_frame_and_stop(_logo_layout, 0.0F, 1U);
    } else if (button_checker->off_trigger()) {
        MR::start_anim(_logo_layout, animation_name, 1U);
    }
}

void TitleSequenceProduct::update_press_start_reaction() {
    if (_a_button_checker.on_trigger()) {
        MR::start_anim(_press_start_layout, "ButtonReaction", 0U);
    } else if (_b_button_checker.on_trigger()) {
        MR::start_anim(_press_start_layout, "ButtonReaction", 0U);
    }
}

void TitleSequenceProduct::exe_display_encourage_pal60_window() {
    set_nerve(&NrvTitleSequenceProductBgmPrepare::s_instance);
}

void TitleSequenceProduct::exe_bgm_prepare() {
    if (MR::is_first_step(this)) {
        MR::start_stage_bgm("STM_TITLE", true);
    }

    if (MR::is_prepared_stage_bgm()) {
        set_nerve(&NrvTitleSequenceProductLogoFadein::s_instance);
    }
}

void TitleSequenceProduct::exe_logo_fadein() {
    if (MR::is_first_step(this)) {
        _logo_layout->appear();
        MR::start_anim(_logo_layout, "Appear", 0U);
        MR::unlock_stage_bgm();
    }

    if (MR::is_anim_stopped(_logo_layout, 0U)) {
        set_nerve(&NrvTitleSequenceProductLogoWait::s_instance);
    }
}

void TitleSequenceProduct::exe_logo_wait() {
    if (MR::is_first_step(this)) {
        MR::start_anim(_logo_layout, "Wait", 0U);
        MR::emit_effect(_logo_layout, "TitleLogoLight");
        MR::emit_effect(_logo_layout, "TitleLogoLightB");
        MR::emit_effect(_logo_layout, "TitleLogoLightC");
        MR::emit_effect(_logo_layout, "TitleLogoLightD");
        MR::emit_effect(_logo_layout, "TitleLogoLightE");
        MR::emit_effect(_logo_layout, "TitleLogoLightF");
        MR::emit_effect(_logo_layout, "TitleLogoLightG");
    }

    if (MR::is_step(this, PRESS_AB_APPEAR_FRAME)) {
        set_nerve(&NrvTitleSequenceProductLogoDisplay::s_instance);
    }
}

void TitleSequenceProduct::exe_logo_display() {
    if (MR::is_first_step(this)) {
        _press_start_layout->appear();
        MR::start_anim(_press_start_layout, "Appear", 0U);
    }

    if (MR::is_anim_stopped(_press_start_layout, 0U)) {
        MR::start_anim(_press_start_layout, "Wait", 0U);
    }

    _a_button_checker.update(MR::test_core_pad_button_a(0));
    _b_button_checker.update(MR::test_core_pad_button_b(0));

    if (_a_button_checker.level() && _b_button_checker.level()) {
        MR::stop_stage_bgm(75);
        MR::start_system_se("SE_SY_GAME_START", -1, -1);
        MR::start_cs_sound("CS_CLICK_CLOSE", 0, 0);
        MR::try_rumble_pad_middle(this, 0);
        set_nerve(&NrvTitleSequenceProductDecide::s_instance);
    } else {
        update_button_reaction(&_a_button_checker, "ReactionA");
        update_button_reaction(&_b_button_checker, "ReactionB");
        update_press_start_reaction();
    }
}

void TitleSequenceProduct::exe_decide() {
    if (MR::is_first_step(this)) {
        MR::start_anim(_logo_layout, "Decide", 0U);
        MR::delete_effect_all(_logo_layout);
        MR::start_anim(_press_start_layout, "End", 0U);
    }

    if (MR::is_anim_stopped(_logo_layout, 0U) && MR::is_anim_stopped(_press_start_layout, 0U)) {
        set_nerve(&NrvTitleSequenceProductDead::s_instance);
    }
}

void TitleSequenceProduct::exe_dead() {
    if (MR::is_first_step(this)) {
        _logo_layout->kill();
        _press_start_layout->kill();
    }
}

const TitleLayoutActor *TitleSequenceProduct::logo_layout() const {
    return _logo_layout;
}

const TitleLayoutActor *TitleSequenceProduct::press_start_layout() const {
    return _press_start_layout;
}

}  // namespace smgpc::game::title
