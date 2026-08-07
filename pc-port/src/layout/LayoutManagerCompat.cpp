#include "layout/LayoutHost.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <utility>

#include "Game/LiveActor/Spine.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Screen/ButtonPaneController.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/Screen/LayoutManager.hpp"
#include "Game/Screen/LayoutPaneCtrl.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "layout/LayoutRuntime.hpp"
#include "runtime/RuntimeContext.hpp"

namespace {

struct ActorState {
    TVec2f translation{};
    std::array< J3DFrameCtrl, 4U > animation_controls{};
    std::array< f32, 4U > last_frames{};
    std::array< f32, 4U > last_rates{};
};

struct PaneMatrixReference {
    std::string pane_name;
    Mtx matrix{};
    bool valid = false;
};

struct ManagerState {
    std::string layout_name;
    bool convert_filename = true;
    u32 animation_layer_count = 0U;
    u32 text_box_buffer_length = 0U;
    LayoutActor* actor = nullptr;
    std::unique_ptr< smgpc::layout::LayoutRuntime > runtime;
    std::vector< std::unique_ptr< LayoutPaneCtrl > > pane_controls;
    std::vector< ButtonPaneController* > button_controllers;
    std::vector< PaneMatrixReference > pane_matrix_references;
};

struct PaneControlState {
    std::string pane_name;
    std::vector< J3DFrameCtrl > animation_controls;
    std::vector< std::string > animation_names;
};

std::unordered_map< const LayoutActor*, ActorState > sActorStates;
std::unordered_map< const LayoutManager*, ManagerState > sManagerStates;
std::unordered_map< const LayoutPaneCtrl*, PaneControlState > sPaneControlStates;

[[nodiscard]] std::string pane_name(const char* name) {
    return name != nullptr ? std::string(name) : std::string{};
}

[[nodiscard]] ActorState& require_actor_state(const LayoutActor* actor, std::string_view operation) {
    if (actor == nullptr) {
        throw std::invalid_argument(std::string(operation) + " requires a layout actor");
    }
    const auto it = sActorStates.find(actor);
    if (it == sActorStates.end()) {
        throw std::logic_error(std::string(operation) + " requires registered layout actor state");
    }
    return it->second;
}

[[nodiscard]] ManagerState& require_manager_state(const LayoutManager* manager, std::string_view operation) {
    if (manager == nullptr) {
        throw std::invalid_argument(std::string(operation) + " requires a layout manager");
    }
    const auto it = sManagerStates.find(manager);
    if (it == sManagerStates.end()) {
        throw std::logic_error(std::string(operation) + " requires registered layout manager state");
    }
    return it->second;
}

[[nodiscard]] PaneControlState& require_pane_control_state(const LayoutPaneCtrl* pane_control, std::string_view operation) {
    if (pane_control == nullptr) {
        throw std::invalid_argument(std::string(operation) + " requires a pane control");
    }
    const auto it = sPaneControlStates.find(pane_control);
    if (it == sPaneControlStates.end()) {
        throw std::logic_error(std::string(operation) + " requires registered pane-control state");
    }
    return it->second;
}

[[nodiscard]] smgpc::layout::LayoutRuntime& require_runtime(const LayoutManager* manager, std::string_view operation) {
    auto& state = require_manager_state(manager, operation);
    if (state.runtime == nullptr) {
        throw std::logic_error(std::string(operation) + " requires an initialized retail layout resource");
    }
    return *state.runtime;
}

[[nodiscard]] u32 require_actor_layer(const ManagerState& manager, u32 layer, std::string_view operation) {
    if (layer >= manager.animation_layer_count || layer >= 4U) {
        throw std::out_of_range(std::string(operation) + " requested unavailable layout animation layer " + std::to_string(layer));
    }
    return layer;
}

[[nodiscard]] u32 require_pane_layer(const PaneControlState& pane, u32 layer, std::string_view operation) {
    if (layer >= pane.animation_controls.size()) {
        throw std::out_of_range(std::string(operation) + " requested unavailable pane animation layer " + std::to_string(layer));
    }
    return layer;
}

[[nodiscard]] LayoutPaneCtrl* find_pane_control(const ManagerState& state, std::string_view name) {
    const auto it = std::ranges::find_if(state.pane_controls, [name](const auto& control) {
        const auto pane_it = sPaneControlStates.find(control.get());
        return pane_it != sPaneControlStates.end() && pane_it->second.pane_name == name;
    });
    return it != state.pane_controls.end() ? it->get() : nullptr;
}

void sync_actor_control_to_runtime(LayoutActor* actor, u32 layer) {
    auto& manager = require_manager_state(actor->mLayoutManager, "Synchronizing layout animation");
    (void)require_actor_layer(manager, layer, "Synchronizing layout animation");
    if (manager.runtime == nullptr || !manager.runtime->hasActiveAnimation(layer)) {
        return;
    }

    auto& actor_state = require_actor_state(actor, "Synchronizing layout animation");
    const auto& control = actor_state.animation_controls[layer];
    if (control.mFrame != actor_state.last_frames[layer]) {
        manager.runtime->setAnimFrame(control.mFrame, layer);
    }
    if (control.mRate != actor_state.last_rates[layer]) {
        manager.runtime->setAnimRate(control.mRate, layer);
    }
}

void sync_actor_control_from_runtime(LayoutActor* actor, u32 layer) {
    auto& manager = require_manager_state(actor->mLayoutManager, "Synchronizing layout animation");
    (void)require_actor_layer(manager, layer, "Synchronizing layout animation");
    if (manager.runtime == nullptr || !manager.runtime->hasActiveAnimation(layer)) {
        return;
    }

    auto& actor_state = require_actor_state(actor, "Synchronizing layout animation");
    auto& control = actor_state.animation_controls[layer];
    control.mFrame = manager.runtime->getAnimFrame(layer);
    control.mEnd = static_cast< s16 >(manager.runtime->getAnimFrameMax(layer));
    control.mRate = manager.runtime->getAnimRate(layer);
    control.mAttribute = manager.runtime->isAnimLooping(layer) ? J3DFrameCtrl::EMode_LOOP : J3DFrameCtrl::EMode_NONE;
    actor_state.last_frames[layer] = control.mFrame;
    actor_state.last_rates[layer] = control.mRate;
}

void bind_actor_manager(LayoutActor* actor, LayoutManager* manager) {
    auto& actor_state = require_actor_state(actor, "Binding a layout manager");
    auto& manager_state = require_manager_state(manager, "Binding a layout manager");
    manager_state.actor = actor;
    manager_state.runtime = std::make_unique< smgpc::layout::LayoutRuntime >(
        actor->getName(), manager_state.layout_name.c_str(), manager_state.animation_layer_count, MR::DrawType_Layout);
    manager_state.runtime->initWithoutIter();
    manager_state.runtime->kill();
    manager_state.runtime->setTrans(actor_state.translation.x, actor_state.translation.y);
}

void refresh_pane_matrix(ManagerState& manager, PaneMatrixReference& reference) {
    if (manager.runtime == nullptr) {
        reference.valid = false;
        return;
    }
    reference.valid = manager.runtime->copyPaneMatrix(reference.pane_name, reference.matrix);
}

[[noreturn]] void throw_retail_nw4r_unavailable(std::string_view operation) {
    throw std::logic_error(std::string(operation) + " requires the unavailable retail NW4R layout object graph");
}

[[nodiscard]] std::string_view require_effect_owner_name(const char* owner_name, std::string_view operation) {
    if (owner_name == nullptr || *owner_name == '\0') {
        throw std::logic_error(std::string(operation) + " requires a real named effect owner");
    }
    return owner_name;
}

#ifndef NDEBUG
[[nodiscard]] const char* button_nerve_name(const ButtonPaneController& button) {
    if (button.mSpine == nullptr || button.mSpine->getCurrentNerve() == nullptr) {
        throw std::logic_error("Inspecting a button controller requires an initialized retail nerve");
    }

    const auto dynamic_name = std::string_view(typeid(*button.mSpine->getCurrentNerve()).name());
    constexpr auto names = std::array{
        std::pair{std::string_view("ButtonPaneControllerNrvHidden"), "Hidden"},
        std::pair{std::string_view("ButtonPaneControllerNrvAppear"), "Appear"},
        std::pair{std::string_view("ButtonPaneControllerNrvWait"), "Wait"},
        std::pair{std::string_view("ButtonPaneControllerNrvPointing"), "Pointing"},
        std::pair{std::string_view("ButtonPaneControllerNrvNotPointing"), "NotPointing"},
        std::pair{std::string_view("ButtonPaneControllerNrvDecidedToDisappear"), "DecidedToDisappear"},
        std::pair{std::string_view("ButtonPaneControllerNrvDecidedWait"), "DecidedWait"},
        std::pair{std::string_view("ButtonPaneControllerNrvDecided"), "Decided"},
        std::pair{std::string_view("ButtonPaneControllerNrvDisappear"), "Disappear"},
    };
    for (const auto& [type_name, label] : names) {
        if (dynamic_name.contains(type_name)) {
            return label;
        }
    }
    throw std::logic_error("Inspecting a button controller encountered an unknown retail nerve type");
}
#endif

}  // namespace

