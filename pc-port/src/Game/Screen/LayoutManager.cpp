#include "Game/Screen/LayoutManager.hpp"

#include <algorithm>

#include "Game/Screen/ButtonPaneController.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/Screen/SimpleLayout.hpp"

LayoutManager::LayoutManager(LayoutActor* pHost) : mHost(pHost), mIsScreenHidden(false), _61(false) {
}

LayoutManager::LayoutManager(const char*, bool, u32, u32) : mHost(nullptr), mIsScreenHidden(false), _61(false) {
}

LayoutManager::~LayoutManager() = default;

void LayoutManager::movement() {
    for (auto& pane_ctrl : mPaneCtrls) {
        pane_ctrl->movement();
    }
}

void LayoutManager::calcAnim() {
    for (auto& pane_ctrl : mPaneCtrls) {
        pane_ctrl->calcAnim();
    }
}

void LayoutManager::draw() const {
}

void LayoutManager::addPaneCtrl(LayoutPaneCtrl* pPaneCtrl) {
    if (pPaneCtrl == nullptr) {
        return;
    }

    mPaneCtrls.emplace_back(pPaneCtrl);
}

LayoutPaneCtrl* LayoutManager::createAndAddRootPaneCtrl(u32 animLayerNum) {
    return createAndAddPaneCtrl(nullptr, animLayerNum);
}

LayoutPaneCtrl* LayoutManager::createAndAddPaneCtrl(const char* pPaneName, u32 animLayerNum) {
    const auto pane_name = normalizedPaneName(pPaneName);
    if (auto* existing = findPaneCtrl(pane_name)) {
        return existing;
    }

    auto pane_ctrl = std::make_unique< LayoutPaneCtrl >(this, pane_name.empty() ? nullptr : pane_name.c_str(), animLayerNum);
    auto* result = pane_ctrl.get();
    mPaneCtrls.push_back(std::move(pane_ctrl));
    return result;
}

LayoutPaneCtrl* LayoutManager::getPaneCtrl(const char* pPaneName) const {
    const auto pane_name = normalizedPaneName(pPaneName);
    if (auto* existing = findPaneCtrl(pane_name)) {
        return existing;
    }

    return const_cast< LayoutManager* >(this)->createAndAddPaneCtrl(pPaneName, 1U);
}

s32 LayoutManager::getIndexOfPane(const char* pPaneName) const {
    const auto* layout = mHost != nullptr ? mHost->getSimpleLayout() : nullptr;
    if (layout == nullptr || !layout->hasPane(normalizedPaneName(pPaneName))) {
        return -1;
    }

    return 0;
}

bool LayoutManager::isExistPaneCtrl(const char* pPaneName) const {
    return findPaneCtrl(normalizedPaneName(pPaneName)) != nullptr;
}

void LayoutManager::showPane(const char* pPaneName) {
    if (auto* layout = mHost != nullptr ? mHost->getSimpleLayout() : nullptr) {
        layout->setPaneVisible(normalizedPaneName(pPaneName), true);
    }
}

void LayoutManager::hidePane(const char* pPaneName) {
    if (auto* layout = mHost != nullptr ? mHost->getSimpleLayout() : nullptr) {
        layout->setPaneVisible(normalizedPaneName(pPaneName), false);
    }
}

void LayoutManager::showPaneRecursive(const char* pPaneName) {
    if (auto* layout = mHost != nullptr ? mHost->getSimpleLayout() : nullptr) {
        layout->setPaneVisibleRecursive(normalizedPaneName(pPaneName), true);
    }
}

void LayoutManager::hidePaneRecursive(const char* pPaneName) {
    if (auto* layout = mHost != nullptr ? mHost->getSimpleLayout() : nullptr) {
        layout->setPaneVisibleRecursive(normalizedPaneName(pPaneName), false);
    }
}

bool LayoutManager::isPaneVisible(const char* pPaneName) const {
    const auto* layout = mHost != nullptr ? mHost->getSimpleLayout() : nullptr;
    return layout == nullptr || layout->isPaneVisible(normalizedPaneName(pPaneName));
}

bool LayoutManager::isPointingPane(const char* pPaneName, f32 screenX, f32 screenY) const {
    const auto* layout = mHost != nullptr ? mHost->getSimpleLayout() : nullptr;
    return layout != nullptr && layout->isPointingPane(normalizedPaneName(pPaneName), screenX, screenY);
}

