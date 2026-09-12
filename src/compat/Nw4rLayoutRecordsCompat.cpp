#include "layout/Nw4rLayoutRecords.hpp"
#include <nw4r/lyt/pane.h>
#include <nw4r/lyt/common.h>
#include <nw4r/lyt/material.h>
#include <nw4r/lyt/group.h>
#include <nw4r/lyt/animation.h>
#include <nw4r/lyt/layout.h>
#include <nw4r/lyt/drawInfo.h>
#include <nw4r/math/triangular.h>
#include <cstring>

namespace nw4r::lyt {
NW4R_UT_RUNTIME_TYPEINFO_ROOT_DEFINITION(Pane);
namespace detail {
PaneBase::PaneBase() = default;
PaneBase::~PaneBase() = default;
}
Pane::Pane() {
    Init();
    mTranslate = {0, 0, 0};
    mRotate = {0, 0, 0};
    mScale = {1, 1};
    mSize = {0, 0};
    mAlpha = 255;
    mGlbAlpha = 255;
    mBasePosition = 0;
    mFlag = 1;
    mName[0] = '\0';
    mUserData[0] = '\0';
    PSMTXIdentity(mMtx.m);
    PSMTXIdentity(mGlbMtx.m);
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
            for (PaneList::Iterator it = mChildList.GetBeginIter(); it != mChildList.GetEndIter();) {
                PaneList::Iterator currIt = it++;
                mChildList.Erase(currIt);
                if (!currIt->IsUserAllocated()) {
                    currIt->~Pane();
                    Layout::FreeMemory(&*currIt);
                }
            }

            UnbindAnimationSelf(0);

            if (mpMaterial && !mpMaterial->IsUserAllocated()) {
                mpMaterial->~Material();
                Layout::FreeMemory(mpMaterial);
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
void Pane::CalculateMtx(const DrawInfo& rInfo) {
            if (smgpc::layout::synchronize_native_pane(this)) return;
            if (!IsVisible() && !rInfo.IsInvisiblePaneCalculateMtx()) {
                return;
            }

            math::MTX34 mtx1, mtx2;
            math::MTX34 rotateMtx;

            math::VEC2 scale = mScale;
            if (rInfo.IsLocationAdjust() && IsLocationAdjust()) {
                scale.x *= rInfo.GetLocationAdjustScale().x;
                scale.y *= rInfo.GetLocationAdjustScale().y;
            }

            PSMTXScale(mtx2, scale.x, scale.y, 1.0f);

            PSMTXRotRad(rotateMtx, 'x', NW4R_MATH_DEG_TO_RAD(mRotate.x));
            PSMTXConcat(rotateMtx, mtx2, mtx1);

            PSMTXRotRad(rotateMtx, 'y', NW4R_MATH_DEG_TO_RAD(mRotate.y));
            PSMTXConcat(rotateMtx, mtx1, mtx2);

            PSMTXRotRad(rotateMtx, 'z', NW4R_MATH_DEG_TO_RAD(mRotate.z));
            PSMTXConcat(rotateMtx, mtx2, mtx1);

            PSMTXTransApply(mtx1, mMtx, mTranslate.x, mTranslate.y, mTranslate.z);

            if (mpParent != NULL) {
                math::MTX34Mult(&mGlbMtx, &mpParent->mGlbMtx, &mMtx);
            } else if (rInfo.IsMultipleViewMtxOnDraw()) {
                mGlbMtx = mMtx;
            } else {
                math::MTX34Mult(&mGlbMtx, &rInfo.GetViewMtx(), &mMtx);
            }

            if (rInfo.IsInfluencedAlpha() && mpParent != NULL) {
                mGlbAlpha = static_cast< u8 >(mAlpha * rInfo.GetGlobalAlpha());
            } else {
                mGlbAlpha = mAlpha;
            }

            f32 glbAlpha = rInfo.GetGlobalAlpha();
            bool influenced = rInfo.IsInfluencedAlpha();
            bool modifyInfo = IsInfluencedAlpha() && mAlpha != 255;

            if (modifyInfo) {
                DrawInfo& rMtInfo = const_cast< DrawInfo& >(rInfo);
                rMtInfo.SetGlobalAlpha(glbAlpha * mAlpha * (1.0f / 255.0f));
                rMtInfo.SetInfluencedAlpha(true);
            }

            CalculateMtxChild(rInfo);

            if (modifyInfo) {
                DrawInfo& rMtInfo = const_cast< DrawInfo& >(rInfo);
                rMtInfo.SetGlobalAlpha(glbAlpha);
                rMtInfo.SetInfluencedAlpha(influenced);
            }
        }
void Pane::CalculateMtxChild(const DrawInfo& info) {
    for (auto iter = mChildList.GetBeginIter(); iter != mChildList.GetEndIter(); ++iter) iter->CalculateMtx(info);
}
void Pane::Animate(u32 option) {
            AnimateSelf(option);

            if (IsVisible() || !(option & 1)) {
                for (PaneList::Iterator it = mChildList.GetBeginIter(); it != mChildList.GetEndIter(); ++it) {
                    it->Animate(option);
                }
            }
        }
void Pane::AnimateSelf(u32 option) {
            smgpc::layout::animate_native_pane(this);
            for (AnimationList::Iterator it = mAnimList.GetBeginIter(); it != mAnimList.GetEndIter(); ++it) {
                if (it->IsEnable()) {
                    AnimTransform* animTrans = it->GetAnimTransform();
                    animTrans->Animate(it->GetIndex(), this);
                }
            }

            if (IsVisible() || !(option & 1)) {
                if (mpMaterial != NULL) {
                    mpMaterial->Animate();
                }
            }
        }
void Pane::Draw(const DrawInfo& rInfo) {
            if (!IsVisible()) {
                return;
            }

            DrawSelf(rInfo);
            for (PaneList::Iterator it = mChildList.GetBeginIter(); it != mChildList.GetEndIter(); ++it) {
                it->Draw(rInfo);
            }
        }
void Pane::DrawSelf(const DrawInfo&) { }
const ut::Color Pane::GetVtxColor(u32) const { return 0xFFFFFFFF; }
void Pane::SetVtxColor(u32, ut::Color) { }
u8 Pane::GetVtxColorElement(u32) const { return 0xFF; }
void Pane::SetVtxColorElement(u32, u8) { }
u8 Pane::GetColorElement(u32 index) const { return index == 0x10 ? mAlpha : GetVtxColorElement(index); }
void Pane::SetColorElement(u32 index, u8 value) { if (index == 0x10) mAlpha = value; else SetVtxColorElement(index, value); }
Material* Pane::FindMaterialByName(const char* pName, bool recursive) {
            if (mpMaterial != NULL && detail::EqualsMaterialName(mpMaterial->GetName(), pName)) {
                return mpMaterial;
            }

            if (recursive) {
                for (PaneList::Iterator it = mChildList.GetBeginIter(); it != mChildList.GetEndIter(); ++it) {
                    if (Material* pMat = it->FindMaterialByName(pName, true)) {
                        return pMat;
                    }
                }
            }

            return nullptr;
        }
void Pane::BindAnimation(AnimTransform* pAnimTrans, bool recursive) {
            pAnimTrans->Bind(this, recursive);
        }
void Pane::UnbindAnimation(AnimTransform* pAnimTrans, bool recursive) {
            UnbindAnimationSelf(pAnimTrans);

            if (recursive) {
                for (PaneList::Iterator it = mChildList.GetBeginIter(); it != mChildList.GetEndIter(); ++it) {
                    it->UnbindAnimation(pAnimTrans, recursive);
                }
            }
        }
void Pane::UnbindAllAnimation(bool recursive) {
            UnbindAnimation(nullptr, recursive);
        }
void Pane::UnbindAnimationSelf(AnimTransform* pAnimTrans) {
            if (mpMaterial != NULL) {
                mpMaterial->UnbindAnimation(pAnimTrans);
            }

            detail::UnbindAnimationLink(&mAnimList, pAnimTrans);
        }
AnimationLink* Pane::FindAnimationLinkSelf(AnimTransform* pAnimTrans) {
            return detail::FindAnimationLink(&mAnimList, pAnimTrans);
        }
void Pane::SetAnimationEnable(AnimTransform* pAnimTrans, bool enable, bool recursive) {
            AnimationLink* pAnimLink = FindAnimationLinkSelf(pAnimTrans);

            if (pAnimLink != NULL) {
                pAnimLink->SetEnable(enable);
            }

            if (mpMaterial != NULL) {
                mpMaterial->SetAnimationEnable(pAnimTrans, enable);
            }

            if (recursive) {
                for (PaneList::Iterator it = mChildList.GetBeginIter(); it != mChildList.GetEndIter(); ++it) {
                    it->SetAnimationEnable(pAnimTrans, enable, recursive);
                }
            }
        }
Material* Pane::GetMaterial() const { return mpMaterial; }
void Pane::LoadMtx(const DrawInfo& info) {
    math::MTX34 matrix;
    const math::MTX34* selected = &mGlbMtx;
    if (info.IsMultipleViewMtxOnDraw()) {
        PSMTXConcat(info.GetViewMtx().m, mGlbMtx.m, matrix.m);
        selected = &matrix;
    } else if (info.IsYAxisUp()) {
        PSMTXCopy(mGlbMtx.m, matrix.m);
        selected = &matrix;
    }
    if (info.IsYAxisUp()) for (u32 row = 0; row < 3; ++row) matrix.m[row][1] = -matrix.m[row][1];
    GXLoadPosMtxImm(selected->m, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
}
math::VEC2 Pane::GetVtxPos() const {
    math::VEC2 base(0, 0);
    switch (mBasePosition % 3) {
    case 1: base.x = -mSize.width / 2; break;
    case 2: base.x = -mSize.width; break;
    }
    switch (mBasePosition / 3) {
    case 1: base.y = -mSize.height / 2; break;
    case 2: base.y = -mSize.height; break;
    }
    return base;
}
void Pane::AddAnimationLink(AnimationLink* pAnimLink) {
            mAnimList.PushBack(pAnimLink);
        }
} // namespace nw4r::lyt
