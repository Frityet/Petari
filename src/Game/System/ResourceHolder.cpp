#include "Game/System/ResourceHolder.hpp"
#include "Game/Animation/MaterialAnmBuffer.hpp"
#include "Game/System/FileLoader.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "Game/Util/MutexHolder.hpp"
#include "Game/Util/StringUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "JSystem/J3DGraphAnimator/J3DMaterialAnm.hpp"
#include "camera/CameraAnimation.hpp"
#include "compat/J3dCommandScope.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "resource/BasResource.hpp"
#include "resource/BtiTextureData.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "resource/J3dAnimationResource.hpp"
#include "resource/J3dModelResource.hpp"
#include "resource/RarcArchive.hpp"
#include <aurora/exception.hpp>
#include <JSystem/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/J3DGraphLoader/J3DAnmLoader.hpp>
#include <JSystem/J3DGraphLoader/J3DModelLoader.hpp>
#include <cstdio>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <map>
#include <revolution.h>

namespace {
    // A native loader may throw across the original manual lock/unlock pair.
    // Recover only this thread's acquisitions made inside the current load.
    class LoadMutexRecovery final {
    public:
        LoadMutexRecovery() : mThread(OSGetCurrentThread()), mExceptions(std::uncaught_exceptions()) {
            const auto enabled = OSDisableInterrupts();
            mCount = mutex().thread == mThread ? mutex().count : 0;
            OSRestoreInterrupts(enabled);
        }
        ~LoadMutexRecovery() {
            if (std::uncaught_exceptions() <= mExceptions)
                return;
            const auto enabled = OSDisableInterrupts();
            while (mutex().thread == mThread && mutex().count > mCount)
                OSUnlockMutex(&mutex());
            OSRestoreInterrupts(enabled);
        }
    private:
        static OSMutex& mutex() { return MR::MutexHolder<0>::sMutex; }
        OSThread* mThread;
        int mExceptions;
        s32 mCount;
    };

    enum class BackingKind { Raw, Animation, Model, Map, Bas, Texture, CameraAnimation };

    BackingKind backingKind(std::string_view name) {
        // Match createAndRegisterObject's ordered, case-sensitive predicates.
        for (const auto ext : {".btp", ".bpk", ".btk", ".brk", ".blk", ".bck", ".bca"})
            if (name.find(ext) != name.npos)
                return BackingKind::Animation;
        if (name.find(".bas") != name.npos)
            return BackingKind::Bas;
        if (name.find(".bmt") != name.npos)
            return BackingKind::Model;
        if (name.find(".bva") != name.npos)
            return BackingKind::Animation;
        if (name.find(".banmt") != name.npos)
            return BackingKind::Map;
        if (name.find(".bdl") != name.npos || name.find(".bmd") != name.npos)
            return BackingKind::Model;
        if (name.ends_with(".bti"))
            return BackingKind::Texture;
        if (name.ends_with(".canm"))
            return BackingKind::CameraAnimation;
        return BackingKind::Raw;
    }

    static const ArchiveName sModelExt[] = {".bdl", ".bmd", nullptr};
    static const ArchiveName sMotionExt[] = {".bck", ".bca", nullptr};
    static const ArchiveName sBtkExt[] = {".btk", nullptr};
    static const ArchiveName sBpkExt[] = {".bpk", nullptr};
    static const ArchiveName sBtpExt[] = {".btp", nullptr};
    static const ArchiveName sBlkExt[] = {".blk", nullptr};
    static const ArchiveName sBrkExt[] = {".brk", nullptr};
    static const ArchiveName sBasExt[] = {".bas", nullptr};
    static const ArchiveName sBmtExt[] = {".bmt", nullptr};
    static const ArchiveName sBvaExt[] = {".bva", nullptr};
    static const ArchiveName sBanmtExt[] = {".banmt", nullptr};

    // static const bool cIgnoreCheckPosFormatList = ;
    // static const bool cIgnoreCheckNrmCompressList = ;
    // static const bool cIgnoreCheckPlanetKLowSizeList = ;
    // static const bool cIgnoreCheckAll = ;

    // static const __ sPlanetLowSizeBorder = ;
};  // namespace

