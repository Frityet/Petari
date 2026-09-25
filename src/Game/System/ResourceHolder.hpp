#pragma once

#include "Game/Animation/BckCtrl.hpp"
#include "Game/System/ResourceInfo.hpp"
#include <JSystem/JKernel/JKRArchive.hpp>
#include <JSystem/JKernel/JKRFileFinder.hpp>
#include <JSystem/JKernel/JKRHeap.hpp>
#include <memory>
#include <filesystem>

namespace smgpc::resource {
    class RarcArchive;
}

struct ResTIMG;
class MaterialAnmBuffer;
class J3DModelData;

typedef const char* ArchiveName;

class ResourceHolder {
public:
    ResourceHolder(JKRArchive&);
    ~ResourceHolder();
    ResourceHolder(const ResourceHolder&) = delete;
    ResourceHolder& operator=(const ResourceHolder&) = delete;

    [[nodiscard]] std::shared_ptr<const void> retainNativeResources() const;
    [[nodiscard]] std::shared_ptr<const void> retainNativeTexture(const ResTIMG*) const;
    [[nodiscard]] std::size_t nativeArchiveReferenceCount() const noexcept;
    void ensureNativeResourcesUnborrowed() const;
    [[nodiscard]] const smgpc::resource::RarcArchive& nativeResourceSource() const;
    [[nodiscard]] const std::filesystem::path& nativeResourcePath() const;
    [[nodiscard]] JKRHeap& heap() const noexcept;

    const char* getMotionName(u32) const;
    bool isExistMaterialAnm() const;
    void newMaterialAnmBuffer(J3DModelData*);
    void newBckCtrl();

    Mtx44& getInitEffectMtx(int, int) const;
    void initializeArc(JKRArchive&);
    static JKRFileFinder* getFileFinder(JKRArchive*, const char*);
    u32 initEachResTable(ResTable**, JKRArchive*, const ArchiveName*);

    static s32 count(JKRArchive*, const char*, const char*);
    void mount(JKRArchive*, char*);

    ResFileInfo* createAndRegisterObject(const char*, void*);
    void backupInitMaterialData(const J3DModelData*);

    // bool checkCorrectMotionsAndModel(J3DModelData*, const char*) const;

    bool isCreatedAtSameHeap(ResourceHolder* pOther) const {
        return mHeap == pOther->mHeap;
    }

    // void makeStringAnmInfoAll(char*, u32) const;
    // bool verifyBckData(J3DModelData*, const char*) const;
    // bool verifyBvaData(J3DModelData*, const char*) const;
    // bool verifyBtpData(J3DModelData*, const char*) const;
    // bool verifyModelData(J3DModelData*, const char*) const;
    // bool verifyFileSize(u32, const char*) const;

    static int findBckIndex(const ResourceHolder* pResHolder, const char* pName) {
        return pResHolder->mMotionResTable->getResIndex(pName);
    }
    static int findBpkIndex(const ResourceHolder* pResHolder, const char* pName) {
        return pResHolder->mBpkResTable->getResIndex(pName);
    }
    static int findBtkIndex(const ResourceHolder* pResHolder, const char* pName) {
        return pResHolder->mBtkResTable->getResIndex(pName);
    }
    static int findBtpIndex(const ResourceHolder* pResHolder, const char* pName) {
        return pResHolder->mBtpResTable->getResIndex(pName);
    }
    static int findBrkIndex(const ResourceHolder* pResHolder, const char* pName) {
        return pResHolder->mBrkResTable->getResIndex(pName);
    }
    static int findBvaIndex(const ResourceHolder* pResHolder, const char* pName) {
        return pResHolder->mBvaResTable->getResIndex(pName);
    }

    const char* getModelName() const {
        return mModelResTable->getResName((u32)0);
    }

    ResTable* mModelResTable;         // 0x0
    ResTable* mMotionResTable;        // 0x4
    ResTable* mBtkResTable;           // 0x8
    ResTable* mBpkResTable;           // 0xC
    ResTable* mBtpResTable;           // 0x10
    ResTable* mBlkResTable;           // 0x14
    ResTable* mBrkResTable;           // 0x18
    ResTable* mBasResTable;           // 0x1C
    ResTable* mBmtResTable;           // 0x20
    ResTable* mBvaResTable;           // 0x24
    ResTable* mBanmtResTable;         // 0x28
    ResTable* mFileInfoTable;         // 0x2C
    ResTable mDefaultTable;           // 0x30
    MaterialAnmBuffer* mMaterialBuf;  // 0x38
    BckCtrl* mBckCtrl;                // 0x3C
    Mtx44* mBackupMaterialData;       // 0x40
    JKRArchive* mArchive;             // 0x44
    JKRHeap* mHeap;                   // 0x48
    u32 mTotalResourceSize;           // 0x4C

private:
    struct NativeResources;
    std::shared_ptr<NativeResources> mNativeResources;
    void destroyNativeResources() noexcept;
};