namespace nw4r::lyt {

DrawInfo::DrawInfo() : mGlobalAlpha(1.0F) {
    std::memset(&mFlag, 0, sizeof(mFlag));
    mLocationAdjustScale.x = 1.0F;
    mLocationAdjustScale.y = 1.0F;
    PSMTXIdentity(mViewMtx.mtx);
}

DrawInfo::~DrawInfo() = default;

}  // namespace nw4r::lyt

LayoutActor::LayoutActor(const char* name, bool)
    : NameObj(name), mLayoutManager(nullptr), mSpine(nullptr), mEffectKeeper(nullptr), mPointingTarget(nullptr) {
    sActorStates.insert_or_assign(this, ActorState{});
}

void LayoutActor::movement() {
    if (mFlag.mIsDead) {
        return;
    }
    updateSpine();
    if (mFlag.mIsDead) {
        return;
    }
    control();
    if (mLayoutManager != nullptr && !mFlag.mIsStopAnimFrame) {
        mLayoutManager->movement();
    }
}

void LayoutActor::calcAnim() {
    if (mFlag.mIsOffCalcAnim) {
        return;
    }
    if (mLayoutManager == nullptr) {
        throw std::logic_error("Layout animation calculation requires an initialized layout manager");
    }
    mLayoutManager->calcAnim();
}

void LayoutActor::draw() const {
    smgpc::layout::draw_layout_actor(this);
}

void LayoutActor::appear() {
    mFlag.mIsDead = false;
    smgpc::layout::require_layout_runtime(this, "Appearing a layout actor").appear();
    calcAnim();
}

void LayoutActor::kill() {
    mFlag.mIsDead = true;
    smgpc::layout::require_layout_runtime(this, "Killing a layout actor").kill();
    if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
        runtime->delete_effect_all(getName(), this);
    }
}

void LayoutActor::setNerve(const Nerve* nerve) const {
    if (mSpine == nullptr) {
        throw std::logic_error("Setting a layout nerve requires an initialized Spine");
    }
    mSpine->setNerve(nerve);
}

