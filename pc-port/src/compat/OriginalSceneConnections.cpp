#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"

namespace MR {
    void connectToSceneCollisionMapObj(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_CollisionMapObj, MR::CalcAnimType_CollisionMapObj, MR::DrawBufferType_MapObj, -1);
    }

    void connectToSceneCollisionMapObjMovementCalcAnim(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_CollisionMapObj, MR::CalcAnimType_CollisionMapObj, -1, -1);
    }

    void connectToSceneCollisionMapObjWeakLight(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_CollisionMapObj, MR::CalcAnimType_CollisionMapObj, MR::DrawBufferType_MapObjWeakLight, -1);
    }

    void connectToSceneCollisionMapObjStrongLight(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_CollisionMapObj, MR::CalcAnimType_CollisionMapObj, MR::DrawBufferType_MapObjStrongLight, -1);
    }

    void connectToSceneCollisionEnemy(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_CollisionEnemy, MR::CalcAnimType_CollisionEnemy, MR::DrawBufferType_Enemy, -1);
    }

    void connectToSceneCollisionEnemyMovement(NameObj* pObj) {
        MR::connectToScene(pObj, MR::MovementType_CollisionEnemy, -1, -1, -1);
    }

    void connectToSceneCollisionEnemyStrongLight(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_CollisionEnemy, MR::CalcAnimType_CollisionEnemy, MR::DrawBufferType_MapObjStrongLight, -1);
    }

    void connectToSceneCollisionEnemyNoShadowedMapObjStrongLight(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_CollisionEnemy, MR::CalcAnimType_CollisionEnemy, MR::DrawBufferType_NoShadowedMapObjStrongLight,
                           -1);
    }

    void connectToSceneNpc(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_NPC, MR::CalcAnimType_NPC, MR::DrawBufferType_NPC, -1);
    }

    void connectToSceneNpcMovement(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_NPC, -1, -1, -1);
    }

    void connectToSceneRide(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_Ride, MR::CalcAnimType_Ride, MR::DrawBufferType_Ride, -1);
    }

    void connectToSceneEnemy(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_Enemy, MR::CalcAnimType_Enemy, MR::DrawBufferType_Enemy, -1);
    }

    void connectToSceneEnemyMovement(NameObj* pObj) {
        MR::connectToScene(pObj, MR::MovementType_Enemy, -1, -1, -1);
    }

    void connectToSceneMapObj(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_MapObj, MR::CalcAnimType_MapObj, MR::DrawBufferType_MapObj, -1);
    }

    void connectToSceneMapObjMovement(NameObj* pObj) {
        MR::connectToScene(pObj, MR::MovementType_MapObj, -1, -1, -1);
    }

    void connectToSceneMapObjMovementCalcAnim(NameObj* pObj) {
        MR::connectToScene(pObj, MR::MovementType_MapObj, MR::CalcAnimType_MapObj, -1, -1);
    }

    void connectToSceneMapObjNoMovement(LiveActor* pActor) {
        MR::connectToScene(pActor, -1, MR::CalcAnimType_MapObj, MR::DrawBufferType_MapObj, -1);
    }

    void connectToSceneMapObjNoCalcAnim(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_MapObj, -1, MR::DrawBufferType_MapObj, -1);
    }

    void connectToSceneMapObjNoCalcAnimStrongLight(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_MapObj, -1, MR::DrawBufferType_MapObjStrongLight, -1);
    }

    void connectToSceneMapObjDecoration(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_MapObjDecoration, MR::CalcAnimType_MapObjDecoration, MR::DrawBufferType_MapObj, -1);
    }

    void connectToSceneMapObjDecorationStrongLight(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_MapObjDecoration, MR::CalcAnimType_MapObjDecoration, MR::DrawBufferType_MapObjStrongLight, -1);
    }

    void connectToSceneMapObjDecorationMovement(NameObj* pObj) {
        MR::connectToScene(pObj, MR::MovementType_MapObjDecoration, -1, -1, -1);
    }

    void connectToSceneMapObjStrongLight(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_MapObj, MR::CalcAnimType_MapObj, MR::DrawBufferType_MapObjStrongLight, -1);
    }

    void connectToSceneMapParts(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_ClippedMapParts, MR::CalcAnimType_ClippedMapParts, MR::DrawBufferType_MapObj, -1);
    }

    void connectToScenePlanet(LiveActor* pActor) {
        if (MR::isExistIndirectTexture(pActor)) {
            MR::connectToScene(pActor, MR::MovementType_Planet, MR::CalcAnimType_Planet, MR::DrawBufferType_IndirectPlanet, -1);
        } else {
            MR::connectToScene(pActor, MR::MovementType_Planet, MR::CalcAnimType_Planet, MR::DrawBufferType_Planet, -1);
        }
    }

    void connectToSceneEnvironment(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_Environment, MR::CalcAnimType_Environment, MR::DrawBufferType_Environment, -1);
    }

    void connectToSceneEnvironmentStrongLight(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_Environment, MR::CalcAnimType_Environment, MR::DrawBufferType_EnvironmentStrongLight, -1);
    }

    void connectToClippedMapParts(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_ClippedMapParts, MR::CalcAnimType_ClippedMapParts, MR::DrawBufferType_ClippedMapParts, -1);
    }

    void connectToSceneEnemyDecoration(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_EnemyDecoration, MR::CalcAnimType_MapObjDecoration, MR::DrawBufferType_EnemyDecoration, -1);
    }

    void connectToSceneEnemyDecorationMovement(NameObj* pObj) {
        MR::connectToScene(pObj, MR::MovementType_EnemyDecoration, -1, -1, -1);
    }

    void connectToSceneEnemyDecorationMovementCalcAnim(NameObj* pObj) {
        MR::connectToScene(pObj, MR::MovementType_EnemyDecoration, MR::CalcAnimType_MapObjDecoration, -1, -1);
    }

    void connectToSceneItem(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_Item, MR::CalcAnimType_Item, MR::DrawBufferType_NoSilhouettedMapObj, -1);
    }

    void connectToSceneItemStrongLight(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_Item, MR::CalcAnimType_Item, MR::DrawBufferType_NoSilhouettedMapObjStrongLight, -1);
    }

    void connectToSceneIndirectEnemy(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_Enemy, MR::CalcAnimType_Enemy, MR::DrawBufferType_IndirectEnemy, -1);
    }

    void connectToSceneIndirectNpc(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_NPC, MR::CalcAnimType_NPC, MR::DrawBufferType_IndirectNpc, -1);
    }

    void connectToSceneIndirectMapObj(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_MapObj, MR::CalcAnimType_MapObj, MR::DrawBufferType_IndirectMapObj, -1);
    }

    void connectToSceneIndirectMapObjStrongLight(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_MapObj, MR::CalcAnimType_MapObj, MR::DrawBufferType_IndirectMapObjStrongLight, -1);
    }

    void connectToSceneScreenEffectMovement(NameObj* pObj) {
        MR::connectToScene(pObj, MR::MovementType_ScreenEffect, -1, -1, -1);
    }

    void connectToSceneAreaObj(NameObj* pObj) {
        MR::connectToScene(pObj, MR::MovementType_AreaObj, -1, -1, -1);
    }

    void connectToScene3DModelFor2D(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_Layout, MR::CalcAnimType_Layout, MR::DrawBufferType_Model3DFor2D, -1);
    }

    void connectToSceneLayout(NameObj* pObj) {
        MR::connectToScene(pObj, MR::MovementType_Layout, MR::CalcAnimType_Layout, -1, MR::DrawType_Layout);
    }

    void connectToSceneLayoutMovementCalcAnim(NameObj* pObj) {
        MR::connectToScene(pObj, MR::MovementType_Layout, MR::CalcAnimType_Layout, -1, -1);
    }

    void connectToSceneLayoutDecoration(NameObj* pObj) {
        MR::connectToScene(pObj, MR::MovementType_LayoutDecoration, MR::CalcAnimType_LayoutDecoration, -1, MR::DrawType_LayoutDecoration);
    }

    void connectToSceneTalkLayout(NameObj* pObj) {
        MR::connectToScene(pObj, MR::MovementType_Layout, MR::CalcAnimType_Layout, -1, MR::DrawType_TalkLayout);
    }

    void connectToSceneTalkLayoutNoMovement(NameObj* pObj) {
        MR::connectToScene(pObj, -1, MR::CalcAnimType_Layout, -1, MR::DrawType_TalkLayout);
    }

    void connectToSceneWipeLayout(NameObj* pObj) {
        MR::connectToScene(pObj, MR::MovementType_WipeLayout, MR::CalcAnimType_Layout, -1, MR::DrawType_WipeLayout);
    }

    void connectToSceneLayoutOnPause(NameObj* pObj) {
        MR::connectToScene(pObj, MR::MovementType_LayoutOnPause, MR::CalcAnimType_Layout, -1, MR::DrawType_LayoutOnPause);
    }

    void connectToSceneLayoutOnPauseNoMovement(NameObj* pObj) {
        MR::connectToScene(pObj, -1, MR::CalcAnimType_Layout, -1, MR::DrawType_LayoutOnPause);
    }

    void connectToSceneLayoutOnPauseMovementCalcAnim(NameObj* pObj) {
        MR::connectToScene(pObj, MR::MovementType_LayoutOnPause, MR::CalcAnimType_Layout, -1, -1);
    }

    void connectToSceneLayoutMovement(NameObj* pObj) {
        MR::connectToScene(pObj, MR::MovementType_Layout, -1, -1, -1);
    }

    void connectToSceneMovie(NameObj* pObj) {
        MR::connectToScene(pObj, MR::MovementType_Movie, -1, -1, MR::DrawType_Movie);
    }

    void connectToSceneMirrorMapObj(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_MapObj, MR::CalcAnimType_MirrorMapObj, MR::DrawBufferType_MirrorMapObj, -1);
    }

    void connectToSceneMirrorMapObjDecoration(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_MapObjDecoration, MR::CalcAnimType_MirrorMapObj, MR::DrawBufferType_MirrorMapObj, -1);
    }

    void connectToSceneMirrorMapObjNoMovement(LiveActor* pActor) {
        MR::connectToScene(pActor, -1, MR::CalcAnimType_MirrorMapObj, MR::DrawBufferType_MirrorMapObj, -1);
    }

    void connectToSceneCamera(NameObj* pObj) {
        MR::connectToScene(pObj, MR::MovementType_Camera, -1, -1, -1);
    }

    void connectToSceneNoShadowedMapObj(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_MapObj, MR::CalcAnimType_MapObj, MR::DrawBufferType_NoShadowedMapObj, -1);
    }

    void connectToSceneNoShadowedMapObjStrongLight(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_MapObj, MR::CalcAnimType_MapObj, MR::DrawBufferType_NoShadowedMapObjStrongLight, -1);
    }

    void connectToSceneNoSilhouettedMapObj(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_MapObj, MR::CalcAnimType_MapObj, MR::DrawBufferType_NoSilhouettedMapObj, -1);
    }

    void connectToSceneNoSilhouettedMapObjStrongLight(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_MapObj, MR::CalcAnimType_MapObj, MR::DrawBufferType_NoSilhouettedMapObjStrongLight, -1);
    }

    void connectToSceneNoSilhouettedMapObjWeakLightNoMovement(LiveActor* pActor) {
        MR::connectToScene(pActor, -1, MR::CalcAnimType_MapObj, MR::DrawBufferType_NoSilhouettedMapObjWeakLight, -1);
    }

    void connectToSceneSky(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_Sky, MR::CalcAnimType_MapObj, MR::DrawBufferType_Sky, -1);
    }

    void connectToSceneAir(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_Sky, MR::CalcAnimType_MapObj, MR::DrawBufferType_Air, -1);
    }

    void connectToSceneSun(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_Sky, MR::CalcAnimType_MapObj, MR::DrawBufferType_Sun, -1);
    }

    void connectToSceneCrystal(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_MapObj, MR::CalcAnimType_MapObj, MR::DrawBufferType_Crystal, -1);
    }

    void connectToSceneNormalMapObj(LiveActor* pActor) {
        MR::connectToScene(pActor, MR::MovementType_MapObj, MR::CalcAnimType_MapObj, -1, 0x18);  // ??
    }
}
