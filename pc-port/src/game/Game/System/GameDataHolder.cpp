#include "Game/System/GameDataHolder.hpp"

#include "Game/System/BinaryDataChunkHolder.hpp"
#include "Game/System/UserFile.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "compat/SaveDataEndian.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <utility>
#include <vector>

namespace {

    constexpr u32 GAME_DATA_CHUNK_BUFFER_SIZE = 0x1000U;
    constexpr s32 GAME_DATA_CHUNK_COUNT = 6;

    constexpr u32 SIGNATURE_FLG1 = 0x464C4731U;
    constexpr u32 SIGNATURE_PCE1 = 0x50434531U;
    constexpr u32 SIGNATURE_SPN1 = 0x53504E31U;
    constexpr u32 SIGNATURE_VLE1 = 0x564C4531U;
    constexpr u32 SIGNATURE_GALA = 0x47414C41U;

    constexpr u32 HASH_FLG1 = 0x65020442U;
    constexpr u32 HASH_PCE1 = 0xF5DE1DC0U;
    constexpr u32 HASH_SPN1 = 0x12345679U;
    constexpr u32 HASH_VLE1 = 0x564C4531U;
    constexpr u32 HASH_GALA = 0xBF0640EEU;

    constexpr u32 DEFAULT_PCE1_PAYLOAD_SIZE = 32U;
    constexpr u32 DEFAULT_SPN1_PAYLOAD_SIZE = 643U;
    constexpr u32 DEFAULT_GALA_ENTRY_SIZE = 20U;

    constexpr u16 DEFAULT_FLG1_VALUES[]{
        0x7ABAU, 0x0C8EU, 0x184FU, 0x0E5EU, 0x62E5U, 0x5965U, 0x4C23U, 0x1880U, 0x3D66U, 0x2CD5U, 0x4E78U, 0x21C7U, 0x7914U, 0x7C3AU, 0x0F4BU,
        0x51A3U, 0x7C94U, 0x6C74U, 0x0602U, 0x3764U, 0x1389U, 0x389AU, 0x7D30U, 0x502BU, 0x1663U, 0x185DU, 0x0086U, 0x6701U, 0x1E54U, 0x1057U,
        0x3F0FU, 0x0AF7U, 0x462AU, 0x0D7AU, 0x16BAU, 0x3871U, 0x4999U, 0x3CDEU, 0x47D8U, 0x009AU, 0x13ECU, 0x485AU, 0x42ACU, 0x58A1U, 0x7C01U,
        0x017DU, 0x7C30U, 0x73B2U, 0x4143U, 0x4A8CU, 0x51A4U, 0x59BDU, 0x5763U, 0x4290U, 0x32ACU, 0x69E5U, 0x2AE2U,
    };

    struct DefaultScenarioRecord {
        u16 mHash;
        u8 mScenarioCount;
    };

    constexpr u8 DEFAULT_SPN1_FIRST_RECORD_HEADER[]{
        0x30U, 0xBCU, 0x0FU, 0x00U, 0x15U, 0x05U,
    };

    constexpr DefaultScenarioRecord DEFAULT_SPN1_RECORDS[]{
        {0x7022U, 6U}, {0xEC16U, 1U}, {0xF2EEU, 1U}, {0x64E5U, 1U}, {0xAC16U, 2U}, {0x21C7U, 5U}, {0x7914U, 5U}, {0x8F4BU, 1U},
        {0x7C3AU, 1U}, {0xD1A3U, 1U}, {0xFC94U, 1U}, {0xEC74U, 5U}, {0x8602U, 1U}, {0xB764U, 5U}, {0x1389U, 1U}, {0x389AU, 1U},
        {0x7D30U, 1U}, {0x502BU, 5U}, {0x1663U, 1U}, {0x185DU, 5U}, {0x8086U, 1U}, {0x6701U, 1U}, {0x9E54U, 1U}, {0x9057U, 1U},
        {0xBF0FU, 5U}, {0x8AF7U, 5U}, {0xC62AU, 1U}, {0x8D7AU, 5U}, {0x16BAU, 1U}, {0xB871U, 1U}, {0xC999U, 5U}, {0x3CDEU, 5U},
        {0x47D8U, 1U}, {0x809AU, 5U}, {0x13ECU, 1U}, {0x485AU, 1U}, {0xC2ACU, 5U}, {0xD8A1U, 5U}, {0xFC01U, 1U}, {0x017DU, 5U},
        {0xFC30U, 1U}, {0xF3B2U, 1U}, {0xC143U, 1U}, {0xCA8CU, 1U}, {0x51A4U, 1U}, {0xD9BDU, 1U}, {0xE5B8U, 1U},
    };