bool LayoutActor::isNerve(const Nerve* nerve) const {
    if (mSpine == nullptr) {
        throw std::logic_error("Reading a layout nerve requires an initialized Spine");
    }
    return mSpine->getCurrentNerve() == nerve;
}

s32 LayoutActor::getNerveStep() const {
    if (mSpine == nullptr) {
        throw std::logic_error("Reading a layout nerve step requires an initialized Spine");
    }
    return mSpine->mStep;
}

TVec2f LayoutActor::getTrans() const {
    auto translation = TVec2f{};
    MR::copyPaneTrans(&translation, this, nullptr);
    MR::convertLayoutPosToScreenPos(&translation, translation);
    return translation;
}

void LayoutActor::setTrans(const TVec2f& screen_translation) {
    auto& actor_state = require_actor_state(this, "Setting a layout translation");
    MR::convertScreenPosToLayoutPos(&actor_state.translation, screen_translation);
    smgpc::layout::require_layout_runtime(this, "Setting a layout translation")
        .setTrans(actor_state.translation.x, actor_state.translation.y);
}

LayoutManager* LayoutActor::getLayoutManager() const {
    return mLayoutManager;
}

void LayoutActor::createPaneMtxRef(const char* name) {
    if (mLayoutManager == nullptr) {
        throw std::logic_error("Creating a pane matrix reference requires an initialized layout manager");
    }
    mLayoutManager->createPaneMtxRef(name);
}

MtxPtr LayoutActor::getPaneMtxRef(const char* name) {
    if (mLayoutManager == nullptr) {
        throw std::logic_error("Reading a pane matrix reference requires an initialized layout manager");
    }
    return mLayoutManager->getPaneMtxRef(name);
}

void LayoutActor::initLayoutManager(const char* name, u32 layer_count) {
    mLayoutManager = new LayoutManager(name, true, layer_count, 0x100U);
    bind_actor_manager(this, mLayoutManager);
}

void LayoutActor::initLayoutManagerNoConvertFilename(const char* name, u32 layer_count) {
    mLayoutManager = new LayoutManager(name, false, layer_count, 0x100U);
    bind_actor_manager(this, mLayoutManager);
}

void LayoutActor::initLayoutManagerWithTextBoxBufferLength(const char* name, u32 buffer_length, u32 layer_count) {
    mLayoutManager = new LayoutManager(name, false, layer_count, buffer_length);
    bind_actor_manager(this, mLayoutManager);
}

void LayoutActor::initNerve(const Nerve* nerve) {
    mSpine = new Spine(this, nerve);
}

void LayoutActor::initEffectKeeper(int effect_count, const char* effect_name, const EffectSystem*) {
    auto& layout = smgpc::layout::require_layout_runtime(this, "Initializing layout effects");
    const auto owner_name = require_effect_owner_name(getName(), "Initializing layout effects");
    auto& runtime = smgpc::runtime::RuntimeContext::instance();
    const auto group_name = effect_name != nullptr ? std::string_view(effect_name) : std::string_view(layout.getLayoutName());
    runtime.register_effect_keeper(smgpc::runtime::EffectKeeperHostKind::LayoutActor, owner_name, effect_count,
                                   group_name, false, this);
}

void LayoutActor::initPointingTarget(int) {
    throw_retail_nw4r_unavailable("Initializing a StarPointerLayoutTargetKeeper");
}

void LayoutActor::updateSpine() {
    if (mSpine != nullptr) {
        mSpine->update();
    }
}

LayoutManager::LayoutManager(const char* layout_name, bool convert_filename, u32 animation_layer_count,
                             u32 text_box_buffer_length)
    : mLayoutHolder(nullptr), mLayout(nullptr), mAnimTransList(nullptr), mDrawInfo(), mIsScreenHidden(false), _61(false),
      _64(0U), _68(0U), _6C(0U), _70(0U), _74(0U), _78(nullptr) {
    if (layout_name == nullptr || *layout_name == '\0') {
        throw std::invalid_argument("A layout manager requires a retail layout resource name");
    }
    if (animation_layer_count == 0U || animation_layer_count > 4U) {
        throw std::invalid_argument("A layout manager requires between one and four animation layers");
    }
    auto [it, inserted] = sManagerStates.try_emplace(this);
    if (!inserted) {
        it->second = ManagerState{};
    }
    it->second.layout_name = layout_name;
    it->second.convert_filename = convert_filename;
    it->second.animation_layer_count = animation_layer_count;
    it->second.text_box_buffer_length = text_box_buffer_length;
}

void LayoutManager::movement() {
    auto& state = require_manager_state(this, "Moving a layout manager");
    for (auto& control : state.pane_controls) {
        control->movement();
    }
    if (state.actor == nullptr || state.runtime == nullptr) {
        throw std::logic_error("Moving a layout manager requires its initialized layout actor host");
    }
    for (auto layer = u32{}; layer < state.animation_layer_count; ++layer) {
        sync_actor_control_to_runtime(state.actor, layer);
    }
    state.runtime->update();
    for (auto layer = u32{}; layer < state.animation_layer_count; ++layer) {
        sync_actor_control_from_runtime(state.actor, layer);
    }
    for (const auto& control : state.pane_controls) {
        auto& pane = require_pane_control_state(control.get(), "Synchronizing pane animation");
        for (auto layer = u32{}; layer < pane.animation_controls.size(); ++layer) {
            if (pane.animation_names[layer].empty()) {
                continue;
            }
            pane.animation_controls[layer].mFrame = state.runtime->getPaneAnimFrame(pane.pane_name, layer);
            if (state.runtime->isPaneAnimStopped(pane.pane_name, layer)) {
                pane.animation_controls[layer].mRate = 0.0F;
            }
        }
    }
    smgpc::layout::refresh_pane_matrices(this);
}