void LayoutManager::createPaneMtxRef(const char* pPaneName) {
    const auto pane_name = normalizedPaneName(pPaneName);
    if (findPaneMtxRef(pane_name) != nullptr) {
        return;
    }

    auto& pane_ref = mPaneMtxRefs.emplace_back();
    pane_ref.pane_name = pane_name;
    refreshPaneMtxRef(pane_ref);
}

MtxPtr LayoutManager::getPaneMtxRef(const char* pPaneName) const {
    const auto* pane_ref = findPaneMtxRef(normalizedPaneName(pPaneName));
    return pane_ref != nullptr ? const_cast< f32 (*)[4] >(pane_ref->matrix) : nullptr;
}

bool LayoutManager::isExistPaneMtxRef(const char* pPaneName) const {
    return findPaneMtxRef(normalizedPaneName(pPaneName)) != nullptr;
}

void LayoutManager::refreshPaneMtxRefs() {
    for (auto& pane_ref : mPaneMtxRefs) {
        refreshPaneMtxRef(pane_ref);
    }
}

void LayoutManager::startPaneAnim(const char* pPaneName, const char* pAnimName, u32 animLayer) {
    if (auto* layout = mHost != nullptr ? mHost->getSimpleLayout() : nullptr) {
        layout->startPaneAnim(normalizedPaneName(pPaneName), pAnimName, animLayer);
    }
}

void LayoutManager::stopPaneAnim(const char* pPaneName, u32 animLayer) {
    if (auto* layout = mHost != nullptr ? mHost->getSimpleLayout() : nullptr) {
        layout->stopPaneAnim(normalizedPaneName(pPaneName), animLayer);
    }
}

void LayoutManager::setPaneAnimFrame(const char* pPaneName, f32 frame, u32 animLayer) {
    if (auto* layout = mHost != nullptr ? mHost->getSimpleLayout() : nullptr) {
        layout->setPaneAnimFrame(normalizedPaneName(pPaneName), frame, animLayer);
    }
}

void LayoutManager::setPaneAnimRate(const char* pPaneName, f32 rate, u32 animLayer) {
    if (auto* layout = mHost != nullptr ? mHost->getSimpleLayout() : nullptr) {
        layout->setPaneAnimRate(normalizedPaneName(pPaneName), rate, animLayer);
    }
}

f32 LayoutManager::getPaneAnimFrame(const char* pPaneName, u32 animLayer) const {
    if (auto* ctrl = findPaneCtrl(normalizedPaneName(pPaneName))) {
        return ctrl->getFrame(animLayer);
    }

    return 0.0F;
}

bool LayoutManager::isPaneAnimStopped(const char* pPaneName, u32 animLayer) const {
    if (auto* ctrl = findPaneCtrl(normalizedPaneName(pPaneName))) {
        return ctrl->isAnimStopped(animLayer);
    }

    return true;
}

f32 LayoutManager::getPaneAnimFrameMax(const char* pPaneName, u32 animLayer) const {
    if (auto* ctrl = findPaneCtrl(normalizedPaneName(pPaneName))) {
        return static_cast< f32 >(ctrl->getFrameCtrl(animLayer)->mEnd);
    }

    return 0.0F;
}

f32 LayoutManager::getAnimFrameMax(const char* pAnimName) const {
    const auto* layout = mHost != nullptr ? mHost->getSimpleLayout() : nullptr;
    return layout == nullptr ? 1.0F : layout->getAnimDuration(pAnimName);
}

bool LayoutManager::isLoopingAnim(const char* pAnimName) const {
    const auto* layout = mHost != nullptr ? mHost->getSimpleLayout() : nullptr;
    if (layout == nullptr || pAnimName == nullptr) {
        return false;
    }

    return layout->isAnimLooping(pAnimName);
}

void LayoutManager::registerButtonController(ButtonPaneController* pController) {
    if (pController == nullptr || std::ranges::find(mButtonControllers, pController) != mButtonControllers.end()) {
        return;
    }

    mButtonControllers.push_back(pController);
}

