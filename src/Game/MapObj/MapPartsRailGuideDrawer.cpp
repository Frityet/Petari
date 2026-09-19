#include "Game/MapObj/MapPartsRailGuideDrawer.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util.hpp"
#include "Game/Util/MapPartsUtil.hpp"
#include <algorithm>

namespace NrvMapPartsRailGuideDrawer {
    class HostTypeHideAll : public Nerve {
    public:
        virtual void execute(Spine*) const {}
        static HostTypeHideAll sInstance;
    };
    class HostTypeDrawAll : public Nerve {
    public:
        virtual void execute(Spine*) const {}
        static HostTypeDrawAll sInstance;
    };
    NEW_NERVE(HostTypeDrawForward, MapPartsRailGuideDrawer, DrawForward);
    HostTypeHideAll HostTypeHideAll::sInstance;
    HostTypeDrawAll HostTypeDrawAll::sInstance;
}

MapPartsRailGuideDrawer::MapPartsRailGuideDrawer(LiveActor* pHost, const char* pName)
    : MapPartsFunction(pHost, "ガイド描画"), _41C(0), _420(-1), _424(pName) {
}

void MapPartsRailGuideDrawer::init(const JMapInfoIter& rIter) {
    MR::getMapPartsArgRailGuideType(&_41C, mHost);
    rIter.getValue("CommonPath_ID", &_420);
    if (_41C == -1) {
        _41C = 0;
    }

    if (_41C == 0) {
        initNerve(&NrvMapPartsRailGuideDrawer::HostTypeHideAll::sInstance);
    } else {
        initGuidePoints(rIter);
        if (_41C == 1 || _41C == 3) {
            initNerve(&NrvMapPartsRailGuideDrawer::HostTypeDrawAll::sInstance);
        } else if (_41C == 2) {
            initNerve(&NrvMapPartsRailGuideDrawer::HostTypeDrawForward::sInstance);
        }
    }
}

bool MapPartsRailGuideDrawer::isWorking() const {
    for (MapPartsRailGuidePoint* const* pPoint = mGuidePoints.begin(); pPoint != mGuidePoints.end(); pPoint++) {
        if (!MR::isDead(*pPoint)) {
            return true;
        }
    }
    return false;
}

void MapPartsRailGuideDrawer::show() {
    std::for_each(mGuidePoints.begin(), mGuidePoints.end(), std::mem_func(&LiveActor::appear));
}

void MapPartsRailGuideDrawer::hide() {
    std::for_each(mGuidePoints.begin(), mGuidePoints.end(), std::mem_func(&LiveActor::kill));
}

void MapPartsRailGuideDrawer::start() {
    show();
}

void MapPartsRailGuideDrawer::end() {
    hide();
}

void MapPartsRailGuideDrawer::initGuidePoints(const JMapInfoIter& rIter) {
    s32 shadowType = 0;
    MR::getMapPartsArgShadowType(&shadowType, rIter);
    bool hasShadow = MR::hasMapPartsShadow(shadowType);
    f32 railLength = MR::getRailTotalLength(mHost);
    f32 curLen = 0.0f;

    while (curLen < railLength) {
        MapPartsRailGuidePoint* pnt = new MapPartsRailGuidePoint(mHost, _424, curLen, hasShadow);
        pnt->initWithoutIter();
        mGuidePoints.push_back(pnt);
        curLen += 200.0f;
    }

    if (_41C == 3) {
        int curPointNum = 0;

        while (curPointNum < MR::getRailPointNum(mHost)) {
            MapPartsRailGuidePoint* point = new MapPartsRailGuidePoint(mHost, _424, curPointNum, hasShadow);
            point->initWithoutIter();
            point->mScale.set(2.0f);
            mGuidePoints.push_back(point);
            curPointNum++;
        }
    }
}

MapPartsRailGuideDrawer::~MapPartsRailGuideDrawer() {
}

void MapPartsRailGuideDrawer::exeDrawForward() {
    f32 coord = MR::getRailCoord(mHost);
    for (MapPartsRailGuidePoint** pPoint = mGuidePoints.begin(); pPoint != mGuidePoints.end(); pPoint++) {
        if (coord < (*pPoint)->_8C) {
            break;
        }
        if (!MR::isDead(*pPoint)) {
            (*pPoint)->kill();
        }
    }
}
