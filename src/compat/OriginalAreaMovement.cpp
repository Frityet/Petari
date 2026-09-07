#include "Game/Util/AreaObjUtil.hpp"
#include "Game/AreaObj/AreaObj.hpp"
#include "Game/AreaObj/AreaObjContainer.hpp"
#include "Game/AreaObj/AreaForm.hpp"
#include "Game/Util/MathUtil.hpp"

// Original AreaObjUtil bodies; the passive sphere uses its actual scene manager.
namespace MR {
    inline AreaObj* getAreaIn(const char* pName, const TVec3f& rPos) {
        return getAreaObjContainer()->getAreaObj(pName, rPos);
    }

    bool calcAreaMoveVelocity(TVec3f* pVelocity, const TVec3f& rPos) {
        AreaObj* area = getAreaIn("AreaMoveSphere", rPos);
        if (area == nullptr) {
            pVelocity->zero();
            return false;
        }

        AreaFormSphere* form = static_cast< AreaFormSphere* >(area->mForm);
        TVec3f center;
        form->calcPos(&center);
        TVec3f up;
        form->calcUpVec(&up);
        TVec3f direction = center;
        direction -= rPos;
        normalizeOrZero(&direction);
        vecKillElement(up, direction, &up);
        normalizeOrZero(&up);
        s32 speed = getAreaObjArg(area, 0);
        if (speed == -1) {
            speed = 10;
        }
        pVelocity->set(up * speed);
        return true;
    }

}