struct ResourceHolder::NativeResources {
    std::shared_ptr<smgpc::compat::JkrAllocationDomain> domain;
    std::shared_ptr<const void> archiveLifetime;
    std::shared_ptr<const smgpc::resource::RarcArchive> source;
    std::filesystem::path path;
    JKRArchive* archive = nullptr;
    std::vector<std::shared_ptr<const void>> convertedEntries;
    std::map<const void*, const void*> loaderSources;
    std::vector<std::shared_ptr<const std::vector<std::uint8_t>>> soundSources;
    std::vector<smgpc::resource::BasResource> soundAnimations;
    std::vector<std::shared_ptr<smgpc::resource::BtiTextureData>> textures;
    std::vector<smgpc::camera::NativeCameraAnimationData> cameraAnimations;
    std::vector<smgpc::resource::J3dAnimationResource> animations;
    std::vector<smgpc::resource::J3dModelResource> models;

    ~NativeResources() { releaseModels(); }

    void releaseModels() noexcept {
        const aurora::allocation::HostAllocationScope host;
        // The original manager may create several holders for one archive.
        // SDK override leases restore the next live cached record in any order.
        convertedEntries.clear();
        loaderSources.clear();
        models.clear();
    }

    void publishConvertedEntry(u32 index, void* data) {
        convertedEntries.push_back(archive->overrideNativeResource(index, data));
    }

    void prepare(JKRArchive& owner, JKRHeap& heap) {
        domain = smgpc::compat::JkrAllocationDomain::retain_heap(heap);
        archiveLifetime = owner.retainNativeResources();
        source = owner.retainSource();
        archive = &owner;
        path = owner.mLoaderName ? owner.mLoaderName : "";
        if (const auto* loader = SingletonHolder<FileLoader>::get())
            if (const auto* entry = loader->mArchiveHolder->findEntry(&owner))
                path = entry->mArchiveName;
        auto* runtime = smgpc::resource::GameResourceRuntime::active();
        if (!runtime)
            aurora::throw_host_exception<std::logic_error>("ResourceHolder requires the original process resource owner");
        const auto& mem1 = runtime->mem1_heap();
        for (const auto& entry : source->entries()) {
            const auto bytes = source->file_data(entry);
            switch (backingKind(entry.name)) {
            case BackingKind::Animation:
                // Preserve the original loader's null dispatch for empty files.
                if (bytes.empty())
                    break;
                animations.emplace_back(bytes);
                loaderSources.emplace(bytes.data(), animations.back().data());
                break;
            case BackingKind::Model:
                models.emplace_back(bytes, domain, mem1);
                loaderSources.emplace(bytes.data(), models.back().data());
                break;
            case BackingKind::Map:
                // The actual ArchiveHolder entry owns bounded JMap registrations.
                break;
            case BackingKind::Bas:
                if (!bytes.empty()) {
                    auto copy = std::make_shared<const std::vector<std::uint8_t>>(bytes.begin(), bytes.end());
                    soundAnimations.emplace_back(*copy, copy);
                    soundSources.push_back(std::move(copy));
                    loaderSources.emplace(bytes.data(), soundAnimations.back().animation());
                }
                break;
            case BackingKind::Texture:
                if (bytes.empty())
                    break;
                textures.push_back(std::make_shared<smgpc::resource::BtiTextureData>(bytes, mem1));
                publishConvertedEntry(entry.file_entry_index, const_cast<ResTIMG*>(textures.back()->image()));
                break;
            case BackingKind::CameraAnimation:
                if (bytes.empty())
                    break;
                cameraAnimations.push_back(smgpc::camera::CameraAnimation::from_bytes(bytes).native_data());
                publishConvertedEntry(entry.file_entry_index, const_cast<std::uint8_t*>(cameraAnimations.back().bytes().data()));
                break;
            case BackingKind::Raw:
                break;
            }
        }
    }
};

