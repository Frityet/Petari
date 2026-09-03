#include "Game/Effect/ParticleResourceHolder.hpp"
#include "Game/Util/FileUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include "JSystem/JKernel/JKRMemArchive.hpp"
#include <cstring>

ParticleResourceHolder::ParticleResourceHolder(const char* pArchiveName)
    : mResourceMgr(), mAutoEffectList(new JMapInfo()), mParticleNames(new JMapInfo()), mNumParticles() {
    JKRMemArchive* archive = MR::mountArchive(pArchiveName, nullptr);
    mResourceMgr = new JPAResourceManager(archive->getResource("Particles.jpc"), MR::getCurrentHeap());
    mParticleNames->attach(archive->getResource("ParticleNames.bcsv"));
    mAutoEffectList->attach(archive->getResource("AutoEffectList.bcsv"));
    countAutoEffectNum();
}

u16 ParticleResourceHolder::getUserIndex(const char* pName) const {
    const JMapInfo* names = mParticleNames;
    s32 first = 0;
    s32 last = names->getNumEntries() - 1;

    while (first < last) {
        s32 middle = (first + last) / 2;
        const char* name = "";
        if (!names->getValue(middle, "name", &name)) {
            return 0xFFFF;
        }

        s32 comparison = strcmp(name, pName);
        if (comparison == 0) {
            return middle;
        }
        if (comparison < 0) {
            first = middle + 1;
        }
        if (comparison >= 0) {
            last = middle;
        }
    }

    const char* name = "";
    if (!names->getValue(first, "name", &name)) {
        return 0xFFFF;
    }
    return strcmp(name, pName) == 0 ? first : 0xFFFF;
}

void ParticleResourceHolder::countAutoEffectNum() {
    for (JMapInfoIter iter(mAutoEffectList, 0); iter != mAutoEffectList->end(); iter.mIndex++) {
        const char* groupName = nullptr;
        iter.getValue("GroupName", &groupName);
        if (groupName == nullptr) {
            continue;
        }

        bool isFound = false;
        for (Particle** particle = mParticles; particle != mParticles + mNumParticles; particle++) {
            if (MR::isEqualStringCase((*particle)->mGroupName, groupName)) {
                (*particle)->mCount++;
                isFound = true;
                break;
            }
        }

        if (!isFound) {
            mParticles[mNumParticles++] = new Particle(groupName);
        }
    }
}

void ParticleResourceHolder::swapTexture(const ResTIMG* pImage, const char* pName) {
    mResourceMgr->swapTexture(pImage, pName);
}

bool ParticleResourceHolder::isExistInResource(const char* pName, u16* pIndex) const {
    u16 index = getUserIndex(pName);
    if (index == 0xFFFF) {
        return false;
    }

    if (pIndex != nullptr) {
        *pIndex = index;
    }
    return true;
}

JMapInfo* ParticleResourceHolder::getAutoEffectListBinary() const {
    return mAutoEffectList;
}

int ParticleResourceHolder::getAutoEffectNum(const char* pName) const {
    if (pName == nullptr) {
        return 0;
    }

    for (Particle* const* particle = mParticles; particle != mParticles + mNumParticles; particle++) {
        if (MR::isEqualStringCase((*particle)->mGroupName, pName)) {
            return (*particle)->mCount;
        }
    }
    return 0;
}
