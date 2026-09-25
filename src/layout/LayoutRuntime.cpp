#include <aurora/exception.hpp>
#include "layout/LayoutRuntime.hpp"
#include "compat/JkrAllocationDomain.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "core/RenderTypes.hpp"
#include "Game/Util/EventUtil.hpp"
#include "layout/LayoutResourceResolver.hpp"
#include "Game/System/LayoutHolder.hpp"
#include "Game/System/LayoutHolder.hpp"
#include "layout/LytTexMap.hpp"
#include "layout/Nw4rLayoutRecords.hpp"
#include "Game/Util/DrawUtil.hpp"
#include <nw4r/lyt/pane.h>
#include <nw4r/lyt/drawInfo.h>
#include "nw4r/ut/Font.h"
#include "render/GXState.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeContext.hpp"

namespace {

    [[nodiscard]] bool ends_with(std::string_view text, std::string_view suffix) {
        return text.size() >= suffix.size() && text.substr(text.size() - suffix.size()) == suffix;
    }

    [[nodiscard]] std::string texture_archive_path(std::string_view texture_name) {
        auto path = std::string("timg/");
        path.append(texture_name);
        if (!ends_with(path, ".tpl")) {
            path.append(".tpl");
        }
        std::ranges::transform(path, path.begin(), [](unsigned char character) { return static_cast< char >(std::tolower(character)); });

        return path;
    }

    [[nodiscard]] std::string lower_copy(std::string_view value) {
        auto lower = std::string(value);
        std::ranges::transform(lower, lower.begin(), [](unsigned char character) { return static_cast< char >(std::tolower(character)); });
        return lower;
    }

    [[nodiscard]] std::string base_name(std::string_view path) {
        const auto slash = path.find_last_of('/');
        if (slash == std::string_view::npos) {
            return std::string(path);
        }

        return std::string(path.substr(slash + 1U));
    }

    [[nodiscard]] std::string animation_name_from_path(std::string_view path) {
        auto name = base_name(path);
        if (ends_with(name, ".brlan")) {
            name.resize(name.size() - std::string_view(".brlan").size());
        }
        return lower_copy(name);
    }

    [[nodiscard]] std::string font_resource_name(std::string_view font_name) {
        auto name = lower_copy(base_name(font_name));
        if (!ends_with(name, ".brfnt")) {
            name.append(".brfnt");
        }

        return name;
    }

    [[nodiscard]] std::vector< std::string > font_resource_candidates(std::string_view font_name) {
        auto candidates = std::vector< std::string >{font_resource_name(font_name)};
        const auto dot = candidates.front().rfind(".brfnt");
        const auto stem = candidates.front().substr(0U, dot);
        constexpr std::array< std::string_view, 7U > locale_suffixes{"jpn", "eng", "fra", "ger", "ita", "spa", "kor"};
        for (const auto suffix : locale_suffixes) {
            if (ends_with(stem, suffix) && stem.size() > suffix.size()) {
                candidates.push_back(stem.substr(0U, stem.size() - suffix.size()) + candidates.front().substr(dot));
                break;
            }
        }

        return candidates;
    }

    [[nodiscard]] bool pane_name_matches_request(std::string_view pane_name, std::string_view requested_name) {
        return pane_name == requested_name;
    }

    [[nodiscard]] std::optional<std::size_t> find_preferred_pane_index(const smgpc::layout::BrlytLayout& layout, std::string_view name) {
        if (name.empty()) return std::nullopt;
        const auto found = std::ranges::find(layout.panes, name, &smgpc::layout::BrlytPane::name);
        if (found == layout.panes.end()) return std::nullopt;
        return static_cast<std::size_t>(found - layout.panes.begin());
    }

    [[nodiscard]] bool font_name_matches(std::string_view loaded_name, std::string_view requested_name) {
        const auto loaded_candidates = font_resource_candidates(loaded_name);
        const auto requested_candidates = font_resource_candidates(requested_name);
        return std::ranges::any_of(loaded_candidates, [&requested_candidates](const auto& loaded) {
            return std::ranges::find(requested_candidates, loaded) != requested_candidates.end();
        });
    }

    [[nodiscard]] bool contains_texture(const std::vector< smgpc::layout::LayoutRuntime::RenderTexture >& textures, std::string_view texture_name) {
        return std::ranges::any_of(textures, [texture_name](const auto& texture) { return texture.name == texture_name; });
    }

    [[nodiscard]] smgpc::layout::LayoutRuntime::RenderTexture* find_texture(std::vector< smgpc::layout::LayoutRuntime::RenderTexture >& textures, std::string_view texture_name) {
        const auto it = std::ranges::find_if(textures, [texture_name](const auto& texture) { return texture.name == texture_name; });

        return it == textures.end() ? nullptr : &(*it);
    }

    [[nodiscard]] std::string texture_format_name(smgpc::resource::TplTextureFormat format) {
        switch (format) {
        case smgpc::resource::TplTextureFormat::I4:
            return "I4";
        case smgpc::resource::TplTextureFormat::I8:
            return "I8";
        case smgpc::resource::TplTextureFormat::IA4:
            return "IA4";
        case smgpc::resource::TplTextureFormat::IA8:
            return "IA8";
        case smgpc::resource::TplTextureFormat::RGB565:
            return "RGB565";
        case smgpc::resource::TplTextureFormat::RGB5A3:
            return "RGB5A3";
        case smgpc::resource::TplTextureFormat::RGBA8:
            return "RGBA8";
        case smgpc::resource::TplTextureFormat::C4:
            return "C4";
        case smgpc::resource::TplTextureFormat::C8:
            return "C8";
        case smgpc::resource::TplTextureFormat::C14X2:
            return "C14X2";
        case smgpc::resource::TplTextureFormat::CMPR:
            return "CMPR";
        }

        return "Unknown";
    }

    [[nodiscard]] bool contains_font(const std::vector< smgpc::layout::LayoutRuntime::RenderFont >& fonts, std::string_view font_name) {
        return std::ranges::any_of(fonts, [font_name](const auto& font) { return font_name_matches(font.name, font_name); });
    }

    [[nodiscard]] const smgpc::resource::RarcEntry* find_font_entry(const smgpc::resource::RarcArchive& archive, std::string_view font_name) {
        const auto candidates = font_resource_candidates(font_name);
        const auto it = std::ranges::find_if(archive.entries(), [&candidates](const auto& entry) {
            return std::ranges::find(candidates, font_resource_name(entry.path)) != candidates.end();
        });

        return it == archive.entries().end() ? nullptr : &(*it);
    }

    [[nodiscard]] bool add_render_font_from_archive(
        std::vector<smgpc::layout::LayoutRuntime::RenderFont>& fonts,
        const smgpc::resource::RarcArchive& archive, std::string_view font_name) {
        if (contains_font(fonts, font_name)) {
            return true;
        }
        const auto* entry = find_font_entry(archive, font_name);
        if (entry == nullptr) {
            return false;
        }

        const auto data = archive.file_data(*entry);
        auto bytes = std::make_shared<std::vector<std::uint8_t>>(data.begin(), data.end());
        auto font = std::make_shared<nw4r::ut::ResFont>();
        if (!font->SetResource(bytes->data(), bytes->size())) {
            aurora::throw_host_exception<std::runtime_error>("Invalid BRFNT resource for " + std::string(font_name));
        }
        fonts.push_back(smgpc::layout::LayoutRuntime::RenderFont{
            .name = std::string(font_name),
            .source_bytes = std::move(bytes),
            .native_font = std::move(font),
        });
        return true;
    }

    [[nodiscard]] std::optional< std::filesystem::path >
    find_companion_font_archive(const std::optional< std::filesystem::path >& layout_archive_path) {
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            if (auto path = runtime->find_layout_archive("Font")) {
                return path;
            }
        }

        if (!layout_archive_path.has_value()) {
            return std::nullopt;
        }

        auto candidate = layout_archive_path->parent_path() / "Font.arc";
        std::error_code error{};
        if (std::filesystem::is_regular_file(candidate, error)) {
            return candidate;
        }

