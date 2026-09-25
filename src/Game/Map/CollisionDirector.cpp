#include "compat/Cp932Literal.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Map/CollisionCategorizedKeeper.hpp"
#include "Game/Map/CollisionCode.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"

#define CATEGORY_KEEPER_NUM 4

CollisionDirector::CollisionDirector() : NameObj(CP932("地形コリジョン")), mCategoryKeeper(), mCode() {
    try {
        mCode = new CollisionCode();
        mCategoryKeeper = new CollisionCategorizedKeeper*[CATEGORY_KEEPER_NUM]{};

        for (s32 i = 0; i < CATEGORY_KEEPER_NUM; i++) {
            mCategoryKeeper[i] = new CollisionCategorizedKeeper(i);
            smgpc::compat::claim_name_obj_runtime_ownership(mCategoryKeeper[i], this);
        }

        MR::connectToScene(this, MR::MovementType_CollisionDirector, -1, -1, -1);
    } catch (...) {
        if (mCategoryKeeper) {
            for (s32 i = 0; i < CATEGORY_KEEPER_NUM; i++)
                delete mCategoryKeeper[i];
        }
        delete[] mCategoryKeeper;
        delete mCode;
        throw;
    }
}

CollisionDirector::~CollisionDirector() {
    for (s32 i = 0; i < CATEGORY_KEEPER_NUM; i++)
        delete mCategoryKeeper[i];
    delete[] mCategoryKeeper;
    delete mCode;
}

void CollisionDirector::init(const JMapInfoIter& rIter) {
}

void CollisionDirector::initAfterPlacement() {
}

void CollisionDirector::movement() {
    for (s32 i = 0; i < CATEGORY_KEEPER_NUM; i++) {
        mCategoryKeeper[i]->movement();
    }
}

CollisionDirector* MR::getCollisionDirector() {
    return MR::getSceneObj< CollisionDirector >(SceneObj_CollisionDirector);
}