    constexpr u32 DEFAULT_VLE1_VALUES[]{
        0x4E3C0000U, 0x4EBE1518U, 0x8D990000U, 0x8E1B1518U, 0x3DF10000U, 0x3E731518U, 0x19B30000U, 0x1A351518U, 0xD68D0000U,
        0xD70F1518U, 0x8E420001U, 0x9AC30000U, 0x95460001U, 0x0479FF00U, 0xBDC00000U, 0x7C330000U, 0x7BA70000U, 0x72DB0000U,
        0xB4CA0000U, 0x27B30000U, 0x62F40000U, 0x9E350000U, 0xD9760000U, 0x14B70000U, 0x4FF80000U,
    };

    constexpr u8 DEFAULT_GALA_PREFIX[]{
        0x00U, 0x2AU, 0x00U, 0x04U, 0x00U, 0x14U, 0x82U, 0x08U, 0x00U, 0x00U, 0x21U,
        0x96U, 0x00U, 0x02U, 0xD4U, 0x23U, 0x00U, 0x03U, 0x81U, 0x7EU, 0x00U, 0x04U,
    };

    constexpr u16 DEFAULT_GALA_GALAXY_HASHES[]{
        0xAC16U, 0x21C7U, 0x7914U, 0x8F4BU, 0x7C3AU, 0xD1A3U, 0xFC94U, 0xEC74U, 0x8602U, 0xB764U, 0x1389U, 0x389AU, 0x7D30U, 0x502BU,
        0x1663U, 0x185DU, 0x8086U, 0x6701U, 0x9E54U, 0x9057U, 0xBF0FU, 0x8AF7U, 0xC62AU, 0x8D7AU, 0x16BAU, 0xB871U, 0xC999U, 0x3CDEU,
        0x47D8U, 0x809AU, 0x13ECU, 0x485AU, 0xC2ACU, 0xD8A1U, 0xFC01U, 0x017DU, 0xFC30U, 0xF3B2U, 0xC143U, 0xCA8CU, 0x51A4U, 0xD9BDU,
    };

    constexpr u32 DEFAULT_GALA_PAYLOAD_SIZE =
        sizeof(DEFAULT_GALA_PREFIX) + DEFAULT_GALA_ENTRY_SIZE * (sizeof(DEFAULT_GALA_GALAXY_HASHES) / sizeof(DEFAULT_GALA_GALAXY_HASHES[0]));

    constexpr u32 calcDefaultScenarioPayloadSize() {
        u32 size = sizeof(DEFAULT_SPN1_FIRST_RECORD_HEADER) + 1U + 5U * 3U;
        for (const DefaultScenarioRecord& record : DEFAULT_SPN1_RECORDS) {
            size += 6U + static_cast< u32 >(record.mScenarioCount) * 3U;
        }
        return size;
    }

    static_assert(sizeof(DEFAULT_FLG1_VALUES) == 114U);
    static_assert(calcDefaultScenarioPayloadSize() == DEFAULT_SPN1_PAYLOAD_SIZE);
    static_assert(sizeof(DEFAULT_VLE1_VALUES) == 100U);
    static_assert(DEFAULT_GALA_PAYLOAD_SIZE == 862U);

    void writeDefaultScenarioValues(u8* pBuffer, u8 scenarioCount) {
        pBuffer[0] = 0U;

        u32 offset = 1U;
        for (u8 idx = 0U; idx < scenarioCount; ++idx) {
            pBuffer[offset] = 0U;
            pBuffer[offset + 1U] = 0x03U;
            pBuffer[offset + 2U] = 0xFFU;
            offset += 3U;
        }
    }

    [[nodiscard]] std::vector< u8 > makeDefaultFlagPayload() {
        std::vector< u8 > payload(sizeof(DEFAULT_FLG1_VALUES));
        u32 offset = 0U;
        for (u16 value : DEFAULT_FLG1_VALUES) {
            SaveDataEndian::write_u16(payload.data() + offset, value);
            offset += sizeof(u16);
        }
        return payload;
    }