        return std::nullopt;
    }

    [[nodiscard]] float base_position_x(std::uint8_t base_position, float width) {
        switch (base_position % 3U) {
        case 1U:
            return -width * 0.5F;
        case 2U:
            return -width;
        default:
            return 0.0F;
        }
    }

    [[nodiscard]] float base_position_y(std::uint8_t base_position, float height) {
        switch (base_position / 3U) {
        case 1U:
            return -height * 0.5F;
        case 2U:
            return -height;
        default:
            return 0.0F;
        }
    }

    [[nodiscard]] std::array< float, 2U > layout_location_adjust_scale() {
        return {0.75F, 1.0F};
    }

    void merge_material_frame(aurora::nw4r::lyt::BrlanMaterialFrame& target,
                              const aurora::nw4r::lyt::BrlanMaterialFrame& source) {
        for (auto component = std::size_t{}; component < target.material_color.size(); ++component) {
            if (source.material_color[component].has_value()) {
                target.material_color[component] = source.material_color[component];
            }
        }
        for (auto color = std::size_t{}; color < target.tev_colors.size(); ++color) {
            for (auto component = std::size_t{}; component < target.tev_colors[color].size(); ++component) {
                if (source.tev_colors[color][component].has_value()) {
                    target.tev_colors[color][component] = source.tev_colors[color][component];
                }
            }
        }
        for (auto color = std::size_t{}; color < target.tev_k_colors.size(); ++color) {
            for (auto component = std::size_t{}; component < target.tev_k_colors[color].size(); ++component) {
                if (source.tev_k_colors[color][component].has_value()) {
                    target.tev_k_colors[color][component] = source.tev_k_colors[color][component];
                }
            }
        }
    }

}  // namespace

smgpc::layout::LayoutRuntime::LayoutRuntime(const char* pName, const char* pLayoutName, u32 animLayerNum, int)
    : mAnimLayerNum(animLayerNum) {
    const smgpc::compat::JkrHostAllocationScope host;
    mName = pName;
    mLayoutName = pLayoutName;
    if (animLayerNum == 0U || animLayerNum > mAnimations.size()) {
        aurora::throw_host_exception<std::invalid_argument>("LayoutRuntime requires between one and four animation layers");
    }
    if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
        mArchivePath = runtime->find_layout_archive(mLayoutName);
        if (mArchivePath.has_value()) {
            runtime->note_layout_archive(mLayoutName, *mArchivePath);
        } else {
            runtime->note_missing_layout_archive(mLayoutName);
        }
    }
}

smgpc::layout::LayoutRuntime::LayoutRuntime(const char* pName, const char* pLayoutName, u32 animLayerNum, int,
                                             std::filesystem::path archivePath)
    : mAnimLayerNum(animLayerNum) {
    const smgpc::compat::JkrHostAllocationScope host;
    mName = pName;
    mLayoutName = pLayoutName;
    mArchivePath = std::move(archivePath);
    if (animLayerNum == 0U || animLayerNum > mAnimations.size()) {
        aurora::throw_host_exception<std::invalid_argument>("LayoutRuntime requires between one and four animation layers");
    }
}

smgpc::layout::LayoutRuntime::LayoutRuntime(const char* pName, const char* pLayoutName, u32 animLayerNum, int,
                                           LayoutHolder& archiveOwner)
    : mAnimLayerNum(animLayerNum), mArchiveOwner(&archiveOwner), mArchiveBorrow(archiveOwner.retainNativeResources()) {
    const compat::JkrHostAllocationScope host;
    mName = pName;
    mLayoutName = pLayoutName;
    if (!mArchiveOwner || animLayerNum == 0U || animLayerNum > mAnimations.size())
        aurora::throw_host_exception<std::invalid_argument>("A mounted layout requires its archive owner and one to four animation layers");
}

smgpc::layout::LayoutRuntime::~LayoutRuntime() {
    const smgpc::compat::JkrHostAllocationScope host;
    mNativeRecords.reset();
}

void smgpc::layout::LayoutRuntime::initWithoutIter() {
}

void smgpc::layout::LayoutRuntime::appear() {
    mIsDead = false;
}

void smgpc::layout::LayoutRuntime::kill() {
    mIsDead = true;
}

void smgpc::layout::LayoutRuntime::update() {
    if (mIsDead) {
        return;
    }

    const auto advance_animation = [](AnimationState& anim) {
        if (anim.name.empty() || anim.stopped) {
            return;
        }

        if (anim.looping) {
            anim.frame += anim.rate;
            if (anim.end > 0.0F && anim.frame >= anim.end) {
                anim.frame = std::fmod(anim.frame, anim.end);
            }
            return;
        }

        anim.frame += anim.rate;
        if (anim.frame >= anim.end || anim.frame <= 0.0F) {
            anim.frame = std::clamp(anim.frame, 0.0F, anim.end);
            anim.stopped = true;
        }
    };

    for (auto& anim : mAnimations) {
        advance_animation(anim);
    }
    for (auto& pane : mPaneAnimations) {
        for (auto& anim : pane.animations) {
            advance_animation(anim);
        }
    }
}

void smgpc::layout::LayoutRuntime::setTrans(f32 x, f32 y) {
    mTransX = x;
    mTransY = y;
}

void smgpc::layout::LayoutRuntime::setScale(f32 x, f32 y) {
    mScaleX = x;
    mScaleY = y;
}

bool smgpc::layout::LayoutRuntime::isDead() const {
    return mIsDead;
}

const std::string& smgpc::layout::LayoutRuntime::getName() const {
    return mName;
}

const std::string& smgpc::layout::LayoutRuntime::getLayoutName() const {
    return mLayoutName;
}

const std::optional< std::filesystem::path >& smgpc::layout::LayoutRuntime::getArchivePath() const {
    return mArchivePath;
}

smgpc::layout::Nw4rLayoutRecords& smgpc::layout::LayoutRuntime::native_records() {
    const smgpc::compat::JkrHostAllocationScope host;
    if (!mNativeRecords) mNativeRecords = std::make_unique<Nw4rLayoutRecords>(*this);
    return *mNativeRecords;
}

void smgpc::layout::LayoutRuntime::draw() {
    const smgpc::compat::JkrHostAllocationScope host;
    if (mIsDead) return;
    auto& records = native_records();
    records.synchronize();
    MR::setupDrawForNW4RLayout(1.0f, true);
    nw4r::lyt::DrawInfo info;
    info.mViewRect = {-mBrlytLayout.width * 0.5f, mBrlytLayout.height * 0.5f,
                      mBrlytLayout.width * 0.5f, -mBrlytLayout.height * 0.5f};
    records.pane(nullptr)->Draw(info);
}

void smgpc::layout::LayoutRuntime::startAnim(const char* pAnimName, u32 animLayer) {
    const smgpc::compat::JkrHostAllocationScope host;
    loadRenderData();
    const auto end = durationFor(pAnimName);
    const auto looping = isLoopingAnim(pAnimName);
    auto& anim = animation(animLayer);
    commitAnimationState(anim);
    anim.name = pAnimName;
    anim.frame = 0.0f;
    anim.end = end;
    anim.rate = 1.0f;
    anim.looping = looping;
    anim.stopped = false;
}

void smgpc::layout::LayoutRuntime::setAnimFrameAndStop(f32 frame, u32 animLayer) {
    const smgpc::compat::JkrHostAllocationScope host;
    auto& anim = animationControl(animLayer);
    anim.frame = frame;
    anim.rate = 0.0f;
    anim.stopped = true;
}

void smgpc::layout::LayoutRuntime::setAnimFrame(f32 frame, u32 animLayer) {
    const smgpc::compat::JkrHostAllocationScope host;
    auto& anim = animationControl(animLayer);
    anim.frame = frame;
}

void smgpc::layout::LayoutRuntime::setAnimRate(f32 rate, u32 animLayer) {
    const smgpc::compat::JkrHostAllocationScope host;
    auto& anim = animationControl(animLayer);
    anim.rate = rate;
    anim.stopped = anim.name.empty() || rate == 0.0f;
}

f32 smgpc::layout::LayoutRuntime::getAnimFrame(u32 animLayer) const {
    const smgpc::compat::JkrHostAllocationScope host;
    const auto& anim = animationControl(animLayer);
    return anim.frame;
}

