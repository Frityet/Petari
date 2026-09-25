#include "Game/AudioLib/AudSoundNameConverter.hpp"
#include <JSystem/JAudio2/JAUSoundTable.hpp>
#include <JSystem/JGadget/hashcode.hpp>
#include <aurora/exception.hpp>
#include <array>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace {
    static const char* sCategoryNamePrefix[] = {
        "SY",   "PV", "PM", "BV", "BM", "OJ", "AT", "DM", "EV", "EM", "SV", "SM", "RS",  //
        "HO",   "BG", "MB", "ST",                                                        //
        "\0\0",                                                                          //
    };

    // static const u8 sGroupNoTable[] = { };
};  // namespace

AudSoundNameConverter::AudSoundNameConverter()
    : mNumItems(), mNumNameDatas(), mGroupItemOffsets(), mNumItemsSection0(), mNumItemsSection1(), mNumItemsSection2(), mSoundNameData() {
    try {
        init();
    } catch (...) {
        delete[] mSoundNameData;
        delete[] mGroupItemOffsets;
        throw;
    }
}

AudSoundNameConverter::~AudSoundNameConverter() {
    if (AudSingletonHolder< AudSoundNameConverter >::get() == this) {
        AudSingletonHolder< AudSoundNameConverter >::exchange(nullptr);
    }
    delete[] mSoundNameData;
    delete[] mGroupItemOffsets;
}

void AudSoundNameConverter::validateTable(const JAUSoundNameTable* table) {
    const auto invalid = [] {
        aurora::throw_host_exception< std::runtime_error >(
            "The sound-name resource does not satisfy the original converter's category/count contract");
    };
    constexpr std::array< int, 3 > groups = {14, 2, 1};
    if (!table || !table->mTable.mData || !table->mTable.mRoot || table->mTable.mRoot->mSectionNumber != groups.size()) {
        invalid();
    }
    std::size_t items = 0;
    for (u8 section = 0; section < groups.size(); section++) {
        if (table->getNumGroups_inSection(section) != groups[section]) {
            invalid();
        }
        for (u8 group = 0; group < groups[section]; group++) {
            const int count = table->getNumItems_inGroup(section, group);
            if (count < 0 || count >= 65536) {
                invalid();
            }
            items += count;
            for (int item = 0; item < count; item++) {
                JAISoundID id;
                id.set(section, group, item);
                if (!table->getName(id)) {
                    invalid();
                }
            }
        }
    }
    if (items > static_cast< std::size_t >(std::numeric_limits< s32 >::max())) {
        invalid();
    }
}

JAISoundID AudSoundNameConverter::getSoundID(const char* pName) const {
    return pName ? getSoundID(pName, JGadget::getHashCode(pName)) : JAISoundID(-1);
}

inline s32 AudSoundNameConverter::getSeSoundCategory(const char* pName) const {
    for (s32 i = 0; i < 13; i++) {
        if (pName[3] == sCategoryNamePrefix[i][0] && pName[4] == sCategoryNamePrefix[i][1]) {
            return i;
        }
    }
    return -1;
}

inline s32 AudSoundNameConverter::getOtherSoundCategory(const char* pName) const {
    for (s32 i = 13; i < 17; i++) {
        if (pName[0] == sCategoryNamePrefix[i][0] && pName[1] == sCategoryNamePrefix[i][1]) {
            return i;
        }
    }
    return -1;
}

JAISoundID AudSoundNameConverter::getSoundID(const char* pName, u32 hash) const {
    if (!pName || std::strlen(pName) < 2) {
        return -1;
    }
    bool isSE;
    if (pName[0] == 'S' && pName[1] == 'E') {
        isSE = true;
    } else {
        isSE = false;
    }

    if (isSE && std::strlen(pName) < 5) {
        return -1;
    }
    const s32 category = isSE ? getSeSoundCategory(pName) : getOtherSoundCategory(pName);
    if (category < 0 || category >= 17) {
        return -1;
    }
    const s32 startingOffset = mGroupItemOffsets[category];

    for (s32 i = startingOffset; i < mNumItems; i++) {
        const AudSoundNameData& data = mSoundNameData[i];
        if (hash == data.mHash && strcmp(pName, data.mName) == 0) {
            return data.mID;
        }
    }
    return -1;
}

void AudSoundNameConverter::init() {
    const JAUSoundNameTable* table = JAUSoundNameTable::getInstance();
    validateTable(table);
    initDataTable(table);
    addSectionInfo(table, 0);
    addSectionInfo(table, 1);
    addSectionInfo(table, 2);
}

void AudSoundNameConverter::initDataTable(const JAUSoundNameTable* pTable) {
    mGroupItemOffsets = new u32[17];
    mNumItems = 0;
    mNumItemsSection0 = getSoundNameTableNumItem_inSection(pTable, 0);
    mNumItemsSection1 = getSoundNameTableNumItem_inSection(pTable, 1);
    mNumItemsSection2 = getSoundNameTableNumItem_inSection(pTable, 2);

    mNumItems += mNumItemsSection0;
    mNumItems += mNumItemsSection1;
    mNumItems += mNumItemsSection2;

    mSoundNameData = new AudSoundNameData[mNumItems];

    for (s32 i = 0; i < mNumItems; i++) {
        mSoundNameData[i].mName = nullptr;
        mSoundNameData[i].mID.setAnonymous();
        mSoundNameData[i].mHash = 0;
    }
}

AudSoundNameData::AudSoundNameData() {
}

void AudSoundNameConverter::addGroupInfo(const JAUSoundNameTable* pTable, u8 sectionID, u8 groupID) {
    for (u16 i = 0; i < pTable->getNumItems_inGroup(sectionID, groupID); i++) {
        AudSoundNameData& data = mSoundNameData[mNumNameDatas];
        data.mID.set(sectionID, groupID, i);
        data.mName = pTable->getName(data.mID);
        data.mHash = JGadget::getHashCode(data.mName);
        mNumNameDatas++;
    }
}

void AudSoundNameConverter::addSectionInfo(const JAUSoundNameTable* pTable, u8 sectionID) {
    for (u8 i = 0; i < pTable->getNumGroups_inSection(sectionID); i++) {
        addGroupInfo(pTable, sectionID, i);
    }
}

u32 AudSoundNameConverter::getSoundNameTableNumItem_inSection(const JAUSoundNameTable* pTable, u8 sectionID) {
    u32 numItems = 0;
    for (u8 i = 0; i < pTable->getNumGroups_inSection(sectionID); i++) {
        switch (sectionID) {
        case 0:
            mGroupItemOffsets[i + 0] = numItems;
            break;
        case 1:
            mGroupItemOffsets[i + 14] = numItems + mNumItemsSection0;
            break;
        case 2:
            mGroupItemOffsets[i + 16] = numItems + mNumItemsSection0 + mNumItemsSection1;
            break;
        }

        numItems += pTable->getNumItems_inGroup(sectionID, i);
    }

    return numItems;
}