ResourceHolder::ResourceHolder(JKRArchive& rArchive)
    : mModelResTable(&mDefaultTable), mMotionResTable(&mDefaultTable), mBtkResTable(&mDefaultTable), mBpkResTable(&mDefaultTable),
      mBtpResTable(&mDefaultTable), mBlkResTable(&mDefaultTable), mBrkResTable(&mDefaultTable), mBasResTable(&mDefaultTable),
      mBmtResTable(&mDefaultTable), mBvaResTable(&mDefaultTable), mBanmtResTable(&mDefaultTable), mFileInfoTable(&mDefaultTable), mArchive(&rArchive),
      mMaterialBuf(nullptr), mBckCtrl(nullptr), mBackupMaterialData(nullptr), mTotalResourceSize(0) {
    mHeap = MR::getCurrentHeap();
    try {
        {
            const aurora::allocation::HostAllocationScope host;
            mNativeResources = std::make_shared<NativeResources>();
            mNativeResources->prepare(rArchive, *mHeap);
        }
        const smgpc::compat::JkrAllocationScope original(mNativeResources->domain);
        const smgpc::compat::J3dCommandScope commands;
        const LoadMutexRecovery recovery;
        initializeArc(*mArchive);

        if (mModelResTable->mCount != 0 && isExistMaterialAnm()) {
            newMaterialAnmBuffer(reinterpret_cast< J3DModelData* >(mModelResTable->getRes(getModelName())));
        }

        if (mMotionResTable->mCount != 0) {
            newBckCtrl();
        }

        if (mModelResTable->mCount != 0) {
            backupInitMaterialData(reinterpret_cast< J3DModelData* >(mModelResTable->getRes(getModelName())));
        }
    } catch (...) {
        destroyNativeResources();
        throw;
    }
}

ResourceHolder::~ResourceHolder() {
    destroyNativeResources();
}

void ResourceHolder::destroyNativeResources() noexcept {
    const aurora::allocation::HostAllocationScope host;
    // Loaded model materials still refer to MaterialAnmBuffer. Retire models
    // before their borrowed animation storage, while archive and heap live.
    if (mNativeResources)
        mNativeResources->releaseModels();
    if (mMaterialBuf) {
        delete[] mMaterialBuf->_0;
        delete[] mMaterialBuf->_4;
        delete mMaterialBuf;
        mMaterialBuf = nullptr;
    }
    if (mBckCtrl) {
        delete[] mBckCtrl->mControlData;
        delete mBckCtrl;
        mBckCtrl = nullptr;
    }
    delete[] mBackupMaterialData;
    mBackupMaterialData = nullptr;
    for (auto** table : {&mModelResTable, &mMotionResTable, &mBtkResTable, &mBpkResTable, &mBtpResTable, &mBlkResTable,
                        &mBrkResTable, &mBasResTable, &mBmtResTable, &mBvaResTable, &mBanmtResTable, &mFileInfoTable}) {
        if (*table == &mDefaultTable)
            continue;
        for (u32 i = 0; i < (*table)->mCount; ++i)
            delete[] (*table)->mFileInfoTable[i].mName;
        delete[] (*table)->mFileInfoTable;
        delete *table;
        *table = &mDefaultTable;
    }
    mNativeResources.reset();
}

std::shared_ptr<const void> ResourceHolder::retainNativeResources() const {
    return mNativeResources;
}

std::shared_ptr<const void> ResourceHolder::retainNativeTexture(const ResTIMG* image) const {
    for (const auto& texture : mNativeResources->textures)
        if (texture->image() == image)
            return texture;
    aurora::throw_host_exception<std::invalid_argument>("Texture does not belong to this original ResourceHolder");
}

std::size_t ResourceHolder::nativeArchiveReferenceCount() const noexcept {
    return 1 + mNativeResources->convertedEntries.size();
}

void ResourceHolder::ensureNativeResourcesUnborrowed() const {
    if (mNativeResources.use_count() != 1)
        aurora::throw_host_exception<std::logic_error>("Cannot unload an original resource heap with live model owners");
}

const smgpc::resource::RarcArchive& ResourceHolder::nativeResourceSource() const {
    return *mNativeResources->source;
}

const std::filesystem::path& ResourceHolder::nativeResourcePath() const {
    return mNativeResources->path;
}

JKRHeap& ResourceHolder::heap() const noexcept {
    return *mHeap;
}

const char* ResourceHolder::getMotionName(u32 index) const {
    if (mMotionResTable->mCount > index) {
        return mMotionResTable->getResName(index);
    }
    return nullptr;
}

bool ResourceHolder::isExistMaterialAnm() const {
    return mBpkResTable->mCount != 0 || mBtpResTable->mCount != 0 || mBtkResTable->mCount != 0 || mBrkResTable->mCount != 0;
}

void ResourceHolder::newMaterialAnmBuffer(J3DModelData* pModelData) {
    MR::CurrentHeapRestorer heap(mHeap);
    mMaterialBuf = new MaterialAnmBuffer(this, pModelData, true);
}

