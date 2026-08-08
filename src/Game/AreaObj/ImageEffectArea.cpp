#include "Game/AreaObj/ImageEffectArea.hpp"

ImageEffectArea::ImageEffectArea(EImageEffectType effectType, int formType, const char* pName) : AreaObj(formType, pName), mEffectType(effectType) {
}

ImageEffectAreaMgr::ImageEffectAreaMgr(s32 maxNum, const char* pName) : AreaObjMgr(maxNum, pName) {
}

void ImageEffectAreaMgr::initAfterPlacement() {
    sort();
}

void ImageEffectAreaMgr::sort() {
    if (mArray.size() == 0) {
        return;
    }

    for (u32 i = 0; i < mArray.size() - 1; i++) {
        int swapIndex = i;
        AreaObj* swapObj = getAreaObj(i);
        AreaObj* curObj = swapObj;
        for (u32 j = i + 1; j < mArray.size(); j++) {
            AreaObj* nextObj = getAreaObj(j);
            if (swapObj->mObjArg7 > nextObj->mObjArg7) {
                swapIndex = j;
                swapObj = nextObj;
            }
        }

        if (swapIndex != i) {
            mArray[i] = swapObj;
            mArray[swapIndex] = curObj;
        }
    }
}