bool smgpc::layout::LayoutRuntime::isAnimStopped(u32 animLayer) {
    const smgpc::compat::JkrHostAllocationScope host;
    auto& anim = animationControl(animLayer);
    return anim.name.empty() || anim.stopped;
}

void smgpc::layout::LayoutRuntime::setPaneScale(std::string_view paneName, f32 x, f32 y) {
    smgpc::compat::JkrHostAllocationScope host;
    loadRenderData();
    const auto index = paneName.empty() ? std::optional<std::size_t>(0) : find_preferred_pane_index(mBrlytLayout, paneName);
    if (!index || *index >= mBrlytLayout.panes.size())
        aurora::throw_host_exception<std::runtime_error>("Setting scale requires an existing layout pane");
    auto& values = mCommittedPaneFrames[mBrlytLayout.panes[*index].name];
    values.scale_x = x;
    values.scale_y = y;
}

void smgpc::layout::LayoutRuntime::setPaneRotation(std::string_view paneName, f32 x, f32 y, f32 z) {
    smgpc::compat::JkrHostAllocationScope host;
    loadRenderData();
    const auto index = paneName.empty() ? std::optional<std::size_t>(0) : find_preferred_pane_index(mBrlytLayout, paneName);
    if (!index || *index >= mBrlytLayout.panes.size())
        aurora::throw_host_exception<std::runtime_error>("Setting rotation requires an existing layout pane");
    auto& frame = mCommittedPaneFrames[mBrlytLayout.panes[*index].name];
    frame.rotate_x = x;
    frame.rotate_y = y;
    frame.rotate_z = z;
}

void smgpc::layout::LayoutRuntime::setPaneAlpha(std::string_view paneName, f32 alpha) {
    const smgpc::compat::JkrHostAllocationScope host;
    loadRenderData();
    if (paneName.empty()) {
        aurora::throw_host_exception<std::invalid_argument>("Setting pane alpha requires a real pane name");
    }

    const auto alpha_u8 = std::clamp(alpha, 0.0F, 1.0F) * 255.0F;
    auto matched = false;
    for (const auto& pane : mBrlytLayout.panes) {
        if (pane_name_matches_request(pane.name, paneName)) {
            mPaneAlphaOverrides[pane.name] = alpha_u8;
            matched = true;
        }
    }
    if (!matched) {
        aurora::throw_host_exception<std::runtime_error>("Layout " + mLayoutName + " has no pane " + std::string(paneName));
    }
}

void smgpc::layout::LayoutRuntime::setPaneVisible(std::string_view paneName, bool visible) {
    const smgpc::compat::JkrHostAllocationScope host;
    loadRenderData();
    if (paneName.empty()) {
        aurora::throw_host_exception<std::invalid_argument>("Setting pane visibility requires a real pane name");
    }

    auto matched = false;
    for (const auto& pane : mBrlytLayout.panes) {
        if (pane_name_matches_request(pane.name, paneName)) {
            mPaneVisibilityOverrides[pane.name] = visible;
            matched = true;
        }
    }
    if (!matched) {
        aurora::throw_host_exception<std::runtime_error>("Layout " + mLayoutName + " has no pane " + std::string(paneName));
    }
}

void smgpc::layout::LayoutRuntime::setPaneVisibleRecursive(std::string_view paneName, bool visible) {
    const smgpc::compat::JkrHostAllocationScope host;
    loadRenderData();
    if (paneName.empty()) {
        aurora::throw_host_exception<std::invalid_argument>("Setting recursive pane visibility requires a real pane name");
    }

    auto root_indices = std::vector< std::size_t >{};
    for (auto i = std::size_t{}; i < mBrlytLayout.panes.size(); ++i) {
        if (pane_name_matches_request(mBrlytLayout.panes[i].name, paneName)) {
            root_indices.push_back(i);
        }
    }
    if (root_indices.empty()) {
        aurora::throw_host_exception<std::runtime_error>("Layout " + mLayoutName + " has no pane " + std::string(paneName));
    }

    auto is_descendant_of = [&](std::size_t pane_index, std::size_t root_index) {
        auto current = static_cast< int >(pane_index);
        while (current >= 0) {
            if (static_cast< std::size_t >(current) == root_index) {
                return true;
            }
            current = mBrlytLayout.panes[static_cast< std::size_t >(current)].parent_index;
        }
        return false;
    };

    for (auto i = std::size_t{}; i < mBrlytLayout.panes.size(); ++i) {
        if (std::ranges::any_of(root_indices, [&](std::size_t root_index) { return is_descendant_of(i, root_index); })) {
            mPaneVisibilityOverrides[mBrlytLayout.panes[i].name] = visible;
        }
    }
}

bool smgpc::layout::LayoutRuntime::isPaneVisible(std::string_view paneName) const {
    const smgpc::compat::JkrHostAllocationScope host;
    const_cast< LayoutRuntime* >(this)->loadRenderData();
    if (mIsDead) {
        return false;
    }
    if (paneName.empty()) {
        return !mBrlytLayout.panes.empty();
    }

    if (const auto pane_index = find_preferred_pane_index(mBrlytLayout, paneName)) {
        return paneRenderState(*pane_index).visible;
    }

    return false;
}

bool smgpc::layout::LayoutRuntime::isPaneLocallyVisible(std::string_view paneName) const {
    const smgpc::compat::JkrHostAllocationScope host;
    const auto index = paneIndex(paneName);
    if (!index)
        aurora::throw_host_exception<std::runtime_error>("Reading visibility requires a real pane");
    const auto& pane = mBrlytLayout.panes.at(*index);
    auto visible = pane.visible;
    if (const auto animated = animationFrameForPane(pane.name).visible)
        visible = *animated;
    if (const auto override = mPaneVisibilityOverrides.find(pane.name); override != mPaneVisibilityOverrides.end())
        visible = override->second;
    return visible;
}

void smgpc::layout::LayoutRuntime::clearPaneFollowPositions() {
    mPaneFollowPositions.clear();
}

void smgpc::layout::LayoutRuntime::setPaneFollowPosition(std::string_view paneName, u32 type, const TVec2f& position) {
    const smgpc::compat::JkrHostAllocationScope host;
    const auto index = paneIndex(paneName);
    if (!index)
        aurora::throw_host_exception<std::runtime_error>("Following a position requires a real pane");
    // Original recalcChildGlobalMtx discards earlier child follow transforms
    // when a parent control is processed later in the same calculation.
    std::erase_if(mPaneFollowPositions, [&](const auto& entry) {
        auto parent = mBrlytLayout.panes.at(entry.first).parent_index;
        while (parent >= 0) {
            if (static_cast<std::size_t>(parent) == *index)
                return true;
            parent = mBrlytLayout.panes.at(static_cast<std::size_t>(parent)).parent_index;
        }
        return false;
    });
    mPaneFollowPositions.insert_or_assign(*index, PaneFollowState{type, position});
}

bool smgpc::layout::LayoutRuntime::hasPane(std::string_view paneName) const {
    const smgpc::compat::JkrHostAllocationScope host;
    const_cast< LayoutRuntime* >(this)->loadRenderData();
    if (paneName.empty()) {
        return !mBrlytLayout.panes.empty();
    }

    return find_preferred_pane_index(mBrlytLayout, paneName).has_value();
}

std::optional< std::size_t > smgpc::layout::LayoutRuntime::paneIndex(std::string_view paneName) const {
    const smgpc::compat::JkrHostAllocationScope host;
    const_cast< LayoutRuntime* >(this)->loadRenderData();
    if (paneName.empty()) {
        if (mBrlytLayout.panes.empty()) {
            return std::nullopt;
        }
        return 0U;
    }
    return find_preferred_pane_index(mBrlytLayout, paneName);
}