void LayoutManager::calcAnim() {
    auto& state = require_manager_state(this, "Calculating a layout manager");
    for (auto& control : state.pane_controls) {
        control->calcAnim();
    }
    smgpc::layout::refresh_pane_matrices(this);
}

void LayoutManager::draw() const {
    const auto& state = require_manager_state(this, "Drawing a layout manager");
    if (state.actor == nullptr || state.runtime == nullptr) {
        throw std::logic_error("Drawing a layout manager requires its initialized layout actor host");
    }
    if (!state.actor->mFlag.mIsDead && !state.actor->mFlag.mIsHidden) {
        state.runtime->draw();
    }
}

void LayoutManager::addPaneCtrl(LayoutPaneCtrl* pane_control) {
    if (pane_control == nullptr) {
        throw std::invalid_argument("Adding a pane control requires a real control");
    }
    auto& state = require_manager_state(this, "Adding a pane control");
    if (std::ranges::any_of(state.pane_controls, [pane_control](const auto& existing) { return existing.get() == pane_control; })) {
        throw std::logic_error("The pane control is already owned by this layout manager");
    }
    state.pane_controls.emplace_back(pane_control);
}

LayoutPaneCtrl* LayoutManager::createAndAddRootPaneCtrl(u32 layer_count) {
    return createAndAddPaneCtrl(nullptr, layer_count);
}

LayoutPaneCtrl* LayoutManager::createAndAddPaneCtrl(const char* name, u32 layer_count) {
    auto& state = require_manager_state(this, "Creating a pane control");
    const auto requested_name = pane_name(name);
    if (auto* existing = find_pane_control(state, requested_name)) {
        return existing;
    }
    auto& runtime = require_runtime(this, "Creating a pane control");
    if (!runtime.hasPane(requested_name)) {
        throw std::runtime_error("Cannot create a control for absent layout pane " + requested_name);
    }
    auto control = std::make_unique< LayoutPaneCtrl >(this, name, layer_count);
    auto* result = control.get();
    state.pane_controls.push_back(std::move(control));
    return result;
}

LayoutPaneCtrl* LayoutManager::getPaneCtrl(const char* name) const {
    const auto& state = require_manager_state(this, "Finding a pane control");
    return find_pane_control(state, pane_name(name));
}

s32 LayoutManager::getIndexOfPane(const char* name) const {
    auto& runtime = require_runtime(this, "Finding a layout pane");
    const auto index = runtime.paneIndex(pane_name(name));
    return index.has_value() ? static_cast< s32 >(*index) : -1;
}

bool LayoutManager::isExistPaneCtrl(const char* name) const {
    return getPaneCtrl(name) != nullptr;
}

void LayoutManager::addGroupCtrl(LayoutGroupCtrl*) {
    throw_retail_nw4r_unavailable("Adding a LayoutGroupCtrl");
}

bool LayoutManager::isPointing(const nw4r::lyt::Pane*, const TVec2f&) const {
    throw_retail_nw4r_unavailable("Testing an NW4R pane pointer");
}

LayoutPaneCtrl* LayoutManager::createAndAddGroupCtrl(const char*, u32) {
    throw_retail_nw4r_unavailable("Creating a LayoutGroupCtrl");
}

s32 LayoutManager::getIndexOfGroupCtrl(const char*) const {
    throw_retail_nw4r_unavailable("Finding a LayoutGroupCtrl");
}

void LayoutManager::createPaneMtxRef(const char* name) {
    auto& state = require_manager_state(this, "Creating a pane matrix reference");
    const auto requested_name = pane_name(name);
    if (std::ranges::any_of(state.pane_matrix_references,
                            [&requested_name](const auto& reference) { return reference.pane_name == requested_name; })) {
        return;
    }
    auto& reference = state.pane_matrix_references.emplace_back();
    reference.pane_name = requested_name;
    refresh_pane_matrix(state, reference);
    if (!reference.valid) {
        state.pane_matrix_references.pop_back();
        throw std::runtime_error("Cannot create a matrix reference for absent layout pane " + requested_name);
    }
}

MtxPtr LayoutManager::getPaneMtxRef(const char* name) const {
    const auto& state = require_manager_state(this, "Reading a pane matrix reference");
    const auto requested_name = pane_name(name);
    const auto it = std::ranges::find_if(state.pane_matrix_references,
                                         [&requested_name](const auto& reference) { return reference.pane_name == requested_name; });
    return it != state.pane_matrix_references.end() && it->valid ? const_cast< MtxPtr >(it->matrix) : nullptr;
}

bool LayoutManager::isExistPaneMtxRef(const char* name) const {
    return getPaneMtxRef(name) != nullptr;
}

bool LayoutManager::isPointing(const char* name, const TVec2f& point) const {
    return smgpc::layout::is_pointing_pane(this, name, point.x, point.y);
}

nw4r::lyt::AnimTransform* LayoutManager::getAnimTransform(const char*) const {
    throw_retail_nw4r_unavailable("Exposing an NW4R AnimTransform");
}

void LayoutManager::bindPaneCtrlAnim(LayoutPaneCtrl*, nw4r::lyt::AnimTransform*) {
    throw_retail_nw4r_unavailable("Binding an NW4R pane animation");
}

void LayoutManager::bindPaneCtrlAnimSub(u32&, nw4r::lyt::AnimTransform*) {
    throw_retail_nw4r_unavailable("Binding an NW4R pane animation subtree");
}

void LayoutManager::unbindPaneCtrlAnim(LayoutPaneCtrl*, nw4r::lyt::AnimTransform*) {
    throw_retail_nw4r_unavailable("Unbinding an NW4R pane animation");
}