    [[nodiscard]] std::vector< u8 > makeDefaultPictureBookPayload() {
        return std::vector< u8 >(DEFAULT_PCE1_PAYLOAD_SIZE, 0U);
    }

    [[nodiscard]] std::vector< u8 > makeDefaultScenarioPayload() {
        std::vector< u8 > payload(DEFAULT_SPN1_PAYLOAD_SIZE);
        MR::copyMemory(payload.data(), DEFAULT_SPN1_FIRST_RECORD_HEADER, sizeof(DEFAULT_SPN1_FIRST_RECORD_HEADER));
        u32 offset = sizeof(DEFAULT_SPN1_FIRST_RECORD_HEADER);
        writeDefaultScenarioValues(payload.data() + offset, 5U);
        offset += 1U + 5U * 3U;

        for (const DefaultScenarioRecord& record : DEFAULT_SPN1_RECORDS) {
            const u32 recordSize = 6U + static_cast< u32 >(record.mScenarioCount) * 3U;
            SaveDataEndian::write_u16(payload.data() + offset, record.mHash);
            SaveDataEndian::write_u16(payload.data() + offset + 2U, static_cast< u16 >(recordSize));
            payload[offset + 4U] = record.mScenarioCount;
            writeDefaultScenarioValues(payload.data() + offset + 5U, record.mScenarioCount);
            offset += recordSize;
        }

        return payload;
    }

    [[nodiscard]] std::vector< u8 > makeDefaultValuePayload() {
        std::vector< u8 > payload(sizeof(DEFAULT_VLE1_VALUES));
        u32 offset = 0U;
        for (u32 value : DEFAULT_VLE1_VALUES) {
            SaveDataEndian::write_u32(payload.data() + offset, value);
            offset += sizeof(u32);
        }
        return payload;
    }

    [[nodiscard]] std::vector< u8 > makeDefaultGalaxyPayload() {
        std::vector< u8 > payload(DEFAULT_GALA_PAYLOAD_SIZE);
        MR::copyMemory(payload.data(), DEFAULT_GALA_PREFIX, sizeof(DEFAULT_GALA_PREFIX));
        u32 offset = sizeof(DEFAULT_GALA_PREFIX);
        for (u16 galaxyHash : DEFAULT_GALA_GALAXY_HASHES) {
            SaveDataEndian::write_u16(payload.data() + offset, galaxyHash);
            MR::zeroMemory(payload.data() + offset + sizeof(u16), DEFAULT_GALA_ENTRY_SIZE - sizeof(u16));
            offset += DEFAULT_GALA_ENTRY_SIZE;
        }
        return payload;
    }

}  // namespace

class GameDataRawChunk : public BinaryDataChunkBase {
public:
    GameDataRawChunk(u32 signature, u32 hash, std::vector< u8 > defaultPayload)
        : mSignature(signature), mHash(hash), mDefaultPayload(std::move(defaultPayload)), mPayload(mDefaultPayload) {
    }

    u32 makeHeaderHashCode() const override {
        return mHash;
    }

    u32 getSignature() const override {
        return mSignature;
    }

    s32 serialize(u8* pBuffer, u32 size) const override {
        if (pBuffer == nullptr || size < mPayload.size()) {
            return 0;
        }

        MR::copyMemory(pBuffer, mPayload.data(), static_cast< u32 >(mPayload.size()));
        return static_cast< s32 >(mPayload.size());
    }

    s32 deserialize(const u8* pBuffer, u32 size) override {
        initializeData();
        if (pBuffer == nullptr) {
            return 1;
        }

        const u32 copySize = std::min< u32 >(size, static_cast< u32 >(mPayload.size()));
        MR::copyMemory(mPayload.data(), pBuffer, copySize);
        return 0;
    }

    void initializeData() override {
        mPayload = mDefaultPayload;
    }

private:
    u32 mSignature;
    u32 mHash;
    std::vector< u8 > mDefaultPayload;
    std::vector< u8 > mPayload;
};

class GameDataPlayerStatusChunk : public BinaryDataChunkBase {
public:
    GameDataPlayerStatusChunk() {
        initializeData();
    }

    u32 makeHeaderHashCode() const override {
        return 0x27C90FU;
    }

    u32 getSignature() const override {
        return 0x504C4159U;
    }

