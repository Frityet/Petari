#include "layout/Nw4rLayoutRecords.hpp"
#include "layout/LayoutRuntime.hpp"
#include "Game/Util/MessageUtil.hpp"
#include <nw4r/lyt/group.h>
#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace smgpc::layout {
namespace {
class NativePaneRecord final : public nw4r::lyt::Pane {
public:
    NativePaneRecord(const nw4r::lyt::res::Pane& resource, Nw4rLayoutRecords& owner,
                     std::array<char, 4> kind) : Pane(&resource), owner(owner), kind(kind) {
        mbUserAllocated = true;
    }
    const nw4r::ut::detail::RuntimeTypeInfo* GetRuntimeTypeInfo() const override {
        require_base("NW4R derived pane RTTI");
        return &nw4r::lyt::Pane::typeInfo;
    }
    void require_base(std::string_view operation) const {
        if (kind != std::array<char, 4>{'p', 'a', 'n', '1'})
            aurora::throw_host_exception<std::logic_error>(std::string(operation) + " requires the actual derived owner for " +
                                                          std::string(kind.data(), kind.size()));
    }
    Nw4rLayoutRecords& owner;
    std::array<char, 4> kind;
};
class NativeGroupRecord final : public nw4r::lyt::Group {
public:
    NativeGroupRecord(const nw4r::lyt::res::Group* resource, nw4r::lyt::Pane* root, Nw4rLayoutRecords& owner)
        : Group(resource, root), owner(owner) {}
    Nw4rLayoutRecords& owner;
};
struct PublishedPane {
    nw4r::math::VEC3 translate;
    nw4r::math::VEC3 rotate;
    nw4r::math::VEC2 scale;
    nw4r::lyt::Size size;
    u8 alpha;
    u8 flags;
};
PublishedPane published(const nw4r::lyt::Pane& pane) {
    return {pane.mTranslate, pane.mRotate, pane.mScale, pane.mSize, pane.mAlpha, pane.mFlag};
}
}

struct Nw4rLayoutRecords::State {
    explicit State(LayoutRuntime& runtime) : runtime(runtime) {}
    ~State() {
        retiring = true;
        groups.clear();
        // All linked records remain alive until every intrusive child list has
        // been detached, including construction failures and vector teardown.
        for (auto& pane : panes) {
            while (pane->mChildList.GetSize()) {
                auto* child = &*pane->mChildList.GetBeginIter();
                pane->mChildList.Erase(child);
                child->mpParent = nullptr;
            }
        }
    }
    LayoutRuntime& runtime;
    bool published = false;
    bool retiring = false;
    bool synchronizing = false;
    std::vector<std::unique_ptr<NativePaneRecord>> panes;
    std::vector<PublishedPane> previous;
    std::vector<std::unique_ptr<nw4r::lyt::Group>> groups;
};

Nw4rLayoutRecords::Nw4rLayoutRecords(LayoutRuntime& runtime) {
    const aurora::allocation::HostAllocationScope host;
    _state = std::make_unique<State>(runtime);
    runtime.loadRenderData();
    const auto& layout = runtime.mBrlytLayout;
    if (layout.panes.empty()) aurora::throw_host_exception<std::logic_error>("NW4R pane ownership requires an actual BRLYT root");
    _state->panes.reserve(layout.panes.size());
    _state->previous.resize(layout.panes.size());
    for (const auto& source : layout.panes) {
        nw4r::lyt::res::Pane resource{};
        std::copy(source.resource_kind.begin(), source.resource_kind.end(), resource.blockHeader.kind);
        resource.blockHeader.size = sizeof(resource);
        resource.flag = (source.visible ? 1 : 0) | (source.influenced_alpha ? 2 : 0) | (source.location_adjust ? 4 : 0);
        resource.basePosition = source.base_position;
        resource.alpha = source.alpha;
        std::strncpy(resource.name, source.name.c_str(), sizeof(resource.name));
        std::strncpy(resource.userData, source.user_data.c_str(), sizeof(resource.userData));
        resource.translate.x = source.translate_x; resource.translate.y = source.translate_y; resource.translate.z = source.translate_z;
        resource.rotate.x = source.rotate_x; resource.rotate.y = source.rotate_y; resource.rotate.z = source.rotate_z;
        resource.scale.x = source.scale_x; resource.scale.y = source.scale_y;
        resource.size.width = source.width; resource.size.height = source.height;
        _state->panes.push_back(std::make_unique<NativePaneRecord>(resource, *this, source.resource_kind));
    }
    for (std::size_t i = 0; i < layout.panes.size(); ++i) {
        const auto parent = layout.panes[i].parent_index;
        if (parent >= 0) _state->panes.at(static_cast<std::size_t>(parent))->AppendChild(_state->panes[i].get());
    }
    for (const auto& source : layout.groups) {
        // NW4R Layout::Build retains only immediate children of the root group.
        if (source.root_group || source.nest_level != 1) continue;
        std::vector<u8> decoded(sizeof(nw4r::lyt::res::Group) + source.pane_names.size() * 16);
        auto* resource = reinterpret_cast<nw4r::lyt::res::Group*>(decoded.data());
        std::copy_n("grp1", 4, resource->blockHeader.kind);
        resource->blockHeader.size = static_cast<u32>(decoded.size());
        std::strncpy(resource->name, source.name.c_str(), sizeof(resource->name));
        resource->paneNum = static_cast<u16>(source.pane_names.size());
        auto* names = reinterpret_cast<char*>(decoded.data()) + sizeof(*resource);
        for (std::size_t i = 0; i < source.pane_names.size(); ++i)
            std::strncpy(names + 16 * i, source.pane_names[i].c_str(), 16);
        _state->groups.push_back(std::make_unique<NativeGroupRecord>(resource, _state->panes.front().get(), *this));
    }
    synchronize();
}
Nw4rLayoutRecords::~Nw4rLayoutRecords() = default;