std::optional< smgpc::layout::LayoutRuntime::PaneBounds > smgpc::layout::LayoutRuntime::paneBounds(std::string_view paneName) const {
    const smgpc::compat::JkrHostAllocationScope host;
    const_cast< LayoutRuntime* >(this)->loadRenderData();
    if (mIsDead) {
        return std::nullopt;
    }

    if (paneName.empty()) {
        if (mBrlytLayout.panes.empty() || mBrlytLayout.width <= 0.0F || mBrlytLayout.height <= 0.0F) {
            return std::nullopt;
        }
        const auto half_width = mBrlytLayout.width * 0.5F;
        const auto half_height = mBrlytLayout.height * 0.5F;
        return PaneBounds{
            .left = -half_width,
            .top = -half_height,
            .right = half_width,
            .bottom = half_height,
        };
    }

    const auto pane_index = find_preferred_pane_index(mBrlytLayout, paneName);
    if (!pane_index.has_value()) {
        return std::nullopt;
    }

    const auto& pane = mBrlytLayout.panes[*pane_index];
    const auto pane_state = paneRenderState(*pane_index);
    if (!pane_state.visible || pane_state.alpha <= 0.0F) {
        return std::nullopt;
    }

    const auto frame = animationFrameForPane(pane.name);
    const auto width = frame.width.value_or(pane.width);
    const auto height = frame.height.value_or(pane.height);
    if (width == 0.0F || height == 0.0F) {
        return std::nullopt;
    }

    const auto local_left = base_position_x(pane.base_position, width);
    const auto local_top = base_position_y(pane.base_position, height);
    const auto corners = std::array< std::array< float, 3U >, 4U >{
        panePointForAurora(pane_state, local_left, local_top),
        panePointForAurora(pane_state, local_left + width, local_top),
        panePointForAurora(pane_state, local_left + width, local_top + height),
        panePointForAurora(pane_state, local_left, local_top + height),
    };
    auto left = corners.front()[0U];
    auto right = corners.front()[0U];
    auto top = corners.front()[1U];
    auto bottom = corners.front()[1U];
    for (const auto& corner : corners) {
        left = std::min(left, corner[0U]);
        right = std::max(right, corner[0U]);
        top = std::min(top, corner[1U]);
        bottom = std::max(bottom, corner[1U]);
    }
    return PaneBounds{
        .left = left,
        .top = top,
        .right = right,
        .bottom = bottom,
    };
}

std::optional< TVec2f > smgpc::layout::LayoutRuntime::paneScale(std::string_view paneName) const {
    const smgpc::compat::JkrHostAllocationScope host;
    const_cast< LayoutRuntime* >(this)->loadRenderData();
    if (paneName.empty()) {
        if (mBrlytLayout.panes.empty()) {
            return std::nullopt;
        }
        return TVec2f{mScaleX, mScaleY};
    }

    const auto pane_index = find_preferred_pane_index(mBrlytLayout, paneName);
    if (!pane_index.has_value()) {
        return std::nullopt;
    }

    const auto pane_state = paneRenderState(*pane_index);
    const auto& matrix = pane_state.matrix;
    return TVec2f{std::hypot(matrix[0][0], matrix[1][0], matrix[2][0]),
                  std::hypot(matrix[0][1], matrix[1][1], matrix[2][1])};
}

bool smgpc::layout::LayoutRuntime::copyPaneMatrix(std::string_view paneName, Mtx matrix) const {
    const smgpc::compat::JkrHostAllocationScope host;
    if (matrix == nullptr) {
        return false;
    }

    const_cast< LayoutRuntime* >(this)->loadRenderData();

    auto set_matrix = [&](const PaneRenderState& pane_state) {
        PSMTXCopy(pane_state.matrix, matrix);
        matrix[0][3] += mTransX;
        matrix[1][3] += mTransY;
    };

    if (paneName.empty()) {
        if (mBrlytLayout.panes.empty()) {
            return false;
        }
        set_matrix(paneRenderState(0U));
        return true;
    }

    const auto pane_index = find_preferred_pane_index(mBrlytLayout, paneName);
    if (!pane_index.has_value()) {
        return false;
    }

    set_matrix(paneRenderState(*pane_index));
    return true;
}

bool smgpc::layout::LayoutRuntime::isPointingPane(std::string_view paneName, f32 screenX, f32 screenY) const {
    const auto bounds = paneBounds(paneName);
    if (!bounds.has_value()) {
        return false;
    }

    const auto layout_x = screenX * static_cast< f32 >(smgpc::render::core::kWiiLayoutWidth) /
                              static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferWidth) -
                          static_cast< f32 >(smgpc::render::core::kWiiLayoutWidth) * 0.5F;
    const auto layout_y = -(screenY - static_cast< f32 >(smgpc::render::core::kWiiLogicalFramebufferHeight) * 0.5F);
    return layout_x >= bounds->left && layout_x <= bounds->right && layout_y >= bounds->top && layout_y <= bounds->bottom;
}

void smgpc::layout::LayoutRuntime::startPaneAnim(std::string_view paneName, const char* pAnimName, u32 animLayer) {
    const smgpc::compat::JkrHostAllocationScope host;
    if (paneName.empty() || pAnimName == nullptr || pAnimName[0] == '\0') {
        aurora::throw_host_exception<std::logic_error>("Pane animation requires a real pane and BRLAN name");
    }

    loadRenderData();
    if (!hasPane(paneName)) {
        aurora::throw_host_exception<std::runtime_error>("Layout " + mLayoutName + " has no pane " + std::string(paneName));
    }
    const auto end = durationFor(pAnimName);
    const auto looping = isLoopingAnim(pAnimName);
    // LayoutPaneCtrl validates its own declared layer count. Named panes have
    // independent players and do not share the root layout layer limit.
    auto& anim = paneAnimation(paneName).animations.at(animLayer);
    anim.name = pAnimName;
    anim.frame = 0.0F;
    anim.end = end;
    anim.rate = 1.0F;
    anim.looping = looping;
    anim.stopped = false;
}

void smgpc::layout::LayoutRuntime::stopPaneAnim(std::string_view paneName, u32 animLayer) {
    const smgpc::compat::JkrHostAllocationScope host;
    if (paneName.empty()) {
        aurora::throw_host_exception<std::logic_error>("Pane animation requires a real pane name");
    }

    auto& anim = paneAnimation(paneName).animations.at(animLayer);
    anim.rate = 0.0F;
    anim.stopped = true;
}

void smgpc::layout::LayoutRuntime::setPaneAnimFrame(std::string_view paneName, f32 frame, u32 animLayer) {
    const smgpc::compat::JkrHostAllocationScope host;
    if (paneName.empty()) {
        aurora::throw_host_exception<std::logic_error>("Pane animation requires a real pane name");
    }

    auto& anim = paneAnimation(paneName).animations.at(animLayer);
    anim.frame = frame;
}

void smgpc::layout::LayoutRuntime::setPaneAnimRate(std::string_view paneName, f32 rate, u32 animLayer) {
    const smgpc::compat::JkrHostAllocationScope host;
    if (paneName.empty()) {
        aurora::throw_host_exception<std::logic_error>("Pane animation requires a real pane name");
    }

    auto& anim = paneAnimation(paneName).animations.at(animLayer);
    anim.rate = rate;
    anim.stopped = anim.name.empty() || rate == 0.0F;
}

f32 smgpc::layout::LayoutRuntime::getPaneAnimFrame(std::string_view paneName, u32 animLayer) const {
    const smgpc::compat::JkrHostAllocationScope host;
    const auto& anim = const_cast<LayoutRuntime*>(this)->paneAnimation(paneName).animations.at(animLayer);
    return anim.frame;
}

bool smgpc::layout::LayoutRuntime::isPaneAnimStopped(std::string_view paneName, u32 animLayer) const {
    const smgpc::compat::JkrHostAllocationScope host;
    const auto& anim = const_cast<LayoutRuntime*>(this)->paneAnimation(paneName).animations.at(animLayer);
    return anim.name.empty() || anim.stopped;
}

f32 smgpc::layout::LayoutRuntime::getAnimFrameMax(u32 animLayer) const {
    const smgpc::compat::JkrHostAllocationScope host;
    const auto& anim = animationControl(animLayer);
    return anim.end;
}

f32 smgpc::layout::LayoutRuntime::getAnimRate(u32 animLayer) const {
    const smgpc::compat::JkrHostAllocationScope host;
    const auto& anim = animationControl(animLayer);
    return anim.rate;
}

bool smgpc::layout::LayoutRuntime::hasActiveAnimation(u32 animLayer) const {
    return !animation(animLayer).name.empty();
}

