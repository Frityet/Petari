#include "Game/Scene/PlacementInfoOrdered.hpp"
#include "Game/Util/MemoryUtil.hpp"

namespace {
    CreationFuncPtr getCreator(const PlacementInfoOrdered::Identifier&);
};  // namespace

PlacementInfoOrdered::PlacementInfoOrdered(int count) {
    mIndexArray = nullptr;
    _4 = 0;
    mSetArray = nullptr;
    mIdentiferArray = nullptr;
    mCount = count;
    mIndexArray = new Index[count];
    mSetArray = new SameIdSet[count];
    mIdentiferArray = new SameIdSet*[count];
    MR::zeroMemory(mIdentiferArray, count * sizeof(SameIdSet*));
}

/*
void PlacementInfoOrdered::requestFileLoad() {
    for (u32 i = 0; i < getUsedArrayNum(); i++) {
        Identifier* id = mIdentiferArray[i];

        if (getCreator(*id)) {
            if (id->_4 == -1) {

            }
        }
    }
}*/