    s32 serialize(u8* pBuffer, u32 size) const override {
        if (pBuffer == nullptr || size < 7U) {
            return 0;
        }

        pBuffer[0] = mStoryProgress;
        SaveDataEndian::write_u32(pBuffer + 1U, mStockedStarPiece);
        SaveDataEndian::write_u16(pBuffer + 5U, mPlayerLeft);
        return 7;
    }

    s32 deserialize(const u8* pBuffer, u32 size) override {
        initializeData();
        if (pBuffer == nullptr || size < 5U) {
            return 1;
        }

        mStoryProgress = pBuffer[0];
        mStockedStarPiece = SaveDataEndian::read_u32(pBuffer + 1U);
        if (size >= 7U) {
            mPlayerLeft = SaveDataEndian::read_u16(pBuffer + 5U);
        }
        return 0;
    }

    void initializeData() override {
        mPlayerLeft = 4U;
        mPlayerLeftSupply = 4U;
        mStockedStarPiece = 0U;
        mStoryProgress = 0U;
        mMissNum = 0;
    }

    s32 getPlayerLeft() const {
        return std::clamp< s32 >(mPlayerLeft, 0, 99);
    }

    void addPlayerLeft(int num) {
        mPlayerLeft = static_cast< u16 >(std::clamp< s32 >(static_cast< s32 >(mPlayerLeft) + num, 0, 99));
    }

    bool isPlayerLeftSupply() const {
        return mPlayerLeftSupply >= 10U;
    }

    void offPlayerLeftSupply() {
        mPlayerLeftSupply = 0U;
    }

    void addStockedStarPiece(int num) {
        mStockedStarPiece = static_cast< u32 >(std::clamp< s32 >(static_cast< s32 >(mStockedStarPiece) + num, 0, 9999));
    }

    /* 0x04 */ u16 mPlayerLeft{};
    /* 0x08 */ u32 mStockedStarPiece{};
    /* 0x0C */ u8 mStoryProgress{};
    /* 0x0E */ u16 mPlayerLeftSupply{};
    /* 0x10 */ s32 mMissNum{};
};

GameDataHolder::GameDataHolder(const UserFile* pUserFile)
    : mChunkHolder(new BinaryDataChunkHolder(GAME_DATA_CHUNK_BUFFER_SIZE, GAME_DATA_CHUNK_COUNT)), mPlayerStatus(new GameDataPlayerStatusChunk()),
      mEventFlagStorage(new GameDataRawChunk(SIGNATURE_FLG1, HASH_FLG1, makeDefaultFlagPayload())),
      mPictureBookStorage(new GameDataRawChunk(SIGNATURE_PCE1, HASH_PCE1, makeDefaultPictureBookPayload())),
      mSpinDriverPathStorage(new GameDataRawChunk(SIGNATURE_SPN1, HASH_SPN1, makeDefaultScenarioPayload())),
      mEventValueStorage(new GameDataRawChunk(SIGNATURE_VLE1, HASH_VLE1, makeDefaultValuePayload())),
      mAllGalaxyStorage(new GameDataRawChunk(SIGNATURE_GALA, HASH_GALA, makeDefaultGalaxyPayload())), mUserFile(pUserFile) {
    mChunkHolder->addChunk(mPlayerStatus);
    mChunkHolder->addChunk(mEventFlagStorage);
    mChunkHolder->addChunk(mPictureBookStorage);
    mChunkHolder->addChunk(mSpinDriverPathStorage);
    mChunkHolder->addChunk(mEventValueStorage);
    mChunkHolder->addChunk(mAllGalaxyStorage);
    resetAllData();
    std::snprintf(mName, sizeof(mName), "mario1");
}

GameDataHolder::~GameDataHolder() {
    delete mChunkHolder;
    delete mPlayerStatus;
    delete mEventFlagStorage;
    delete mPictureBookStorage;
    delete mSpinDriverPathStorage;
    delete mEventValueStorage;
    delete mAllGalaxyStorage;
}

bool GameDataHolder::isDataMario() const {
    return std::strstr(mName, "mario") != nullptr;
}

s32 GameDataHolder::getGalaxyNumCanOpen() const {
    return 0;
}

bool GameDataHolder::canOnGameEventFlag(const char*) const {
    return false;
}

bool GameDataHolder::isOnGameEventFlag(const char*) const {
    return false;
}

void GameDataHolder::tryOnGameEventFlag(const char*) {
}