f32 smgpc::layout::LayoutRuntime::getAnimDuration(const char* pAnimName) const {
    const smgpc::compat::JkrHostAllocationScope host;
    if (pAnimName == nullptr || pAnimName[0] == '\0') {
        aurora::throw_host_exception<std::logic_error>("Animation duration requires a real BRLAN name");
    }

    const_cast< LayoutRuntime* >(this)->loadRenderData();
    return durationFor(pAnimName);
}

bool smgpc::layout::LayoutRuntime::isAnimLooping(const char* pAnimName) const {
    const smgpc::compat::JkrHostAllocationScope host;
    if (pAnimName == nullptr || pAnimName[0] == '\0') {
        aurora::throw_host_exception<std::logic_error>("Animation loop state requires a real BRLAN name");
    }

    const_cast< LayoutRuntime* >(this)->loadRenderData();
    return isLoopingAnim(pAnimName);
}

bool smgpc::layout::LayoutRuntime::isAnimLooping(u32 animLayer) const {
    const smgpc::compat::JkrHostAllocationScope host;
    const auto& anim = animationControl(animLayer);
    return anim.looping;
}

#ifndef NDEBUG
f32 smgpc::layout::LayoutRuntime::debugPaneAnimEndFrame(std::string_view paneName, u32 animLayer) const {
    const auto* pane = findPaneAnimation(paneName);
    if (pane == nullptr) {
        return 0.0F;
    }

    return pane->animations.at(animLayer).end;
}

std::size_t smgpc::layout::LayoutRuntime::debugAnimLayerCount() const {
    return mAnimLayerNum;
}

std::string_view smgpc::layout::LayoutRuntime::debugAnimName(u32 animLayer) const {
    return animation(animLayer).name;
}

f32 smgpc::layout::LayoutRuntime::debugAnimDuration(const char* pAnimName) const {
    return getAnimDuration(pAnimName);
}

bool smgpc::layout::LayoutRuntime::debugAnimLooping(const char* pAnimName) const {
    return isAnimLooping(pAnimName);
}

f32 smgpc::layout::LayoutRuntime::debugAnimEndFrame(u32 animLayer) const {
    return getAnimFrameMax(animLayer);
}

f32 smgpc::layout::LayoutRuntime::debugAnimRate(u32 animLayer) const {
    return getAnimRate(animLayer);
}

bool smgpc::layout::LayoutRuntime::debugAnimLooping(u32 animLayer) const {
    return isAnimLooping(animLayer);
}

bool smgpc::layout::LayoutRuntime::debugAnimStopped(u32 animLayer) const {
    return animation(animLayer).stopped;
}

std::size_t smgpc::layout::LayoutRuntime::debugPaneCount() const {
    return mBrlytLayout.panes.size();
}

std::size_t smgpc::layout::LayoutRuntime::debugPictureCount() const {
    return mBrlytLayout.pictures.size();
}

std::size_t smgpc::layout::LayoutRuntime::debugTextBoxCount() const {
    return mBrlytLayout.text_boxes.size();
}

std::size_t smgpc::layout::LayoutRuntime::debugMaterialCount() const {
    return mBrlytLayout.materials.size();
}

std::size_t smgpc::layout::LayoutRuntime::debugTextureCount() const {
    return mRenderTextures.size();
}

std::size_t smgpc::layout::LayoutRuntime::debugFontCount() const {
    return mRenderFonts.size();
}

std::size_t smgpc::layout::LayoutRuntime::debugCommittedPaneFrameCount() const {
    return mCommittedPaneFrames.size();
}

std::vector< smgpc::layout::LayoutRuntime::DebugPaneState > smgpc::layout::LayoutRuntime::debugPanes() const {
    const smgpc::compat::JkrHostAllocationScope host;
    auto states = std::vector< DebugPaneState >{};
    states.reserve(mBrlytLayout.panes.size());

    for (auto pane_index = std::size_t{}; pane_index < mBrlytLayout.panes.size(); ++pane_index) {
        const auto& pane = mBrlytLayout.panes[pane_index];
        const auto render_state = paneRenderState(pane_index);
        const auto frame = animationFrameForPane(pane.name);
        auto state = DebugPaneState{
            .index = pane_index,
            .name = pane.name,
            .parent_index = pane.parent_index,
            .base_visible = pane.visible,
            .effective_visible = render_state.visible,
            .translate_x = render_state.matrix[0][3],
            .translate_y = render_state.matrix[1][3],
            .scale_x = std::hypot(render_state.matrix[0][0], render_state.matrix[1][0], render_state.matrix[2][0]),
            .scale_y = std::hypot(render_state.matrix[0][1], render_state.matrix[1][1], render_state.matrix[2][1]),
            .alpha = render_state.alpha,
            .width = frame.width.value_or(pane.width),
            .height = frame.height.value_or(pane.height),
            .contents = {},
        };

        for (const auto& picture : mBrlytLayout.pictures) {
            if (picture.pane_index != pane_index) {
                continue;
            }

            auto content = DebugPaneContentState{
                .kind = "picture",
                .name = picture.name,
                .material_index = static_cast< s32 >(picture.material_index),
                .material_name = {},
                .texture_name = picture.texture_name,
                .font_name = {},
                .visible = picture.visible,
            };
            if (picture.material_index < mBrlytLayout.materials.size()) {
                const auto& material = mBrlytLayout.materials[picture.material_index];
                content.material_name = material.name;
                if (!material.textures.empty()) {
                    content.texture_name = material.textures.front().texture_name;
                }
            }
            state.contents.push_back(std::move(content));
        }

        for (const auto& text_box : mBrlytLayout.text_boxes) {
            if (text_box.pane_index != pane_index) {
                continue;
            }

            auto content = DebugPaneContentState{
                .kind = "text_box",
                .name = text_box.name,
                .material_index = static_cast< s32 >(text_box.material_index),
                .material_name = {},
                .texture_name = {},
                .font_name = text_box.font_name,
                .visible = text_box.visible,
            };
            if (text_box.material_index < mBrlytLayout.materials.size()) {
                const auto& material = mBrlytLayout.materials[text_box.material_index];
                content.material_name = material.name;
                if (!material.textures.empty()) {
                    content.texture_name = material.textures.front().texture_name;
                }
            }
            state.contents.push_back(std::move(content));
        }

        states.push_back(std::move(state));
    }

    return states;
}

std::vector< smgpc::layout::LayoutRuntime::DebugMaterialState > smgpc::layout::LayoutRuntime::debugMaterials() const {
    const smgpc::compat::JkrHostAllocationScope host;
    auto states = std::vector< DebugMaterialState >{};
    states.reserve(mBrlytLayout.materials.size());

    for (auto material_index = std::size_t{}; material_index < mBrlytLayout.materials.size(); ++material_index) {
        const auto& material = mBrlytLayout.materials[material_index];
        auto state = DebugMaterialState{
            .index = material_index,
            .name = material.name,
            .texture_count = material.textures.size(),
            .tex_coord_gen_count = material.tex_coord_gens.size(),
            .tev_stage_count = material.tev_stages.size(),
            .alpha_compare_enabled = material.alpha_compare.enabled,
            .blend_enabled = material.blend_mode.enabled,
            .textures = {},
        };
        state.textures.reserve(material.textures.size());
        for (auto slot = std::size_t{}; slot < material.textures.size(); ++slot) {
            const auto& texture = material.textures[slot];
            state.textures.push_back(DebugMaterialTextureState{
                .slot = slot,
                .texture_index = texture.texture_index,
                .texture_name = texture.texture_name,
                .wrap_s = texture.wrap_s,
                .wrap_t = texture.wrap_t,
                .min_filter = texture.min_filter,
                .mag_filter = texture.mag_filter,
            });
        }
        states.push_back(std::move(state));
    }

    return states;
}

std::vector< smgpc::layout::LayoutRuntime::DebugTextureState > smgpc::layout::LayoutRuntime::debugTextures() const {
    const smgpc::compat::JkrHostAllocationScope host;
    auto states = std::vector< DebugTextureState >{};
    states.reserve(mRenderTextures.size());

    for (auto texture_index = std::size_t{}; texture_index < mRenderTextures.size(); ++texture_index) {
        const auto& texture = mRenderTextures[texture_index];
        states.push_back(DebugTextureState{
            .index = texture_index,
            .name = texture.name,
            .width = texture.decoded.width,
            .height = texture.decoded.height,
            .format_raw = static_cast< std::uint32_t >(texture.decoded.format),
            .format_name = texture_format_name(texture.decoded.format),
            .uploaded = texture.handle.is_valid(),
            .rgba_byte_count = texture.decoded.rgba.size(),
        });
    }

    return states;
}

