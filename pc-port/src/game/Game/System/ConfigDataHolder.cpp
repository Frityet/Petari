#include "Game/System/ConfigDataHolder.hpp"

#include "Game/System/ConfigDataMii.hpp"
#include "Game/System/ConfigDataMisc.hpp"

#include <cstdio>

ConfigDataHolder::ConfigDataHolder()
    : mChunkHolder(new BinaryDataChunkHolder(64U, 3)),
      mCreateChunk(new ConfigDataCreateChunk()),
      mMii(new ConfigDataMii()),
      mMisc(new ConfigDataMisc()) {
    mChunkHolder->addChunk(mCreateChunk);
    mChunkHolder->addChunk(mMii);
    mChunkHolder->addChunk(mMisc);
    resetAllData();
    std::snprintf(mName, sizeof(mName), "config1");
}

ConfigDataHolder::~ConfigDataHolder() {
    delete mChunkHolder;
    delete mCreateChunk;
    delete mMii;
    delete mMisc;
}

void ConfigDataHolder::setIsCreated(bool isCreated) {
    mCreateChunk->mIsCreated = isCreated;
}

bool ConfigDataHolder::isCreated() const {
    return mCreateChunk->mIsCreated;
}

void ConfigDataHolder::setLastLoadedMario(bool lastLoadedMario) {
    mMisc->setLastLoadedMario(lastLoadedMario);
}

bool ConfigDataHolder::isLastLoadedMario() const {
    return mMisc->isLastLoadedMario();
}

void ConfigDataHolder::onCompleteEndingMario() {
    mMisc->onCompleteEndingMario();
}

void ConfigDataHolder::onCompleteEndingLuigi() {
    mMisc->onCompleteEndingLuigi();
}

bool ConfigDataHolder::isOnCompleteEndingMario() {
    return mMisc->isOnCompleteEndingMario();
}

bool ConfigDataHolder::isOnCompleteEndingLuigi() {
    return mMisc->isOnCompleteEndingLuigi();
}

void ConfigDataHolder::updateLastModified() {
    mMisc->updateLastModified();
}

OSTime ConfigDataHolder::getLastModified() const {
    return mMisc->getLastModified();
}

void ConfigDataHolder::setMiiOrIconId(const void *pMiiId, const u32 *pIconId) {
    mMii->setMiiOrIconId(pMiiId, pIconId);
}

bool ConfigDataHolder::getMiiId(void *pMiiId) const {
    return mMii->getMiiId(pMiiId);
}

bool ConfigDataHolder::getIconId(u32 *pIconId) const {
    return mMii->getIconId(pIconId);
}

void ConfigDataHolder::resetAllData() {
    mCreateChunk->initializeData();
    mMii->initializeData();
    mMisc->initializeData();
}

s32 ConfigDataHolder::makeFileBinary(u8 *pBuffer, u32 size) {
    return static_cast<s32>(mChunkHolder->makeFileBinary(pBuffer, size));
}

bool ConfigDataHolder::loadFromFileBinary(const char *pName, const u8 *pBuffer, u32 size) {
    std::snprintf(mName, sizeof(mName), "%s", pName != nullptr ? pName : "config1");
    return mChunkHolder->loadFromFileBinary(pBuffer, size);
}

ConfigDataCreateChunk::ConfigDataCreateChunk()
    : mIsCreated(false) {
    initializeData();
}

u32 ConfigDataCreateChunk::makeHeaderHashCode() const {
    return 0x2432DAU;
}

u32 ConfigDataCreateChunk::getSignature() const {
    return 0x434F4E46U;
}

s32 ConfigDataCreateChunk::serialize(u8 *pBuffer, u32 size) const {
    if (pBuffer == nullptr || size < 1U) {
        return 0;
    }

    pBuffer[0] = mIsCreated ? 0xFFU : 0U;
    return 1;
}

s32 ConfigDataCreateChunk::deserialize(const u8 *pBuffer, u32 size) {
    initializeData();
    if (pBuffer == nullptr || size < 1U) {
        return 1;
    }

    mIsCreated = pBuffer[0] != 0U;
    return 0;
}

void ConfigDataCreateChunk::initializeData() {
    mIsCreated = false;
}