u16 GameDataHolder::getGameEventValue(const char*) const {
    return 0U;
}

void GameDataHolder::setGameEventValue(const char*, u16) {
}

bool GameDataHolder::isOnGameEventValueForBit(const char*, int) const {
    return false;
}

void GameDataHolder::setGameEventValueForBit(const char*, int, bool) {
}

s32 GameDataHolder::getPictureBookChapterCanRead() const {
    return 0;
}

u16 GameDataHolder::getPictureBookChapterAlreadyRead() const {
    return 0U;
}

void GameDataHolder::setPictureBookChapterAlreadyRead(int) {
}

void GameDataHolder::setRaceBestTime(const char*, u32) {
}

u32 GameDataHolder::getRaceBestTime(const char*) const {
    return 0U;
}

void GameDataHolder::addMissPoint(int) {
}

void GameDataHolder::resetMissPoint() {
}

bool GameDataHolder::isPointCollectForLetter() const {
    return false;
}

void GameDataHolder::incPlayerMissNum() {
    ++mPlayerStatus->mMissNum;
}

s32 GameDataHolder::getPlayerMissNum() const {
    return mPlayerStatus->mMissNum;
}

bool GameDataHolder::hasPowerStar(const char*, s32) const {
    return false;
}

bool GameDataHolder::hasGrandStar(int) const {
    return false;
}

void GameDataHolder::setPowerStar(const char*, s32, bool) {
}

s32 GameDataHolder::getPowerStarNumOwned(const char*) const {
    return 0;
}

s32 GameDataHolder::calcCurrentPowerStarNum() const {
    return 0;
}

bool GameDataHolder::isOnGalaxyScenarioFlagAlreadyVisited(const char*, s32) const {
    return false;
}

void GameDataHolder::onGalaxyScenarioFlagAlreadyVisited(const char*, s32) {
}

bool GameDataHolder::isAppearGalaxy(const char*) const {
    return true;
}

s32 GameDataHolder::getPlayerLeft() const {
    return mPlayerStatus->getPlayerLeft();
}

void GameDataHolder::addPlayerLeft(int num) {
    mPlayerStatus->addPlayerLeft(num);
}

bool GameDataHolder::isPlayerLeftSupply() const {
    return mPlayerStatus->isPlayerLeftSupply();
}

void GameDataHolder::offPlayerLeftSupply() {
    mPlayerStatus->offPlayerLeftSupply();
}

s32 GameDataHolder::getStockedStarPieceNum() const {
    return static_cast< s32 >(mPlayerStatus->mStockedStarPiece);
}

void GameDataHolder::addStockedStarPiece(int num) {
    mPlayerStatus->addStockedStarPiece(num);
}

s32 GameDataHolder::setupSpinDriverPathStorage(const char*, int, int, int, f32*) {
    return -1;
}

void GameDataHolder::updateSpinDriverPathStorage(const char*, int, int, f32) {
}

s32 GameDataHolder::getStarPieceNumGivingToTicoSeed(int) const {
    return 0;
}

u32 GameDataHolder::getStarPieceNumMaxGivingToTicoSeed(int) const {
    return 0U;
}

void GameDataHolder::addStarPieceGivingToTicoSeed(int, int) {
}

bool GameDataHolder::isCompleteMarioAndLuigi() const {
    return mUserFile != nullptr && mUserFile->isOnCompleteEndingMarioAndLuigi();
}

bool GameDataHolder::isPassedStoryEvent(const char*) const {
    return false;
}

void GameDataHolder::followStoryEventByName(const char*) {
}

void GameDataHolder::resetAllData() {
    mPlayerStatus->initializeData();
    mEventFlagStorage->initializeData();
    mPictureBookStorage->initializeData();
    mSpinDriverPathStorage->initializeData();
    mEventValueStorage->initializeData();
    mAllGalaxyStorage->initializeData();
}

s32 GameDataHolder::makeFileBinary(u8* pBuffer, u32 size) {
    return static_cast< s32 >(mChunkHolder->makeFileBinary(pBuffer, size));
}

bool GameDataHolder::loadFromFileBinary(const char* pName, const u8* pBuffer, u32 size) {
    std::snprintf(mName, sizeof(mName), "%s", pName != nullptr ? pName : "mario1");
    return mChunkHolder->loadFromFileBinary(pBuffer, size);
}
