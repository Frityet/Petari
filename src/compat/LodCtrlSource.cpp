#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/ModelObj.hpp"

namespace MR {
    const char* createLowModelObjName(const LiveActor*);
    const char* createMiddleModelObjName(const LiveActor*);
    void copyTransRotateScale(const LiveActor*, LiveActor*);
    void calcAnimDirect(LiveActor*);
    void setClippingTypeSphereContainsModelBoundingBox(LiveActor*, f32);
    const char* getModelResName(const LiveActor*);
    void hideModelAndOnCalcAnim(LiveActor*);
}  // namespace MR

namespace {
    class LodCtrlModelObj final : public ModelObj {
    public:
        LodCtrlModelObj(const char* pName, const char* pModelName, MtxPtr pMtx, int drawBufferType, int movementType,
                        int calcAnimType, bool useScale)
            : ModelObj(pName, pModelName, pMtx, drawBufferType, movementType, calcAnimType, useScale) {
        }
    };
}  // namespace

#undef NO_INLINE
#define NO_INLINE
#define ModelObj(...) LodCtrlModelObj(__VA_ARGS__)
#include "Game/LiveActor/LodCtrl.cpp"