#endif

smgpc::layout::LayoutRuntime::AnimationState& smgpc::layout::LayoutRuntime::animation(u32 animLayer) {
    const smgpc::compat::JkrHostAllocationScope host;
    if (animLayer >= mAnimLayerNum) {
        aurora::throw_host_exception<std::out_of_range>("Layout " + mLayoutName + " has no animation layer " + std::to_string(animLayer));
    }
    return mAnimations.at(animLayer);
}

const smgpc::layout::LayoutRuntime::AnimationState& smgpc::layout::LayoutRuntime::animation(u32 animLayer) const {
    const smgpc::compat::JkrHostAllocationScope host;
    if (animLayer >= mAnimLayerNum) {
        aurora::throw_host_exception<std::out_of_range>("Layout " + mLayoutName + " has no animation layer " + std::to_string(animLayer));
    }
    return mAnimations.at(animLayer);
}

smgpc::layout::LayoutRuntime::AnimationState& smgpc::layout::LayoutRuntime::animationControl(u32 animLayer) {
    const smgpc::compat::JkrHostAllocationScope host;
    auto& control = animation(animLayer);
    // An initialized LayoutAnmPlayer owns its frame controller before any BRLAN
    // starts. A missing layout still has no actual root player to control.
    if (!hasPane("")) {
        aurora::throw_host_exception<std::runtime_error>("Layout " + mLayoutName + " has no initialized root animation controller");
    }
    return control;
}

const smgpc::layout::LayoutRuntime::AnimationState& smgpc::layout::LayoutRuntime::animationControl(u32 animLayer) const {
    return const_cast<LayoutRuntime*>(this)->animationControl(animLayer);
}

smgpc::layout::LayoutRuntime::PaneAnimationState& smgpc::layout::LayoutRuntime::paneAnimation(std::string_view paneName) {
    const smgpc::compat::JkrHostAllocationScope host;
    loadRenderData();
    const auto pane_index = find_preferred_pane_index(mBrlytLayout, paneName);
    if (!pane_index.has_value()) {
        aurora::throw_host_exception<std::runtime_error>("Layout " + mLayoutName + " has no pane " + std::string(paneName));
    }
    const auto& resolved_name = mBrlytLayout.panes[*pane_index].name;
    const auto it = std::ranges::find_if(mPaneAnimations, [&resolved_name](const auto& pane) { return pane.pane_name == resolved_name; });
    if (it != mPaneAnimations.end()) {
        return *it;
    }

    mPaneAnimations.push_back(PaneAnimationState{.pane_name = resolved_name});
    return mPaneAnimations.back();
}

const smgpc::layout::LayoutRuntime::PaneAnimationState* smgpc::layout::LayoutRuntime::findPaneAnimation(std::string_view paneName) const {
    const smgpc::compat::JkrHostAllocationScope host;
    const_cast< LayoutRuntime* >(this)->loadRenderData();
    const auto pane_index = find_preferred_pane_index(mBrlytLayout, paneName);
    if (!pane_index.has_value()) {
        return nullptr;
    }
    const auto& resolved_name = mBrlytLayout.panes[*pane_index].name;
    const auto it = std::ranges::find_if(mPaneAnimations, [&resolved_name](const auto& pane) { return pane.pane_name == resolved_name; });
    if (it != mPaneAnimations.end()) {
        return &*it;
    }

    return nullptr;
}

void smgpc::layout::LayoutRuntime::commitAnimationState(const AnimationState& anim) {
    if (anim.name.empty()) {
        return;
    }

    const auto it = mRenderAnimations.find(lower_copy(anim.name));
    if (it == mRenderAnimations.end()) {
        return;
    }

    auto frame = anim.frame;
    if (it->second.loop && it->second.frame_size > 0U) {
        frame = std::fmod(frame, static_cast< float >(it->second.frame_size));
    }

    for (const auto& content : it->second.contents) {
        const auto pane_frame = it->second.pane_frame(content.name, frame);
        auto& committed = mCommittedPaneFrames[content.name];
        if (pane_frame.translate_x.has_value()) {
            committed.translate_x = pane_frame.translate_x;
        }
        if (pane_frame.translate_y.has_value()) {
            committed.translate_y = pane_frame.translate_y;
        }
        if (pane_frame.translate_z.has_value()) {
            committed.translate_z = pane_frame.translate_z;
        }
        if (pane_frame.scale_x.has_value()) {
            committed.scale_x = pane_frame.scale_x;
        }
        if (pane_frame.scale_y.has_value()) {
            committed.scale_y = pane_frame.scale_y;
        }
        if (pane_frame.rotate_x.has_value()) {
            committed.rotate_x = pane_frame.rotate_x;
        }
        if (pane_frame.rotate_y.has_value()) {
            committed.rotate_y = pane_frame.rotate_y;
        }
        if (pane_frame.rotate_z.has_value()) {
            committed.rotate_z = pane_frame.rotate_z;
        }
        if (pane_frame.width.has_value()) {
            committed.width = pane_frame.width;
        }
        if (pane_frame.height.has_value()) {
            committed.height = pane_frame.height;
        }
        if (pane_frame.alpha.has_value()) {
            committed.alpha = pane_frame.alpha;
        }
        if (pane_frame.visible.has_value()) {
            committed.visible = pane_frame.visible;
        }
        merge_material_frame(mCommittedMaterialFrames[content.name], it->second.material_frame(content.name, frame));
    }
}

void smgpc::layout::LayoutRuntime::loadRenderData() {
    const smgpc::compat::JkrHostAllocationScope host;
    if (mRenderDataLoaded) {
        return;
    }

    mRenderDataLoaded = true;
    if (!mArchiveOwner && !mArchivePath.has_value()) {
        return;
    }

    try {
        auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        auto local_archive = std::optional< smgpc::resource::RarcArchive >{};
        const auto* archive = static_cast< const smgpc::resource::RarcArchive* >(nullptr);
        if (mArchiveOwner) {
            archive = &mArchiveOwner->nativeResourceSource();
        } else if (runtime != nullptr) {
            archive = &runtime->dvd().archive_for_path(*mArchivePath);
        } else {
            local_archive = smgpc::resource::RarcArchive::from_file(*mArchivePath);
            archive = &*local_archive;
        }

        const auto* brlyt_entry = mArchiveOwner ? archive->find_by_basename(mLayoutName + ".brlyt")
            : smgpc::layout::find_layout_brlyt(archive->entries(), mLayoutName);
        if (brlyt_entry == nullptr) {
            if (mArchiveOwner)
                aurora::throw_host_exception<std::runtime_error>("Mounted LayoutHolder has no layout resource: " + mLayoutName);
            if (runtime != nullptr) {
                runtime->note_layout_texture_decode_failed(mLayoutName, "blyt/" + lower_copy(mLayoutName) + ".brlyt",
                                                           "exact layout resource is absent");
            }
            return;
        }

        mBrlytLayout = smgpc::layout::parse_brlyt_layout(archive->file_data(*brlyt_entry));
        for (const auto& entry : archive->entries()) {
            const auto animation_name = animation_name_from_path(entry.path);
            if (smgpc::layout::find_layout_brlan(archive->entries(), animation_name) != &entry) {
                continue;
            }

            try {
                mRenderAnimations[animation_name] = aurora::nw4r::lyt::parse_brlan_animation(archive->file_data(entry));
            } catch (const std::exception& e) {
                if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
                    runtime->note_layout_texture_decode_failed(mLayoutName, entry.path, e.what());
                }
            }
        }

        for (const auto& material : mBrlytLayout.materials) {
            for (const auto& material_texture : material.textures) {
                if (material_texture.texture_name.empty() || contains_texture(mRenderTextures, material_texture.texture_name)) {
                    continue;
                }

                const auto* texture_entry = mArchiveOwner ? archive->find_by_basename(material_texture.texture_name)
                    : archive->find(texture_archive_path(material_texture.texture_name));
                if (!texture_entry) continue;

                try {
                    mRenderTextures.push_back(RenderTexture{
                        .name = material_texture.texture_name,
                        .decoded = smgpc::resource::decode_tpl_texture(archive->file_data(*texture_entry)),
                        .handle = {},
                        .native_texture = std::make_shared<nw4r::lyt::TexMap>(make_tex_map(
                            material_texture.texture_name, archive->file_data(*texture_entry), nullptr)),
                    });
                } catch (const std::exception& e) {
                    if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
                        runtime->note_layout_texture_decode_failed(mLayoutName, material_texture.texture_name, e.what());
                    }
                }
            }
        }

        // Original layout accessors resolve process fonts, including the embedded
        // startup font. Only standalone archives discover a companion Font.arc.
        if (!mArchiveOwner && !mBrlytLayout.text_boxes.empty()) {
            if (const auto font_path = find_companion_font_archive(mArchivePath)) {
                try {
                    auto local_font_archive = std::optional< smgpc::resource::RarcArchive >{};
                    const auto* font_archive = static_cast< const smgpc::resource::RarcArchive* >(nullptr);
                    if (runtime != nullptr) {
                        font_archive = &runtime->dvd().archive_for_path(*font_path);
                    } else {
                        local_font_archive = smgpc::resource::RarcArchive::from_file(*font_path);
                        font_archive = &*local_font_archive;
                    }

                    for (const auto& text_box : mBrlytLayout.text_boxes) {
                        (void)add_render_font_from_archive(
                            mRenderFonts, *font_archive, text_box.font_name);
                    }
                } catch (const std::exception& e) {
                    if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
                        runtime->note_layout_texture_decode_failed(mLayoutName, "<font>", e.what());
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        mBrlytLayout = {};
        mRenderTextures.clear();
        mRenderFonts.clear();
        mRenderAnimations.clear();
        mCommittedPaneFrames.clear();
        mCommittedMaterialFrames.clear();
        mRenderDataLoaded = false;
        if (mArchiveOwner) throw;
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->note_layout_texture_decode_failed(mLayoutName, "<layout>", e.what());
        }
    }
}