void LayoutManager::unbindPaneCtrlAnimSub(u32&, nw4r::lyt::AnimTransform*) {
    throw_retail_nw4r_unavailable("Unbinding an NW4R pane animation subtree");
}

void LayoutManager::calcAnimWithoutLocationAdjust(const nw4r::lyt::DrawInfo&) {
    throw_retail_nw4r_unavailable("Calculating with an external NW4R DrawInfo");
}

nw4r::lyt::Group* LayoutManager::getGroup(const char*) const {
    throw_retail_nw4r_unavailable("Exposing an NW4R layout group");
}

void LayoutManager::initArc(const char*, const char*) {
    throw_retail_nw4r_unavailable("Reinitializing a retail layout archive");
}

void LayoutManager::initDrawInfo() {
    mDrawInfo = nw4r::lyt::DrawInfo{};
}

void LayoutManager::initPaneInfo() {
    throw_retail_nw4r_unavailable("Initializing NW4R pane metadata");
}

void LayoutManager::initPaneInfoRecursive(u32&, nw4r::lyt::Pane*) {
    throw_retail_nw4r_unavailable("Initializing NW4R pane metadata recursively");
}

u32 LayoutManager::countPanes(nw4r::lyt::Pane*) {
    throw_retail_nw4r_unavailable("Counting an NW4R pane subtree");
}

void LayoutManager::initGroupCtrlList() {
    throw_retail_nw4r_unavailable("Initializing NW4R group controls");
}

void LayoutManager::initTextBoxRecursive(nw4r::lyt::Pane*, nw4r::lyt::Pane*, const char*, u32) {
    throw_retail_nw4r_unavailable("Initializing NW4R text boxes");
}

void LayoutManager::animateRecursive(u32&, nw4r::lyt::Pane*) {
    throw_retail_nw4r_unavailable("Animating an NW4R pane subtree");
}

nw4r::lyt::Pane* LayoutManager::getPane(const char*) const {
    throw_retail_nw4r_unavailable("Exposing an NW4R Pane");
}

nw4r::lyt::Pane* LayoutManager::findPaneByName(const char*) const {
    throw_retail_nw4r_unavailable("Exposing an NW4R Pane");
}

void LayoutManager::replaceIndDummyTexture() {
    throw_retail_nw4r_unavailable("Replacing an NW4R indirect dummy texture");
}

void LayoutManager::removeUnnecessaryPanes(nw4r::lyt::Pane*) {
    throw_retail_nw4r_unavailable("Removing an NW4R pane subtree");
}

LayoutPaneCtrl::LayoutPaneCtrl(LayoutManager* host, const char* name, u32 layer_count)
    : mHost(host), mPane(nullptr), mPaneIndex(-1), mAnmPlayerArray(static_cast< s32 >(layer_count)), mFollowType(0U),
      mFollowPos(nullptr) {
    if (host == nullptr) {
        throw std::invalid_argument("A pane control requires a layout manager host");
    }
    if (layer_count == 0U || layer_count > 4U) {
        throw std::invalid_argument("A pane control requires between one and four animation layers");
    }
    auto& runtime = require_runtime(host, "Creating a pane control");
    const auto requested_name = pane_name(name);
    if (!runtime.hasPane(requested_name)) {
        throw std::runtime_error("A pane control requires a real layout pane: " + requested_name);
    }
    for (auto layer = u32{}; layer < layer_count; ++layer) {
        mAnmPlayerArray[layer] = nullptr;
    }
    auto [it, inserted] = sPaneControlStates.try_emplace(this);
    if (!inserted) {
        it->second = PaneControlState{};
    }
    it->second.pane_name = requested_name;
    it->second.animation_controls.resize(layer_count);
    it->second.animation_names.resize(layer_count);
}

void LayoutPaneCtrl::movement() {
    auto& state = require_pane_control_state(this, "Moving a pane control");
    auto& runtime = require_runtime(mHost, "Moving a pane control");
    for (auto layer = u32{}; layer < state.animation_controls.size(); ++layer) {
        if (state.animation_names[layer].empty()) {
            continue;
        }
        auto& control = state.animation_controls[layer];
        runtime.setPaneAnimFrame(state.pane_name, control.mFrame, layer);
        runtime.setPaneAnimRate(state.pane_name, control.mRate, layer);
        control.mFrame = runtime.getPaneAnimFrame(state.pane_name, layer);
        control.mRate = runtime.isPaneAnimStopped(state.pane_name, layer) ? 0.0F : control.mRate;
    }
}

void LayoutPaneCtrl::calcAnim() {
    auto& state = require_pane_control_state(this, "Calculating a pane animation");
    auto& runtime = require_runtime(mHost, "Calculating a pane animation");
    for (auto layer = u32{}; layer < state.animation_controls.size(); ++layer) {
        if (!state.animation_names[layer].empty()) {
            runtime.setPaneAnimFrame(state.pane_name, state.animation_controls[layer].mFrame, layer);
        }
    }
}

void LayoutPaneCtrl::start(const char* animation_name, u32 layer) {
    auto& state = require_pane_control_state(this, "Starting a pane animation");
    (void)require_pane_layer(state, layer, "Starting a pane animation");
    auto& runtime = require_runtime(mHost, "Starting a pane animation");
    runtime.startPaneAnim(state.pane_name, animation_name, layer);
    state.animation_names[layer] = animation_name != nullptr ? animation_name : "";
    auto& control = state.animation_controls[layer];
    control.init(static_cast< s16 >(runtime.getAnimDuration(animation_name)));
    control.mAttribute = runtime.isAnimLooping(animation_name) ? J3DFrameCtrl::EMode_LOOP : J3DFrameCtrl::EMode_NONE;
    control.mFrame = runtime.getPaneAnimFrame(state.pane_name, layer);
    control.mRate = 1.0F;
}