void LayoutManager::unregisterButtonController(ButtonPaneController* pController) {
    const auto it = std::ranges::find(mButtonControllers, pController);
    if (it != mButtonControllers.end()) {
        mButtonControllers.erase(it);
    }
}

#ifndef NDEBUG
std::vector< LayoutPaneControlDebugState > LayoutManager::debugPaneControls() const {
    auto out = std::vector< LayoutPaneControlDebugState >{};
    out.reserve(mPaneCtrls.size());
    const auto* layout = mHost != nullptr ? mHost->getSimpleLayout() : nullptr;
    for (const auto& pane_ctrl : mPaneCtrls) {
        const auto pane_name = std::string(pane_ctrl->paneName());
        out.push_back(LayoutPaneControlDebugState{
            .pane_name = pane_name,
            .exists_in_layout = layout != nullptr && layout->hasPane(pane_name),
            .visible = layout == nullptr || layout->isPaneVisible(pane_name),
            .animations = pane_ctrl->debugAnimations(),
        });
    }
    return out;
}

std::vector< LayoutButtonControllerDebugState > LayoutManager::debugButtonControllers() const {
    auto out = std::vector< LayoutButtonControllerDebugState >{};
    out.reserve(mButtonControllers.size());
    for (const auto* button : mButtonControllers) {
        if (button == nullptr) {
            continue;
        }

        out.push_back(LayoutButtonControllerDebugState{
            .pane_name = button->mPaneName != nullptr ? button->mPaneName : "",
            .bounding_pane_name = button->mBoundingPaneName != nullptr ? button->mBoundingPaneName : "",
            .nerve = button->debugNerveName(),
            .anim_layer = button->mAnimIndex,
            .active = button->_24,
            .selected = button->mIsSelected,
            .pointing = button->mIsPointing || button->isPointing(),
            .appearance_enabled = button->mAppearAnimName != nullptr || button->mDisappearAnimName != nullptr,
            .decide_enabled = button->mDecideAnimName != nullptr,
            .pointing_anim_start_frame = button->mPointingAnimStartFrame,
        });
    }
    return out;
}
#endif

std::string LayoutManager::normalizedPaneName(const char* pPaneName) const {
    return pPaneName != nullptr ? std::string(pPaneName) : std::string();
}

LayoutPaneCtrl* LayoutManager::findPaneCtrl(std::string_view paneName) const {
    const auto it = std::ranges::find_if(mPaneCtrls, [paneName](const auto& pane_ctrl) { return pane_ctrl->paneName() == paneName; });
    return it == mPaneCtrls.end() ? nullptr : it->get();
}

LayoutManager::PaneMtxRef* LayoutManager::findPaneMtxRef(std::string_view paneName) {
    const auto it = std::ranges::find_if(mPaneMtxRefs, [paneName](const auto& pane_ref) { return pane_ref.pane_name == paneName; });
    return it == mPaneMtxRefs.end() ? nullptr : &*it;
}

const LayoutManager::PaneMtxRef* LayoutManager::findPaneMtxRef(std::string_view paneName) const {
    const auto it = std::ranges::find_if(mPaneMtxRefs, [paneName](const auto& pane_ref) { return pane_ref.pane_name == paneName; });
    return it == mPaneMtxRefs.end() ? nullptr : &*it;
}

void LayoutManager::refreshPaneMtxRef(PaneMtxRef& paneRef) {
    auto identity = [&]() {
        paneRef.matrix[0][0] = 1.0F;
        paneRef.matrix[0][1] = 0.0F;
        paneRef.matrix[0][2] = 0.0F;
        paneRef.matrix[0][3] = 0.0F;
        paneRef.matrix[1][0] = 0.0F;
        paneRef.matrix[1][1] = 1.0F;
        paneRef.matrix[1][2] = 0.0F;
        paneRef.matrix[1][3] = 0.0F;
        paneRef.matrix[2][0] = 0.0F;
        paneRef.matrix[2][1] = 0.0F;
        paneRef.matrix[2][2] = 1.0F;
        paneRef.matrix[2][3] = 0.0F;
    };

    const auto* layout = mHost != nullptr ? mHost->getSimpleLayout() : nullptr;
    if (layout == nullptr || !layout->copyPaneMatrix(paneRef.pane_name, paneRef.matrix)) {
        identity();
    }
}