void ResourceHolder::newBckCtrl() {
    MR::CurrentHeapRestorer heap(mHeap);
    mBckCtrl = new BckCtrl(this, mArchive->mLoaderName);
}

Mtx44& ResourceHolder::getInitEffectMtx(int materialIndex, int mtxIndex) const {
    return mBackupMaterialData[materialIndex * 8 + mtxIndex];
}

void ResourceHolder::initializeArc(JKRArchive& rArchive) {
    u32 fileInfoSize = rArchive.countResource();
    fileInfoSize -= initEachResTable(&mModelResTable, &rArchive, ::sModelExt);
    fileInfoSize -= initEachResTable(&mMotionResTable, &rArchive, ::sMotionExt);
    fileInfoSize -= initEachResTable(&mBlkResTable, &rArchive, ::sBlkExt);
    fileInfoSize -= initEachResTable(&mBtkResTable, &rArchive, ::sBtkExt);
    fileInfoSize -= initEachResTable(&mBpkResTable, &rArchive, ::sBpkExt);
    fileInfoSize -= initEachResTable(&mBtpResTable, &rArchive, ::sBtpExt);
    fileInfoSize -= initEachResTable(&mBrkResTable, &rArchive, ::sBrkExt);
    fileInfoSize -= initEachResTable(&mBasResTable, &rArchive, ::sBasExt);
    fileInfoSize -= initEachResTable(&mBmtResTable, &rArchive, ::sBmtExt);
    fileInfoSize -= initEachResTable(&mBvaResTable, &rArchive, ::sBvaExt);
    fileInfoSize -= initEachResTable(&mBanmtResTable, &rArchive, ::sBanmtExt);

    if (fileInfoSize > 0) {
        mFileInfoTable = new (mHeap, 0) ResTable();
        mFileInfoTable->newFileInfoTable(fileInfoSize);
    }

    mount(&rArchive, nullptr);
}

JKRFileFinder* ResourceHolder::getFileFinder(JKRArchive* pArchive, const char* pName) {
    if (pName == nullptr) {
        return pArchive->getFirstFile("/");
    }

    return pArchive->getFirstFile(pName);
}

u32 ResourceHolder::initEachResTable(ResTable** pResTable, JKRArchive* pArchive, const ArchiveName* pExtNames) {
    u32 numResources = 0;
    for (s32 idx = 0; pExtNames[idx] != nullptr; idx++) {
        numResources += count(pArchive, pExtNames[idx], nullptr);
    }

    if (numResources != 0) {
        *pResTable = new (mHeap, 0) ResTable();
        (*pResTable)->newFileInfoTable(numResources);
    }

    return numResources;
}

s32 ResourceHolder::count(JKRArchive* pArchive, const char* pExtName, const char* pPath) {
    s32 num = 0;
    std::unique_ptr<JKRFileFinder> finder(getFileFinder(pArchive, pPath));

    while (finder->mHasMoreFiles) {
        if (finder->mFileIsFolder) {
            if (finder->mName[0] != '.') {
                char path[128];
                sprintf(path, "%s%s%s", pPath, "/", finder->mName);
                num += count(pArchive, pExtName, path);
            }
        } else {
            if (pExtName == nullptr || strstr(finder->mName, pExtName) != nullptr) {
                num++;
            }
        }
        finder->findNextFile();
    }

    return num;
}

void ResourceHolder::mount(JKRArchive* pArchive, char* pPath) {
    std::unique_ptr<JKRFileFinder> finder(getFileFinder(pArchive, pPath));

    while (finder->mHasMoreFiles) {
        if (finder->mFileIsFolder) {
            if (finder->mName[0] != '.') {
                char path[128];
                snprintf(path, 128, "%s/%s", pPath, finder->mName);
                mount(pArchive, path);
            }
        } else {
            u32 fileID = pArchive->getFileAttribute(finder->mFileID);
            ResFileInfo* info = createAndRegisterObject(finder->mName, pArchive->getResource(finder->mFileID));
            info->_8 = pArchive->getResource(finder->mFileID);
            info->_4 = pArchive->getResSize(info->_8);
            info->_C = finder->mFileID;
            mTotalResourceSize += info->_4;
        }
        finder->findNextFile();
    }

}

