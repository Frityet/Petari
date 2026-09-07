#include "Game/AreaObj/AreaForm.hpp"
#include "Game/AreaObj/AreaObj.hpp"
#include "Game/Util/AreaObjUtil.hpp"

// Recovered root AreaObjUtil.cpp bodies. The active PC AreaObjUtil translation
// unit supplies calcCylinderPos; these two methods are absent from that mirror.
namespace MR {
    void calcCylinderUpVec(TVec3f* pUpVec, const AreaObj* pAreaObj) {
        static_cast< AreaFormCylinder* >(pAreaObj->mForm)->calcUpVec(pUpVec);
    }

    f32 getCylinderRadius(const AreaObj* pAreaObj) {
        return static_cast< AreaFormCylinder* >(pAreaObj->mForm)->_20;
    }
};  // namespace MR
