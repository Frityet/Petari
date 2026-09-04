#include "Game/Util/ObjUtil.hpp"
#include "Game/NameObj/NameObjListExecutor.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/SingletonHolder.hpp"
namespace MR {
    void registerPreDrawFunction(const MR::FunctorBase& rFunc, int drawType) {
        SingletonHolder< GameSystem >::get()->mSceneController->getNameObjListExecutor()->registerPreDrawFunction(rFunc, drawType);
    }
}