void Nw4rLayoutRecords::synchronize() {
    const aurora::allocation::HostAllocationScope host;
    auto& state = *_state;
    if (state.synchronizing) return;
    state.synchronizing = true;
    struct Restore { bool& value; ~Restore() { value = false; } } restore{state.synchronizing};
    auto& runtime = state.runtime;
    if (state.published) for (std::size_t i = 0; i < state.panes.size(); ++i) {
        auto& pane = *state.panes[i];
        const auto& old = state.previous[i];
        auto& source = runtime.mBrlytLayout.panes[i];
        const auto* expected_parent = source.parent_index < 0 ? nullptr : state.panes.at(source.parent_index).get();
        if (std::strncmp(pane.mName, source.name.c_str(), 16) != 0 || pane.mpParent != expected_parent)
            aurora::throw_host_exception<std::logic_error>("Changing a published NW4R resource name/hierarchy requires matching renderer topology support");
        if (pane.mSize.width != old.size.width || pane.mSize.height != old.size.height)
            aurora::throw_host_exception<std::logic_error>("Changing SDK pane size requires the derived geometry owner");
        auto& frame = runtime.mCommittedPaneFrames[source.name];
        if (pane.mTranslate.x != old.translate.x) frame.translate_x = pane.mTranslate.x;
        if (pane.mTranslate.y != old.translate.y) frame.translate_y = pane.mTranslate.y;
        if (pane.mTranslate.z != old.translate.z) frame.translate_z = pane.mTranslate.z;
        if (pane.mRotate.x != old.rotate.x) frame.rotate_x = pane.mRotate.x;
        if (pane.mRotate.y != old.rotate.y) frame.rotate_y = pane.mRotate.y;
        if (pane.mScale.x != old.scale.x) frame.scale_x = pane.mScale.x;
        if (pane.mScale.y != old.scale.y) frame.scale_y = pane.mScale.y;
        if (pane.mRotate.z != old.rotate.z) frame.rotate_z = pane.mRotate.z;
        if (pane.mAlpha != old.alpha) runtime.mPaneAlphaOverrides[source.name] = pane.mAlpha;
        if ((pane.mFlag & 1) != (old.flags & 1)) runtime.setPaneVisible(source.name, pane.IsVisible());
        if ((pane.mFlag & 2) != (old.flags & 2)) source.influenced_alpha = pane.IsInfluencedAlpha();
        if ((pane.mFlag & 4) != (old.flags & 4)) source.location_adjust = pane.IsLocationAdjust();
    }
    for (std::size_t i = 0; i < state.panes.size(); ++i) {
        auto& pane = *state.panes[i];
        const auto& source = runtime.mBrlytLayout.panes[i];
        const auto anim = runtime.animationFrameForPane(source.name);
        pane.mTranslate.x = anim.translate_x.value_or(source.translate_x);
        pane.mTranslate.y = anim.translate_y.value_or(source.translate_y);
        pane.mTranslate.z = anim.translate_z.value_or(source.translate_z);
        pane.mRotate.x = anim.rotate_x.value_or(source.rotate_x);
        pane.mRotate.y = anim.rotate_y.value_or(source.rotate_y);
        pane.mRotate.z = anim.rotate_z.value_or(source.rotate_z);
        pane.mScale.x = anim.scale_x.value_or(source.scale_x);
        pane.mScale.y = anim.scale_y.value_or(source.scale_y);
        pane.mAlpha = static_cast<u8>(std::clamp(anim.alpha.value_or(float(source.alpha)), 0.0F, 255.0F));
        if (const auto override = runtime.mPaneAlphaOverrides.find(source.name); override != runtime.mPaneAlphaOverrides.end())
            pane.mAlpha = static_cast<u8>(std::clamp(override->second, 0.0F, 255.0F));
        pane.mFlag = (runtime.isPaneLocallyVisible(source.name) ? 1 : 0) | (source.influenced_alpha ? 2 : 0) | (source.location_adjust ? 4 : 0);
        // Use the very same active BRLAN, pane follow and actor transforms as
        // the renderer. These are live SDK matrices, not resource-only copies.
        const auto global = runtime.paneRenderState(i);
        PSMTXCopy(global.matrix, pane.mGlbMtx.m);
        pane.mGlbMtx.m[0][3] += runtime.mTransX;
        pane.mGlbMtx.m[1][3] += runtime.mTransY;
        pane.mGlbAlpha = static_cast<u8>(std::clamp(global.alpha, 0.0F, 255.0F));
        runtime.paneLocalMatrix(i, pane.mMtx.m);
        state.previous[i] = published(pane);
    }
    state.published = true;
}