ResFileInfo* ResourceHolder::createAndRegisterObject(const char* pName, void* pData) {
    // Disk resources keep their archive identities. Native SDK loaders use
    // this holder's independent typed backing, including duplicate mounts.
    if (const auto source = mNativeResources->loaderSources.find(pData); source != mNativeResources->loaderSources.end())
        pData = const_cast<void*>(source->second);
    if (strstr(pName, ".btp") != nullptr) {
        return mBtpResTable->add(pName, J3DAnmLoaderDataBase::load(pData), true);
    }

    if (strstr(pName, ".bpk") != nullptr) {
        return mBpkResTable->add(pName, J3DAnmLoaderDataBase::load(pData), true);
    }

    if (strstr(pName, ".btk") != nullptr) {
        return mBtkResTable->add(pName, J3DAnmLoaderDataBase::load(pData), true);
    }

    if (strstr(pName, ".brk") != nullptr) {
        return mBrkResTable->add(pName, J3DAnmLoaderDataBase::load(pData), true);
    }

    if (strstr(pName, ".blk") != nullptr) {
        return mBlkResTable->add(pName, J3DAnmLoaderDataBase::load(pData), true);
    }

    if (strstr(pName, ".bck") != nullptr) {
        return mMotionResTable->add(pName, pData == nullptr ? nullptr : J3DAnmLoaderDataBase::load(pData), true);
    }

    if (strstr(pName, ".bca") != nullptr) {
        return mMotionResTable->add(pName, J3DAnmLoaderDataBase::load(pData), true);
    }

    if (strstr(pName, ".bas") != nullptr) {
        return mBasResTable->add(pName, pData, true);
    }

    if (strstr(pName, ".bmt") != nullptr) {
        OSLockMutex(&MR::MutexHolder< 0 >::sMutex);
        ResFileInfo* ret = mBmtResTable->add(pName, J3DModelLoaderDataBase::loadMaterialTable(pData), true);
        OSUnlockMutex(&MR::MutexHolder< 0 >::sMutex);
        return ret;
    }

    if (strstr(pName, ".bva") != nullptr) {
        return mBvaResTable->add(pName, J3DAnmLoaderDataBase::load(pData), true);
    }

    if (strstr(pName, ".banmt") != nullptr) {
        return mBanmtResTable->add(pName, pData, true);
    }

    if (strstr(pName, ".bdl") != nullptr) {
        OSLockMutex(&MR::MutexHolder< 0 >::sMutex);
        J3DModelData* modelData = J3DModelLoaderDataBase::loadBinaryDisplayList(pData, J3DMLF_Material_UseIndirect | J3DMLF_UseUniqueMaterials);
        OSUnlockMutex(&MR::MutexHolder< 0 >::sMutex);
        MR::initEnvelopeAndEnvMapOrProjMapModelData(modelData);
        if (MR::isUseFur(modelData) || MR::strcasecmp(pName, "MarioShadow.bdl") == 0) {
            MR::downFracVtx(modelData);
        }
        return mModelResTable->add(pName, modelData, true);
    }

    if (strstr(pName, ".bmd") != nullptr) {
        J3DModelData* modelData =
            J3DModelLoaderDataBase::load(pData, J3DMLF_Material_PE_Full | J3DMLF_Material_UseIndirect | J3DMLF_UseUniqueMaterials | J3DMLF_21);
        modelData->newSharedDisplayList(0);
        return mModelResTable->add(pName, modelData, true);
    }

    return mFileInfoTable->add(pName, pData, false);
}

void ResourceHolder::backupInitMaterialData(const J3DModelData* pModelData) {
    u16 numMaterials = pModelData->getMaterialNum();
    mBackupMaterialData = new Mtx44[numMaterials * 8];

    for (u16 matIdx = 0; matIdx < numMaterials; matIdx++) {
        J3DMaterial* material = pModelData->getMaterialNodePointer(matIdx);
        for (u32 mtxIdx = 0; mtxIdx < 8; mtxIdx++) {
            J3DTexMtx* texMtx = material->getTexGenBlock()->getTexMtx(mtxIdx);
            if (texMtx != nullptr) {
                PSMTX44Copy(texMtx->getTexMtxInfo().mEffectMtx, mBackupMaterialData[matIdx * 8 + mtxIdx]);
            } else {
                PSMTX44Identity(mBackupMaterialData[matIdx * 8 + mtxIdx]);
            }
        }
    }
}
