#include "compat/Cp932Literal.hpp"
#include "Game/MapObj/MapPartsRailGuideHolder.hpp"
#include "Game/MapObj/MapPartsRailGuideDrawer.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util.hpp"

MapPartsRailGuideHolder::~MapPartsRailGuideHolder() {
}

MapPartsRailGuideHolder::MapPartsRailGuideHolder() : NameObj(CP932("レールガイド保持")) {
    mNumRailGuides = 0;
}

void MapPartsRailGuideHolder::init(const JMapInfoIter&) {
}

MapPartsRailGuideDrawer* MapPartsRailGuideHolder::createRailGuide(LiveActor* pActor, const char* pName, const JMapInfoIter& rIter) {
    s32 id = -1;
    rIter.getValue("CommonPath_ID", &id);
    MapPartsRailGuideDrawer* pDrawer = find(id);
    if (pDrawer == nullptr) {
        pDrawer = new MapPartsRailGuideDrawer(pActor, pName);
        pDrawer->init(rIter);
        mDrawers[mNumRailGuides++] = pDrawer;
    }
    return pDrawer;
}

MapPartsRailGuideDrawer* MapPartsRailGuideHolder::find(s32 id) {
    MapPartsRailGuideDrawer** pDrawer = &mDrawers[0];
    MapPartsRailGuideDrawer** pEnd = &mDrawers[mNumRailGuides];
    for (; pDrawer != pEnd; pDrawer++) {
        if ((*pDrawer)->_420 == id) {
            return *pDrawer;
        }
    }
    return nullptr;
}