std::array< float, 3U > smgpc::layout::LayoutRuntime::panePointForAurora(const PaneRenderState& pane_state, float local_x, float local_y) const {
    // NW4R flips the local Y column for its centered, upward-facing layout
    // rectangle. Keep that operation in local space, before the 3D matrix.
    const auto y = mBrlytLayout.origin_type == 1U ? -local_y : local_y;
    const auto& matrix = pane_state.matrix;
    auto result = std::array<float, 3U>{
        mTransX + matrix[0][3] + matrix[0][0] * local_x + matrix[0][1] * y,
        mTransY + matrix[1][3] + matrix[1][0] * local_x + matrix[1][1] * y,
        matrix[2][3] + matrix[2][0] * local_x + matrix[2][1] * y,
    };
    if (mBrlytLayout.origin_type != 1U) {
        result[0] -= mBrlytLayout.width * 0.5F;
        result[1] = mBrlytLayout.height * 0.5F - result[1];
    }
    return result;
}

smgpc::layout::LayoutRuntime::PaneRenderState smgpc::layout::LayoutRuntime::paneRenderState(std::size_t pane_index) const {
    const auto& pane = mBrlytLayout.panes.at(pane_index);
    const auto anim = animationFrameForPane(pane.name);
    auto local_alpha = anim.alpha.value_or(static_cast<float>(pane.alpha));
    auto local_visible = anim.visible.value_or(pane.visible);
    if (const auto override = mPaneVisibilityOverrides.find(pane.name); override != mPaneVisibilityOverrides.end()) {
        local_visible = override->second;
    }
    if (const auto override = mPaneAlphaOverrides.find(pane.name); override != mPaneAlphaOverrides.end()) {
        local_alpha = override->second;
    }

    Mtx local_matrix;
    paneLocalMatrix(pane_index, local_matrix);
    PaneRenderState result;
    const auto modifies_child_alpha = pane.influenced_alpha && local_alpha != 255.0F;
    result.alpha = local_alpha;
    result.child_alpha_scale = modifies_child_alpha ? local_alpha / 255.0F : 1.0F;
    result.visible = local_visible;
    result.location_adjust = pane.location_adjust;
    result.child_alpha_influenced = modifies_child_alpha;
    if (pane.parent_index < 0) {
        PSMTXCopy(local_matrix, result.matrix);
        for (auto column = 0U; column < 3U; ++column) {
            result.matrix[0][column] *= mScaleX;
            result.matrix[1][column] *= mScaleY;
        }
    } else {
        const auto parent = paneRenderState(static_cast<std::size_t>(pane.parent_index));
        PSMTXConcat(parent.matrix, local_matrix, result.matrix);
        result.alpha = parent.child_alpha_influenced ? local_alpha * parent.child_alpha_scale : local_alpha;
        result.child_alpha_scale *= parent.child_alpha_scale;
        result.visible = parent.visible && local_visible;
        result.child_alpha_influenced = parent.child_alpha_influenced || modifies_child_alpha;
    }

    const auto it = mPaneFollowPositions.find(pane_index);
    if (it == mPaneFollowPositions.end()) return result;
    const auto& follow = it->second;
    switch (follow.type) {
    case 0:
        result.matrix[0][3] = follow.position.x - mTransX;
        result.matrix[1][3] = follow.position.y - mTransY;
        break;
    case 1:
        result.matrix[0][3] += follow.position.x;
        result.matrix[1][3] += follow.position.y;
        break;
    case 2: {
        // PaneCtrl replaces local XY through global * inverse(local).
        // Inverting only the XY projection loses rotations out of the plane.
        Mtx inverse_local, parent;
        if (!PSMTXInverse(local_matrix, inverse_local))
            aurora::throw_host_exception<std::logic_error>("Replacing a pane's local position requires an invertible matrix");
        PSMTXConcat(result.matrix, inverse_local, parent);
        const auto dx = follow.position.x - local_matrix[0][3];
        const auto dy = follow.position.y - local_matrix[1][3];
        for (auto row = 0U; row < 3U; ++row)
            result.matrix[row][3] += parent[row][0] * dx + parent[row][1] * dy;
        break;
    }
    case 3:
        // The original control adds the transformed XY offset only.
        for (auto row = 0U; row < 2U; ++row)
            result.matrix[row][3] += local_matrix[row][0] * follow.position.x + local_matrix[row][1] * follow.position.y;
        break;
    }
    return result;
}

