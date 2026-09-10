#include "layout/Nw4rLayoutRecords.hpp"
#include <nw4r/lyt/pane.h>
#include <nw4r/lyt/group.h>
#include <nw4r/lyt/animation.h>
#include <aurora/exception.hpp>
#include <aurora/allocation.hpp>
#include <cstring>
#include <stdexcept>

namespace {
[[noreturn]] void unavailable(const char* operation) {
    aurora::throw_host_exception<std::logic_error>(std::string(operation) + " requires the original derived NW4R renderer/animation owner");
}
}
namespace nw4r::lyt {
NW4R_UT_RUNTIME_TYPEINFO_ROOT_DEFINITION(Pane);
namespace detail {
PaneBase::PaneBase() = default;
PaneBase::~PaneBase() = default;
}
Pane::Pane(const res::Pane* resource) {
    Init();
    mBasePosition = resource->basePosition;
    SetName(resource->name);
    SetUserData(resource->userData);
    mTranslate = resource->translate;
    mRotate = resource->rotate;
    mScale = resource->scale;
    mSize = resource->size;
    mAlpha = resource->alpha;
    mGlbAlpha = mAlpha;
    mFlag = resource->flag;
}
Pane::~Pane() {
    for (auto iter = mChildList.GetBeginIter(); iter != mChildList.GetEndIter();) {
        auto current = iter++;
        mChildList.Erase(current);
        if (!current->IsUserAllocated()) delete &*current;
    }
}
void Pane::Init() { mpParent = nullptr; mpMaterial = nullptr; mbUserAllocated = false; }
void Pane::SetName(const char* name) { smgpc::layout::validate_native_pane_rename(this, name); std::strncpy(mName, name, 16); mName[16] = '\0'; }
void Pane::SetUserData(const char* data) { std::strncpy(mUserData, data, 8); mUserData[8] = '\0'; }
void Pane::AppendChild(Pane* child) { InsertChild(mChildList.GetEndIter(), child); }
void Pane::InsertChild(PaneList::Iterator next, Pane* child) { smgpc::layout::validate_native_pane_hierarchy_change(this, child); mChildList.Insert(next, child); child->mpParent = this; }
void Pane::RemoveChild(Pane* child) { smgpc::layout::validate_native_pane_hierarchy_change(this, child); mChildList.Erase(child); child->mpParent = nullptr; }
Pane* Pane::FindPaneByName(const char* name, bool recursive) {
    if (std::strncmp(mName, name, 16) == 0) return this;
    if (recursive) for (auto iter = mChildList.GetBeginIter(); iter != mChildList.GetEndIter(); ++iter)
        if (auto* found = iter->FindPaneByName(name, true)) return found;
    return nullptr;
}
void Pane::CalculateMtx(const DrawInfo&) { smgpc::layout::synchronize_native_pane(this); }
void Pane::CalculateMtxChild(const DrawInfo& info) {
    for (auto iter = mChildList.GetBeginIter(); iter != mChildList.GetEndIter(); ++iter) iter->CalculateMtx(info);
}
void Pane::Animate(u32 option) {
    AnimateSelf(option);
    for (auto iter = mChildList.GetBeginIter(); iter != mChildList.GetEndIter(); ++iter) iter->Animate(option);
}
void Pane::AnimateSelf(u32) { smgpc::layout::animate_native_pane(this); }
void Pane::Draw(const DrawInfo&) { unavailable("Direct NW4R pane drawing"); }
void Pane::DrawSelf(const DrawInfo&) { smgpc::layout::require_native_base_pane(this, "Derived pane DrawSelf"); }
const ut::Color Pane::GetVtxColor(u32) const { smgpc::layout::require_native_base_pane(this, "Derived vertex colors"); return 0xFFFFFFFF; }
void Pane::SetVtxColor(u32, ut::Color) { smgpc::layout::require_native_base_pane(this, "Derived vertex colors"); }
u8 Pane::GetVtxColorElement(u32) const { smgpc::layout::require_native_base_pane(this, "Derived vertex colors"); return 0xFF; }
void Pane::SetVtxColorElement(u32, u8) { smgpc::layout::require_native_base_pane(this, "Derived vertex colors"); }
u8 Pane::GetColorElement(u32 index) const { return index == 0x10 ? mAlpha : GetVtxColorElement(index); }
void Pane::SetColorElement(u32 index, u8 value) { if (index == 0x10) mAlpha = value; else SetVtxColorElement(index, value); }
Material* Pane::FindMaterialByName(const char*, bool) { unavailable("NW4R material identity"); }
void Pane::BindAnimation(AnimTransform*, bool) { unavailable("NW4R animation binding"); }
void Pane::UnbindAnimation(AnimTransform*, bool) { unavailable("NW4R animation binding"); }
void Pane::UnbindAllAnimation(bool) { unavailable("NW4R animation binding"); }
void Pane::UnbindAnimationSelf(AnimTransform*) { unavailable("NW4R animation binding"); }
AnimationLink* Pane::FindAnimationLinkSelf(AnimTransform*) { unavailable("NW4R animation binding"); }
void Pane::SetAnimationEnable(AnimTransform*, bool, bool) { unavailable("NW4R animation binding"); }
Material* Pane::GetMaterial() const { smgpc::layout::require_native_base_pane(this, "Derived material identity"); return mpMaterial; }
void Pane::LoadMtx(const DrawInfo&) { unavailable("Direct NW4R pane drawing"); }
u16 AnimTransform::GetFrameSize() const { return mpRes->frameSize; }
bool AnimTransform::IsLoopData() const { return mpRes->loop != 0; }

void Group::Init() { mbUserAllocated = false; }
void Group::AppendPane(Pane* pane) {
    smgpc::layout::validate_native_group_append(this);
    const aurora::allocation::HostAllocationScope host;
    auto* link = new detail::PaneLink;
    link->mTarget = pane;
    mPaneLinkList.PushBack(link);
}
Group::Group(const res::Group* resource, Pane* root) {
    const aurora::allocation::HostAllocationScope host;
    Init();
    std::strncpy(mName, resource->name, sizeof(mName) - 1);
    mName[sizeof(mName) - 1] = '\0';
    const auto* names = reinterpret_cast<const char*>(resource) + sizeof(res::Group);
    for (int i = 0; i < resource->paneNum; ++i)
        if (auto* pane = root->FindPaneByName(names + i * 16, true)) AppendPane(pane);
}
Group::~Group() {
    for (auto iter = mPaneLinkList.GetBeginIter(); iter != mPaneLinkList.GetEndIter();) {
        auto current = iter++;
        mPaneLinkList.Erase(current);
        delete &*current;
    }
}
} // namespace nw4r::lyt