void LayoutPaneCtrl::stop(u32 layer) {
    auto& state = require_pane_control_state(this, "Stopping a pane animation");
    (void)require_pane_layer(state, layer, "Stopping a pane animation");
    require_runtime(mHost, "Stopping a pane animation").stopPaneAnim(state.pane_name, layer);
    state.animation_controls[layer].mRate = 0.0F;
}

bool LayoutPaneCtrl::isAnimStopped(u32 layer) const {
    auto& state = require_pane_control_state(this, "Reading a pane animation state");
    (void)require_pane_layer(state, layer, "Reading a pane animation state");
    return require_runtime(mHost, "Reading a pane animation state").isPaneAnimStopped(state.pane_name, layer);
}

void LayoutPaneCtrl::reflectFollowPos() {
    if (mFollowPos == nullptr) {
        throw std::logic_error("Reflecting a pane follow position requires a real follow position");
    }
    throw_retail_nw4r_unavailable("Reflecting a pane follow position");
}

J3DFrameCtrl* LayoutPaneCtrl::getFrameCtrl(u32 layer) const {
    auto& state = require_pane_control_state(this, "Reading a pane animation control");
    return &state.animation_controls[require_pane_layer(state, layer, "Reading a pane animation control")];
}

void LayoutPaneCtrl::recalcChildGlobalMtx(nw4r::lyt::Pane*) {
    throw_retail_nw4r_unavailable("Recalculating an NW4R pane subtree matrix");
}

namespace smgpc::layout {

LayoutRuntime* layout_runtime(LayoutActor* actor) {
    return const_cast< LayoutRuntime* >(layout_runtime(static_cast< const LayoutActor* >(actor)));
}

const LayoutRuntime* layout_runtime(const LayoutActor* actor) {
    if (actor == nullptr || actor->mLayoutManager == nullptr) {
        return nullptr;
    }
    const auto it = sManagerStates.find(actor->mLayoutManager);
    return it != sManagerStates.end() ? it->second.runtime.get() : nullptr;
}

LayoutRuntime& require_layout_runtime(LayoutActor* actor, std::string_view operation) {
    return const_cast< LayoutRuntime& >(require_layout_runtime(static_cast< const LayoutActor* >(actor), operation));
}

const LayoutRuntime& require_layout_runtime(const LayoutActor* actor, std::string_view operation) {
    const auto* runtime = layout_runtime(actor);
    if (runtime == nullptr) {
        throw std::logic_error(std::string(operation) + " requires an initialized retail layout resource");
    }
    return *runtime;
}

bool is_layout_actor_dead(const LayoutActor* actor) {
    if (actor == nullptr) {
        throw std::invalid_argument("Reading layout actor life state requires a real layout actor");
    }
    return actor->mFlag.mIsDead;
}

void release_layout_actor_if_registered(NameObj* object) {
    if (object == nullptr) {
        return;
    }
    auto* actor = reinterpret_cast< LayoutActor* >(object);
    const auto actor_it = sActorStates.find(actor);
    if (actor_it == sActorStates.end()) {
        return;
    }

    if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
        runtime->unregister_effect_keeper(actor->getName(), actor);
        runtime->unregister_layout_actor(*actor);
    }

    if (actor->mLayoutManager != nullptr) {
        const auto manager_it = sManagerStates.find(actor->mLayoutManager);
        if (manager_it != sManagerStates.end()) {
            for (const auto& control : manager_it->second.pane_controls) {
                sPaneControlStates.erase(control.get());
            }
            sManagerStates.erase(manager_it);
        }
        delete actor->mLayoutManager;
        actor->mLayoutManager = nullptr;
    }
    sActorStates.erase(actor_it);
}

void draw_layout_actor(const LayoutActor* actor) {
    if (actor == nullptr) {
        throw std::invalid_argument("Drawing a layout actor requires an actor");
    }
    if (actor->mLayoutManager == nullptr) {
        throw std::logic_error("Drawing a layout actor requires an initialized layout manager");
    }
    actor->mLayoutManager->draw();
}

void start_layout_anim(LayoutActor* actor, const char* animation_name, u32 layer) {
    auto& manager = require_manager_state(actor != nullptr ? actor->mLayoutManager : nullptr, "Starting a layout animation");
    (void)require_actor_layer(manager, layer, "Starting a layout animation");
    require_layout_runtime(actor, "Starting a layout animation").startAnim(animation_name, layer);
    sync_actor_control_from_runtime(actor, layer);
}

void set_layout_anim_frame(LayoutActor* actor, f32 frame, u32 layer) {
    auto& manager = require_manager_state(actor != nullptr ? actor->mLayoutManager : nullptr, "Setting a layout animation frame");
    (void)require_actor_layer(manager, layer, "Setting a layout animation frame");
    require_layout_runtime(actor, "Setting a layout animation frame").setAnimFrame(frame, layer);
    sync_actor_control_from_runtime(actor, layer);
}

void set_layout_anim_frame_and_stop(LayoutActor* actor, f32 frame, u32 layer) {
    auto& manager = require_manager_state(actor != nullptr ? actor->mLayoutManager : nullptr, "Stopping a layout animation at a frame");
    (void)require_actor_layer(manager, layer, "Stopping a layout animation at a frame");
    require_layout_runtime(actor, "Stopping a layout animation at a frame").setAnimFrameAndStop(frame, layer);
    sync_actor_control_from_runtime(actor, layer);
}

void set_layout_anim_rate(LayoutActor* actor, f32 rate, u32 layer) {
    auto& manager = require_manager_state(actor != nullptr ? actor->mLayoutManager : nullptr, "Setting a layout animation rate");
    (void)require_actor_layer(manager, layer, "Setting a layout animation rate");
    require_layout_runtime(actor, "Setting a layout animation rate").setAnimRate(rate, layer);
    sync_actor_control_from_runtime(actor, layer);
}