aurora::nw4r::lyt::BrlanPaneFrame smgpc::layout::LayoutRuntime::animationFrameForPane(std::string_view pane_name) const {
    auto result = aurora::nw4r::lyt::BrlanPaneFrame{};
    if (const auto committed = mCommittedPaneFrames.find(std::string(pane_name)); committed != mCommittedPaneFrames.end()) {
        result = committed->second;
    }

    for (const auto& anim_state : mAnimations) {
        if (anim_state.name.empty()) {
            continue;
        }

        const auto it = mRenderAnimations.find(lower_copy(anim_state.name));
        if (it == mRenderAnimations.end()) {
            continue;
        }

        auto frame = anim_state.frame;
        if (it->second.loop && it->second.frame_size > 0U) {
            frame = std::fmod(frame, static_cast< float >(it->second.frame_size));
        }

        const auto layer_frame = it->second.pane_frame(pane_name, frame);
        if (layer_frame.translate_x.has_value()) {
            result.translate_x = layer_frame.translate_x;
        }
        if (layer_frame.translate_y.has_value()) {
            result.translate_y = layer_frame.translate_y;
        }
        if (layer_frame.translate_z.has_value()) {
            result.translate_z = layer_frame.translate_z;
        }
        if (layer_frame.scale_x.has_value()) {
            result.scale_x = layer_frame.scale_x;
        }
        if (layer_frame.scale_y.has_value()) {
            result.scale_y = layer_frame.scale_y;
        }
        if (layer_frame.rotate_x.has_value()) {
            result.rotate_x = layer_frame.rotate_x;
        }
        if (layer_frame.rotate_y.has_value()) {
            result.rotate_y = layer_frame.rotate_y;
        }
        if (layer_frame.rotate_z.has_value()) {
            result.rotate_z = layer_frame.rotate_z;
        }
        if (layer_frame.width.has_value()) {
            result.width = layer_frame.width;
        }
        if (layer_frame.height.has_value()) {
            result.height = layer_frame.height;
        }
        if (layer_frame.alpha.has_value()) {
            result.alpha = layer_frame.alpha;
        }
        if (layer_frame.visible.has_value()) {
            result.visible = layer_frame.visible;
        }
    }
    if (const auto* pane_anim = findPaneAnimation(pane_name)) {
        for (const auto& anim_state : pane_anim->animations) {
            if (anim_state.name.empty()) {
                continue;
            }

            const auto it = mRenderAnimations.find(lower_copy(anim_state.name));
            if (it == mRenderAnimations.end()) {
                continue;
            }

            auto frame = anim_state.frame;
            if (it->second.loop && it->second.frame_size > 0U) {
                frame = std::fmod(frame, static_cast< float >(it->second.frame_size));
            }

            const auto layer_frame = it->second.pane_frame(pane_name, frame);
            if (layer_frame.translate_x.has_value()) {
                result.translate_x = layer_frame.translate_x;
            }
            if (layer_frame.translate_y.has_value()) {
                result.translate_y = layer_frame.translate_y;
            }
            if (layer_frame.translate_z.has_value()) {
                result.translate_z = layer_frame.translate_z;
            }
            if (layer_frame.scale_x.has_value()) {
                result.scale_x = layer_frame.scale_x;
            }
            if (layer_frame.scale_y.has_value()) {
                result.scale_y = layer_frame.scale_y;
            }
            if (layer_frame.rotate_x.has_value()) {
                result.rotate_x = layer_frame.rotate_x;
            }
            if (layer_frame.rotate_y.has_value()) {
                result.rotate_y = layer_frame.rotate_y;
            }
            if (layer_frame.rotate_z.has_value()) {
                result.rotate_z = layer_frame.rotate_z;
            }
            if (layer_frame.width.has_value()) {
                result.width = layer_frame.width;
            }
            if (layer_frame.height.has_value()) {
                result.height = layer_frame.height;
            }
            if (layer_frame.alpha.has_value()) {
                result.alpha = layer_frame.alpha;
            }
            if (layer_frame.visible.has_value()) {
                result.visible = layer_frame.visible;
            }
        }
    }

    return result;
}

aurora::nw4r::lyt::BrlanTextureFrame smgpc::layout::LayoutRuntime::textureFrameForContent(std::string_view content_name) const {
    auto result = aurora::nw4r::lyt::BrlanTextureFrame{};
    for (const auto& anim_state : mAnimations) {
        if (anim_state.name.empty()) {
            continue;
        }

        const auto it = mRenderAnimations.find(lower_copy(anim_state.name));
        if (it == mRenderAnimations.end()) {
            continue;
        }

        auto frame = anim_state.frame;
        if (it->second.loop && it->second.frame_size > 0U) {
            frame = std::fmod(frame, static_cast< float >(it->second.frame_size));
        }

        const auto layer_frame = it->second.texture_frame(content_name, frame);
        if (layer_frame.translate_s.has_value()) {
            result.translate_s = layer_frame.translate_s;
        }
        if (layer_frame.translate_t.has_value()) {
            result.translate_t = layer_frame.translate_t;
        }
        if (layer_frame.rotate.has_value()) {
            result.rotate = layer_frame.rotate;
        }
        if (layer_frame.scale_s.has_value()) {
            result.scale_s = layer_frame.scale_s;
        }
        if (layer_frame.scale_t.has_value()) {
            result.scale_t = layer_frame.scale_t;
        }
    }

    return result;
}

aurora::nw4r::lyt::BrlanMaterialFrame smgpc::layout::LayoutRuntime::materialFrameForContent(std::string_view content_name) const {
    auto result = aurora::nw4r::lyt::BrlanMaterialFrame{};
    if (const auto committed = mCommittedMaterialFrames.find(std::string(content_name)); committed != mCommittedMaterialFrames.end()) {
        result = committed->second;
    }

    for (const auto& anim_state : mAnimations) {
        if (anim_state.name.empty()) {
            continue;
        }

        const auto it = mRenderAnimations.find(lower_copy(anim_state.name));
        if (it == mRenderAnimations.end()) {
            continue;
        }

        auto frame = anim_state.frame;
        if (it->second.loop && it->second.frame_size > 0U) {
            frame = std::fmod(frame, static_cast< float >(it->second.frame_size));
        }

        merge_material_frame(result, it->second.material_frame(content_name, frame));
    }

    for (const auto& pane_anim : mPaneAnimations) {
        if (pane_anim.pane_name != content_name) {
            continue;
        }

        for (const auto& anim_state : pane_anim.animations) {
            if (anim_state.name.empty()) {
                continue;
            }

            const auto it = mRenderAnimations.find(lower_copy(anim_state.name));
            if (it == mRenderAnimations.end()) {
                continue;
            }

            auto frame = anim_state.frame;
            if (it->second.loop && it->second.frame_size > 0U) {
                frame = std::fmod(frame, static_cast< float >(it->second.frame_size));
            }

            merge_material_frame(result, it->second.material_frame(content_name, frame));
        }
    }

    return result;
}

f32 smgpc::layout::LayoutRuntime::durationFor(const char* pAnimName) const {
    const smgpc::compat::JkrHostAllocationScope host;
    if (pAnimName == nullptr || pAnimName[0] == '\0') {
        aurora::throw_host_exception<std::logic_error>("Animation duration requires a real BRLAN name");
    }
    const auto it = mRenderAnimations.find(lower_copy(pAnimName));
    if (it != mRenderAnimations.end() && it->second.frame_size > 0U) {
        return static_cast< f32 >(it->second.frame_size);
    }

    aurora::throw_host_exception<std::runtime_error>("Layout " + mLayoutName + " has no usable BRLAN " + std::string(pAnimName));
}

bool smgpc::layout::LayoutRuntime::isLoopingAnim(const char* pAnimName) const {
    const smgpc::compat::JkrHostAllocationScope host;
    if (pAnimName == nullptr || pAnimName[0] == '\0') {
        aurora::throw_host_exception<std::logic_error>("Animation loop state requires a real BRLAN name");
    }
    const auto it = mRenderAnimations.find(lower_copy(pAnimName));
    if (it != mRenderAnimations.end()) {
        return it->second.loop;
    }

    aurora::throw_host_exception<std::runtime_error>("Layout " + mLayoutName + " has no BRLAN " + std::string(pAnimName));
}

void smgpc::layout::LayoutRuntime::paneLocalMatrix(std::size_t index, MtxPtr matrix) const {
    const auto& pane = mBrlytLayout.panes.at(index);
    const auto frame = animationFrameForPane(pane.name);
    auto scale_x = frame.scale_x.value_or(pane.scale_x);
    auto scale_y = frame.scale_y.value_or(pane.scale_y);
    if (pane.location_adjust) {
        const auto adjust = layout_location_adjust_scale();
        scale_x *= adjust[0];
        scale_y *= adjust[1];
    }
    // nw4r::lyt::Pane::CalculateMtx: T * Rz * Ry * Rx * S.
    constexpr auto radians = 3.14159265358979323846F / 180.0F;
    Mtx rotation, intermediate;
    PSMTXScale(matrix, scale_x, scale_y, 1.0F);
    PSMTXRotRad(rotation, 'x', frame.rotate_x.value_or(pane.rotate_x) * radians);
    PSMTXConcat(rotation, matrix, intermediate);
    PSMTXRotRad(rotation, 'y', frame.rotate_y.value_or(pane.rotate_y) * radians);
    PSMTXConcat(rotation, intermediate, matrix);
    PSMTXRotRad(rotation, 'z', frame.rotate_z.value_or(pane.rotate_z) * radians);
    PSMTXConcat(rotation, matrix, intermediate);
    PSMTXTransApply(intermediate, matrix, frame.translate_x.value_or(pane.translate_x),
                   frame.translate_y.value_or(pane.translate_y), frame.translate_z.value_or(pane.translate_z));
}