nw4r::lyt::Pane* Nw4rLayoutRecords::pane(const char* name) {
    synchronize();
    if (!name) return _state->panes.front().get();
    // Retail manager lookup uses first pane in original depth-first order.
    for (auto& pane : _state->panes) if (std::strcmp(pane->mName, name) == 0) return pane.get();
    return nullptr;
}
nw4r::lyt::Group* Nw4rLayoutRecords::group(const char* name) {
    for (auto& group : _state->groups) if (name && std::strcmp(group->GetName(), name) == 0) return group.get();
    return nullptr;
}
u32 Nw4rLayoutRecords::group_index(const char* name) const {
    for (u32 i = 0; i < _state->groups.size(); ++i)
        if (name && std::strcmp(_state->groups[i]->GetName(), name) == 0) return i;
    return group_count();
}
u32 Nw4rLayoutRecords::pane_count() const { return static_cast<u32>(_state->panes.size()); }
u32 Nw4rLayoutRecords::group_count() const { return static_cast<u32>(_state->groups.size()); }
u32 Nw4rLayoutRecords::pane_index(const nw4r::lyt::Pane* pane) const {
    for (u32 i = 0; i < _state->panes.size(); ++i) if (_state->panes[i].get() == pane) return i;
    aurora::throw_host_exception<std::logic_error>("The NW4R pane belongs to a different layout resource owner");
}
u32 Nw4rLayoutRecords::text_line_count(const char* pane_name) const {
    const aurora::allocation::HostAllocationScope host;
    auto* root = const_cast<Nw4rLayoutRecords*>(this)->pane(pane_name);
    if (!root) return 0;
    const auto root_index = pane_index(root);
    const auto& layout = _state->runtime.mBrlytLayout;
    u32 maximum = 0;
    for (const auto& text : layout.text_boxes) {
        auto index = static_cast<s32>(text.pane_index);
        while (index >= 0 && static_cast<u32>(index) != root_index) index = layout.panes[index].parent_index;
        if (index < 0) continue;
        std::wstring wide;
        wide.reserve(text.raw_text.size() + 1);
        for (auto word : text.raw_text) wide.push_back(static_cast<wchar_t>(word));
        maximum = std::max(maximum, static_cast<u32>(MR::countMessageLine(wide.c_str())));
    }
    return maximum;
}
void synchronize_native_pane(const nw4r::lyt::Pane* pane) {
    if (const auto* record = dynamic_cast<const NativePaneRecord*>(pane)) record->owner.synchronize();
    else aurora::throw_host_exception<std::logic_error>("The pane has no native layout resource owner");
}
void Nw4rLayoutRecords::require_mutable_resource_graph(std::string_view operation) const {
    if (_state->published && !_state->retiring)
        aurora::throw_host_exception<std::logic_error>(std::string(operation) + " requires matching renderer resource topology support");
}
void validate_native_pane_hierarchy_change(const nw4r::lyt::Pane* parent, const nw4r::lyt::Pane* child) {
    if (const auto* record = dynamic_cast<const NativePaneRecord*>(parent)) record->owner.require_mutable_resource_graph("Reparenting a published NW4R pane");
    if (const auto* record = dynamic_cast<const NativePaneRecord*>(child)) record->owner.require_mutable_resource_graph("Reparenting a published NW4R pane");
}
void validate_native_pane_rename(const nw4r::lyt::Pane* pane, const char* name) {
    if (const auto* record = dynamic_cast<const NativePaneRecord*>(pane); record && std::strncmp(pane->mName, name, 16) != 0)
        record->owner.require_mutable_resource_graph("Renaming a published NW4R pane");
}
void validate_native_group_append(const nw4r::lyt::Group* group) {
    if (const auto* record = dynamic_cast<const NativeGroupRecord*>(group)) record->owner.require_mutable_resource_graph("Appending to a published NW4R resource group");
}
void require_native_base_pane(const nw4r::lyt::Pane* pane, std::string_view operation) {
    if (const auto* record = dynamic_cast<const NativePaneRecord*>(pane)) record->require_base(operation);
}
} // namespace smgpc::layout