f32 layout_anim_frame(const LayoutActor* actor, u32 layer) {
    return require_layout_runtime(actor, "Reading a layout animation frame").getAnimFrame(layer);
}

f32 layout_anim_frame_max(const LayoutActor* actor, u32 layer) {
    return require_layout_runtime(actor, "Reading a layout animation duration").getAnimFrameMax(layer);
}

bool is_layout_anim_stopped(LayoutActor* actor, u32 layer) {
    sync_actor_control_to_runtime(actor, layer);
    const auto stopped = require_layout_runtime(actor, "Reading a layout animation state").isAnimStopped(layer);
    sync_actor_control_from_runtime(actor, layer);
    return stopped;
}

J3DFrameCtrl* layout_anim_ctrl(LayoutActor* actor, u32 layer) {
    auto& manager = require_manager_state(actor != nullptr ? actor->mLayoutManager : nullptr, "Reading a layout animation control");
    (void)require_actor_layer(manager, layer, "Reading a layout animation control");
    if (!require_layout_runtime(actor, "Reading a layout animation control").hasActiveAnimation(layer)) {
        throw std::logic_error("Reading a layout animation control requires an active retail BRLAN");
    }
    sync_actor_control_to_runtime(actor, layer);
    sync_actor_control_from_runtime(actor, layer);
    return &require_actor_state(actor, "Reading a layout animation control").animation_controls[layer];
}

void set_text_box_number(LayoutActor* actor, const char* name, s32 number) {
    require_layout_runtime(actor, "Setting a layout text-box number").setTextBoxNumberRecursive(name, number);
}

void set_text_box_string(LayoutActor* actor, const char* name, std::u16string_view text) {
    require_layout_runtime(actor, "Setting a layout text-box string").setTextBoxStringRecursive(name, text);
}

void set_layout_scale(LayoutActor* actor, f32 x, f32 y) {
    require_layout_runtime(actor, "Setting a layout scale").setScale(x, y);
}

void set_pane_visible(LayoutManager* manager, const char* name, bool visible, bool recursive) {
    auto& runtime = require_runtime(manager, "Changing pane visibility");
    if (recursive) {
        runtime.setPaneVisibleRecursive(pane_name(name), visible);
    } else {
        runtime.setPaneVisible(pane_name(name), visible);
    }
}

bool is_pane_visible(const LayoutManager* manager, const char* name) {
    return require_runtime(manager, "Reading pane visibility").isPaneVisible(pane_name(name));
}

bool is_pointing_pane(const LayoutManager* manager, const char* name, f32 screen_x, f32 screen_y) {
    return require_runtime(manager, "Testing pane pointing").isPointingPane(pane_name(name), screen_x, screen_y);
}

void set_pane_alpha(LayoutManager* manager, const char* name, f32 alpha) {
    require_runtime(manager, "Setting pane alpha").setPaneAlpha(pane_name(name), alpha);
}

void replace_pane_texture(LayoutManager* manager, const char* name, const nw4r::lyt::TexMap& texture, u8 texture_index) {
    require_runtime(manager, "Replacing a pane texture").replacePaneTexture(pane_name(name), texture, texture_index);
}

void set_text_box_tagged_string(LayoutManager* manager, const char* name, std::u16string_view raw_text,
                                std::u16string_view display_text) {
    require_runtime(manager, "Setting tagged text-box content").setTextBoxTaggedStringRecursive(name, raw_text, display_text);
}

void set_text_box_arg_number(LayoutManager* manager, const char* name, s32 number, s32 arg_index) {
    require_runtime(manager, "Setting a text-box number argument").setTextBoxArgNumberRecursive(name, number, arg_index);
}

void set_text_box_arg_string(LayoutManager* manager, const char* name, std::u16string_view text, s32 arg_index) {
    require_runtime(manager, "Setting a text-box string argument").setTextBoxArgStringRecursive(name, text, arg_index);
}

void set_text_box_horizontal_position(LayoutManager* manager, const char* name, u8 position) {
    require_runtime(manager, "Setting text-box horizontal position").setTextBoxHorizontalPosition(pane_name(name), position);
}

void set_text_box_vertical_position(LayoutManager* manager, const char* name, u8 position) {
    require_runtime(manager, "Setting text-box vertical position").setTextBoxVerticalPosition(pane_name(name), position);
}

void set_pane_anim_frame(LayoutPaneCtrl* pane_control, f32 frame, u32 layer) {
    auto& state = require_pane_control_state(pane_control, "Setting a pane animation frame");
    (void)require_pane_layer(state, layer, "Setting a pane animation frame");
    require_runtime(pane_control->mHost, "Setting a pane animation frame").setPaneAnimFrame(state.pane_name, frame, layer);
    state.animation_controls[layer].mFrame = frame;
}

void set_pane_anim_rate(LayoutPaneCtrl* pane_control, f32 rate, u32 layer) {
    auto& state = require_pane_control_state(pane_control, "Setting a pane animation rate");
    (void)require_pane_layer(state, layer, "Setting a pane animation rate");
    require_runtime(pane_control->mHost, "Setting a pane animation rate").setPaneAnimRate(state.pane_name, rate, layer);
    state.animation_controls[layer].mRate = rate;
}

f32 pane_anim_frame(const LayoutPaneCtrl* pane_control, u32 layer) {
    const auto& state = require_pane_control_state(pane_control, "Reading a pane animation frame");
    (void)require_pane_layer(state, layer, "Reading a pane animation frame");
    return require_runtime(pane_control->mHost, "Reading a pane animation frame").getPaneAnimFrame(state.pane_name, layer);
}

