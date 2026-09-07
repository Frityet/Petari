// Original LiveActorUtil collision wrappers; resource decoding and ownership
// enter the native boundary only where the retail helper obtains KCL bytes.
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/CollisionParts.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/System/ResourceHolder.hpp"
#include <cstdio>
#include "compat/CollisionPartsCompat.hpp"

namespace {
void calcCollisionMtx(TPos3f* pMtx, const LiveActor* pActor) NO_INLINE {
        pMtx->set(pActor->getBaseMtx());

        f32 scale = pActor->mScale.x;
        pMtx->mMtx[0][0] *= scale;
        pMtx->mMtx[0][1] *= scale;
        pMtx->mMtx[0][2] *= scale;
        pMtx->mMtx[1][0] *= scale;
        pMtx->mMtx[1][1] *= scale;
        pMtx->mMtx[1][2] *= scale;
        pMtx->mMtx[2][0] *= scale;
        pMtx->mMtx[2][1] *= scale;
        pMtx->mMtx[2][2] *= scale;
    }
CollisionParts* createCollisionParts(ResourceHolder* resources, const char* name,
                                     HitSensor* sensor, const TPos3f& matrix,
                                     MR::CollisionScaleType scale, s32 category) {
    return smgpc::compat::create_collision_parts(resources, name, sensor, matrix, scale, category);
}
}
namespace MR {
void initCollisionParts(LiveActor* pActor, const char* pName, HitSensor* pSensor, MtxPtr pMtx) {
        pActor->initActorCollisionParts(pName, pSensor, nullptr, pMtx, false, false);
    }

void initCollisionPartsAutoEqualScale(LiveActor* pActor, const char* pName, HitSensor* pSensor, MtxPtr pMtx) {
        pActor->initActorCollisionParts(pName, pSensor, nullptr, pMtx, true, false);
    }

void initCollisionPartsAutoEqualScaleOne(LiveActor* pActor, const char* pName, HitSensor* pSensor, MtxPtr pMtx) {
        pActor->initActorCollisionParts(pName, pSensor, nullptr, pMtx, true, true);
    }

void initCollisionPartsFromResourceHolder(LiveActor* pActor, const char* pName, HitSensor* pSensor, ResourceHolder* pResHolder, MtxPtr pMtx) {
        pActor->initActorCollisionParts(pName, pSensor, pResHolder, pMtx, false, false);
    }

CollisionParts* createCollisionPartsFromLiveActor(LiveActor* pActor, const char* pName, HitSensor* pSensor, CollisionScaleType scaleType) {
        TPos3f mtx;
        makeMtxTRS(mtx, pActor);
        return ::createCollisionParts(getResourceHolder(pActor), pName, pSensor, mtx, scaleType, 0);
    }

CollisionParts* createCollisionPartsFromLiveActor(LiveActor* pActor, const char* pName, HitSensor* pSensor, MtxPtr pMtx,
                                                      CollisionScaleType scaleType) {
        TPos3f mtx;
        mtx.set(pMtx);
        CollisionParts* pParts = ::createCollisionParts(getResourceHolder(pActor), pName, pSensor, mtx, scaleType, 0);
        pParts->_0 = reinterpret_cast< TPos3f* >(pMtx);
        return pParts;
    }

CollisionParts* createCollisionPartsFromResourceHolder(ResourceHolder* pResHolder, const char* pName, HitSensor* pSensor, const TPos3f& rMtx,
                                                           CollisionScaleType scaleType) {
        return ::createCollisionParts(pResHolder, pName, pSensor, rMtx, scaleType, 0);
    }

f32 getCollisionBoundingSphereRange(const LiveActor* pActor) {
        return pActor->mCollisionParts->_D8;
    }

bool isValidCollisionParts(LiveActor* pActor) {
        return pActor->mCollisionParts->_CC;
    }

void validateCollisionParts(LiveActor* pActor) {
        validateCollisionParts(pActor->mCollisionParts);
        CollisionParts* pParts = pActor->mCollisionParts;

        if (pParts->_0 != nullptr) {
            pParts->updateBoundingSphereRange();
        } else {
            pParts->updateBoundingSphereRange(pActor->mScale);
        }

        resetAllCollisionMtx(pActor);
    }

void validateCollisionParts(CollisionParts* pParts) {
        if (!pParts->_CC) {
            pParts->addToBelongZone();
        }

        pParts->_CC = true;
    }

void invalidateCollisionParts(LiveActor* pActor) {
        invalidateCollisionParts(pActor->mCollisionParts);
    }

void invalidateCollisionParts(CollisionParts* pParts) {
        if (pParts->_CC) {
            pParts->removeFromBelongZone();
        }

        pParts->_CC = false;
    }

void onUpdateCollisionParts(LiveActor* pActor) {
        CollisionParts* pParts = pActor->mCollisionParts;
        if (!pParts->_CC) {
            validateCollisionParts(pActor);
        }

        pActor->mCollisionParts->_CD = true;
    }

void onUpdateCollisionPartsOnetimeImmediately(LiveActor* pActor) {
        CollisionParts* pParts = pActor->mCollisionParts;
        if (!pParts->_CC) {
            validateCollisionParts(pActor);
        }

        pParts = pActor->mCollisionParts;
        pParts->_CE = true;
        makeMtxTR(pActor->getBaseMtx(), pActor);
        setCollisionMtx(pActor, pActor->mCollisionParts);
        pParts->updateMtx();
    }

void offUpdateCollisionParts(LiveActor* pActor) {
        CollisionParts* pParts = pActor->mCollisionParts;
        if (!pParts->_CC) {
            validateCollisionParts(pActor);
        }

        pActor->mCollisionParts->_CD = false;
    }

void resetAllCollisionMtx(LiveActor* pActor) {
        CollisionParts* pParts = pActor->mCollisionParts;
        if (pParts->_0 != nullptr) {
            pParts->resetAllMtx();
            return;
        }

        TPos3f mtx;
        ::calcCollisionMtx(&mtx, pActor);
        pParts->resetAllMtx(mtx);
    }

void setCollisionMtx(LiveActor* pActor) {
        setCollisionMtx(pActor, getCollisionParts(pActor));
    }

void setCollisionMtx(LiveActor* pActor, CollisionParts* pParts) {
        if (!pParts->_CC) {
            return;
        }

        if (pParts->_0 != nullptr) {
            pParts->setMtx();
            return;
        }

        TPos3f mtx;
        ::calcCollisionMtx(&mtx, pActor);
        pParts->setMtx(mtx);
    }

CollisionParts* getCollisionParts(const LiveActor* pActor) {
        return pActor->mCollisionParts;
    }

bool isExistCollisionParts(const LiveActor* pActor) {
        return pActor->mCollisionParts != nullptr;
    }
CollisionParts* tryCreateCollisionMoveLimit(LiveActor* pActor, HitSensor* pSensor) {
        char kcl[0x80];
        {
            const char* pKclName = "MoveLimit";
            snprintf(kcl, sizeof(kcl), "%s.kcl", pKclName);
        }

        if (!MR::getResourceHolder(pActor)->mFileInfoTable->isExistRes(kcl)) {
            return nullptr;
        }

        TPos3f mtx;
        const char* pCollisionName = "MoveLimit";
        MR::makeMtxTRS(mtx, pActor);
        CollisionParts* parts = ::createCollisionParts(MR::getResourceHolder(pActor), pCollisionName, pSensor, mtx, MR::CollisionScaleType_Unk2, 3);
        if (parts != nullptr) {
            MR::validateCollisionParts(parts);
        }

        return parts;
    }
CollisionParts* tryCreateCollisionWaterSurface(LiveActor* pActor, HitSensor* pSensor) {
        char kcl[0x80];
        {
            const char* pKclName = "WaterSurface";
            snprintf(kcl, sizeof(kcl), "%s.kcl", pKclName);
        }

        if (!MR::getResourceHolder(pActor)->mFileInfoTable->isExistRes(kcl)) {
            return nullptr;
        }

        TPos3f mtx;
        const char* pCollisionName = "WaterSurface";
        MR::makeMtxTRS(mtx, pActor);
        CollisionParts* parts = ::createCollisionParts(MR::getResourceHolder(pActor), pCollisionName, pSensor, mtx, MR::CollisionScaleType_Unk2, 2);
        if (parts != nullptr) {
            MR::validateCollisionParts(parts);
        }

        return parts;
    }
CollisionParts* tryCreateCollisionSunshade(LiveActor* pActor, HitSensor* pSensor) {
        char kcl[0x80];
        {
            const char* pKclName = "Sunshade";
            snprintf(kcl, sizeof(kcl), "%s.kcl", pKclName);
        }

        if (!MR::getResourceHolder(pActor)->mFileInfoTable->isExistRes(kcl)) {
            return nullptr;
        }

        TPos3f mtx;
        const char* pCollisionName = "Sunshade";
        MR::makeMtxTRS(mtx, pActor);
        CollisionParts* parts = ::createCollisionParts(MR::getResourceHolder(pActor), pCollisionName, pSensor, mtx, MR::CollisionScaleType_Unk2, 1);
        if (parts != nullptr) {
            MR::validateCollisionParts(parts);
        }

        return parts;
    }
}