f32 pane_anim_frame_max(const LayoutPaneCtrl* pane_control, u32 layer) {
    const auto& state = require_pane_control_state(pane_control, "Reading a pane animation duration");
    (void)require_pane_layer(state, layer, "Reading a pane animation duration");
    if (state.animation_names[layer].empty()) {
        throw std::logic_error("Reading a pane animation duration requires an active retail BRLAN");
    }
    return require_runtime(pane_control->mHost, "Reading a pane animation duration")
        .getAnimDuration(state.animation_names[layer].c_str());
}

f32 animation_duration(const LayoutManager* manager, const char* animation_name) {
    return require_runtime(manager, "Reading layout animation metadata").getAnimDuration(animation_name);
}

f32 pane_animation_frame(const LayoutManager* manager, const char* name, u32 layer) {
    auto* pane_control = manager != nullptr ? manager->getPaneCtrl(name) : nullptr;
    if (pane_control == nullptr) {
        throw std::runtime_error("Reading a pane animation frame requires a real pane control");
    }
    return pane_anim_frame(pane_control, layer);
}

f32 pane_animation_frame_max(const LayoutManager* manager, const char* name, u32 layer) {
    auto* pane_control = manager != nullptr ? manager->getPaneCtrl(name) : nullptr;
    if (pane_control == nullptr) {
        throw std::runtime_error("Reading a pane animation duration requires a real pane control");
    }
    return pane_anim_frame_max(pane_control, layer);
}

bool is_pane_animation_stopped(const LayoutManager* manager, const char* name, u32 layer) {
    auto* pane_control = manager != nullptr ? manager->getPaneCtrl(name) : nullptr;
    if (pane_control == nullptr) {
        throw std::runtime_error("Reading a pane animation state requires a real pane control");
    }
    return pane_control->isAnimStopped(layer);
}

void register_button_controller(LayoutManager* manager, ButtonPaneController* controller) {
    if (controller == nullptr) {
        throw std::invalid_argument("Registering a button controller requires a controller");
    }
    auto& controllers = require_manager_state(manager, "Registering a button controller").button_controllers;
    if (std::ranges::find(controllers, controller) == controllers.end()) {
        controllers.push_back(controller);
    }
}

void unregister_button_controller(LayoutManager* manager, ButtonPaneController* controller) {
    auto& controllers = require_manager_state(manager, "Unregistering a button controller").button_controllers;
    const auto it = std::ranges::find(controllers, controller);
    if (it == controllers.end()) {
        throw std::logic_error("The button controller is not registered with this layout manager");
    }
    controllers.erase(it);
}

void refresh_pane_matrices(LayoutManager* manager) {
    auto& state = require_manager_state(manager, "Refreshing pane matrices");
    for (auto& reference : state.pane_matrix_references) {
        refresh_pane_matrix(state, reference);
        if (!reference.valid) {
            throw std::runtime_error("A registered pane matrix reference lost its retail pane");
        }
    }
}

#ifndef NDEBUG
std::vector< PaneControlDebugState > debug_pane_controls(const LayoutManager* manager) {
    const auto& state = require_manager_state(manager, "Inspecting pane controls");
    auto output = std::vector< PaneControlDebugState >{};
    output.reserve(state.pane_controls.size());
    for (const auto& control : state.pane_controls) {
        const auto& pane = require_pane_control_state(control.get(), "Inspecting pane controls");
        auto debug = PaneControlDebugState{
            .pane_name = pane.pane_name,
            .exists_in_layout = state.runtime != nullptr && state.runtime->hasPane(pane.pane_name),
            .visible = state.runtime != nullptr && state.runtime->isPaneVisible(pane.pane_name),
        };
        debug.animations.reserve(pane.animation_controls.size());
        for (auto layer = u32{}; layer < pane.animation_controls.size(); ++layer) {
            const auto& animation = pane.animation_controls[layer];
            debug.animations.push_back(PaneControlAnimationDebugState{
                .layer_index = layer,
                .name = pane.animation_names[layer],
                .frame = animation.mFrame,
                .end_frame = static_cast< f32 >(animation.mEnd),
                .rate = animation.mRate,
                .stopped = pane.animation_names[layer].empty() ||
                           state.runtime->isPaneAnimStopped(pane.pane_name, layer),
                .looping = animation.mAttribute == J3DFrameCtrl::EMode_LOOP,
            });
        }
        output.push_back(std::move(debug));
    }
    return output;
}

std::vector< ButtonControllerDebugState > debug_button_controllers(const LayoutManager* manager) {
    const auto& state = require_manager_state(manager, "Inspecting button controllers");
    auto output = std::vector< ButtonControllerDebugState >{};
    output.reserve(state.button_controllers.size());
    for (const auto* button : state.button_controllers) {
        if (button == nullptr) {
            throw std::logic_error("A layout manager contains an absent button controller");
        }
        output.push_back(ButtonControllerDebugState{
            .pane_name = button->mPaneName != nullptr ? button->mPaneName : "",
            .bounding_pane_name = button->mBoundingPaneName != nullptr ? button->mBoundingPaneName : "",
            .nerve = button_nerve_name(*button),
            .anim_layer = button->mAnimIndex,
            .active = button->_24,
            .selected = button->mIsSelected,
            .pointing = button->mIsPointing || button->isPointing(),
            .appearance_enabled = button->mAppearAnimName != nullptr || button->mDisappearAnimName != nullptr,
            .decide_enabled = button->mDecideAnimName != nullptr,
            .pointing_anim_start_frame = button->mPointingAnimStartFrame,
        });
    }
    return output;
}
#endif

}  // namespace smgpc::layout
