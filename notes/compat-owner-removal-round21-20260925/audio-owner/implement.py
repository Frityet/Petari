from pathlib import Path
from edit import write

p='src/Game/System/AudSystemWrapper.hpp'
s=Path(p).read_text().replace('#include <revolution/types.h>','#include <revolution/types.h>\n#include <JSystem/JAudio2/JAUSoundTable.hpp>\n#include <memory>\n#include <vector>')
s=s.replace('class AudSystem;','class AudSystem;\nclass AudSceneMgr;\nclass AudBgmMgr;\nclass AudSoundObject;\nclass AudSoundObjHolder;\nclass AudSoundNameConverter;')
s=s.replace('''#if defined(TARGET_PC)
namespace smgpc { namespace compat { class DisabledAudioBackend; } }
#endif

''','')
s=s.replace('    static bool isOutputDisabled();','''    static AudSystemWrapper* getCurrent() noexcept;
    static bool isOutputDisabled();
    AudSceneMgr* getSceneMgr() const noexcept;
    AudBgmMgr* getBgmMgr() const noexcept;
    AudSoundObject* getSystemSeObject() const noexcept;
    AudSoundObjHolder* getSoundObjHolder() const noexcept;
    void setTriggerSePermitted(bool) noexcept;
    void setLevelSePermitted(bool) noexcept;
    bool isSePermitted() const noexcept;''')
s=s.replace('''    // Own the explicit native disabled-output backend; original AudSystem stays absent.
    smgpc::compat::DisabledAudioBackend* mDisabledBackend;''','''private:
    enum class InitializePhase { Created, Requested, Received, Initialized };
    void releaseResources() noexcept;

    // Bytes precede all original borrowers; destruction releases the converter
    // and its arrays before the native table and decoded byte storage.
    std::vector< u8 > mSoundNameBytes;
    JAUSoundNameTable mSoundNameTable{false};
    JAUSoundNameTable* mPreviousNameTable = nullptr;
    AudSoundNameConverter* mPreviousNameConverter = nullptr;
    std::unique_ptr< AudSoundNameConverter > mSoundNameConverter;
    std::unique_ptr< AudSceneMgr > mSceneMgr;
    std::unique_ptr< AudBgmMgr > mBgmMgr;
    std::unique_ptr< AudSoundObjHolder > mSoundObjHolder;
    std::unique_ptr< AudSoundObject > mSystemSeObject;
    InitializePhase mInitializePhase = InitializePhase::Created;
    bool mStaticWaveRequested = false;
    bool mStageWaveRequested = false;
    bool mScenarioWaveRequested = false;
    bool mResetRequested = false;
    bool mTriggerSePermitted = true;
    bool mLevelSePermitted = true;''')
write(p,s)

p='src/Game/System/AudSystemWrapper.cpp'
s='''#include "Game/System/AudSystemWrapper.hpp"
#include "Game/AudioLib/AudBgmMgr.hpp"
#include "Game/AudioLib/AudParams.hpp"
#include "Game/AudioLib/AudSceneMgr.hpp"
#include "Game/AudioLib/AudSoundNameConverter.hpp"
#include "Game/AudioLib/AudSoundObject.hpp"
#include "Game/AudioLib/AudSoundObjHolder.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "Game/Util/FileUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include <JSystem/JKernel/JKRHeap.hpp>
#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>
#include <aurora/j_audio_sound_archive.hpp>
#include <new>
#include <stdexcept>

namespace {
    void retireWrapper(void* object) noexcept {
        static_cast< AudSystemWrapper* >(object)->~AudSystemWrapper();
    }
}

AudSystemWrapper::AudSystemWrapper(JKRSolidHeap* audioHeap, JKRHeap* resourceHeap)
    : mAudSystem(nullptr), _4(audioHeap), _8(resourceHeap), mSmrRes(nullptr), mJaiSeqRes(nullptr), mJaiCordRes(nullptr),
      mJaiMeRes(nullptr), mJaiRemixSeqRes(nullptr), mSpkHeap(nullptr), mSpkRes(nullptr), _28(false), _29(false), _2A(false) {
    if (!JKRHeap::findFromRoot(this) || !audioHeap || !resourceHeap) {
        aurora::throw_host_exception< std::logic_error >("An audio wrapper requires its actual original process heaps");
    }
    JKRHeap::registerFinalizer(this, retireWrapper);
}

AudSystemWrapper::~AudSystemWrapper() {
    JKRHeap::unregisterFinalizer(this);
    mInitializePhase = InitializePhase::Created;
    releaseResources();
}

AudSystemWrapper* AudSystemWrapper::getCurrent() noexcept {
    auto* system = SingletonHolder< GameSystem >::get();
    return system && system->mObjHolder ? system->mObjHolder->mAudioSystem : nullptr;
}

bool AudSystemWrapper::isOutputDisabled() {
    auto* wrapper = getCurrent();
    return !wrapper || !wrapper->mAudSystem;
}

AudSceneMgr* AudSystemWrapper::getSceneMgr() const noexcept {
    return mInitializePhase == InitializePhase::Initialized ? mSceneMgr.get() : nullptr;
}

AudBgmMgr* AudSystemWrapper::getBgmMgr() const noexcept {
    return mInitializePhase == InitializePhase::Initialized ? mBgmMgr.get() : nullptr;
}

AudSoundObject* AudSystemWrapper::getSystemSeObject() const noexcept {
    return mInitializePhase == InitializePhase::Initialized ? mSystemSeObject.get() : nullptr;
}

AudSoundObjHolder* AudSystemWrapper::getSoundObjHolder() const noexcept {
    return mInitializePhase == InitializePhase::Initialized ? mSoundObjHolder.get() : nullptr;
}

void AudSystemWrapper::setTriggerSePermitted(bool permitted) noexcept {
    mTriggerSePermitted = permitted;
}

void AudSystemWrapper::setLevelSePermitted(bool permitted) noexcept {
    mLevelSePermitted = permitted;
}

bool AudSystemWrapper::isSePermitted() const noexcept {
    return mTriggerSePermitted && mLevelSePermitted;
}

void AudSystemWrapper::requestResourceForInitialize() {
    if (mInitializePhase != InitializePhase::Created) {
        aurora::throw_host_exception< std::logic_error >("Audio initialization was already requested");
    }
    // Name conversion remains required with output disabled. The absent DSP,
    // rhythm and speaker owners have no bank requests to enqueue.
    MR::loadAsyncToMainRAM("/AudioRes/SMR.szs", nullptr, _8, JKRDvdRipper::ALLOC_DIRECTION_BACKWARD);
    mInitializePhase = InitializePhase::Requested;
}

void AudSystemWrapper::receiveResourceForInitialize() {
    if (mInitializePhase == InitializePhase::Received) {
        return;
    }
    if (mInitializePhase != InitializePhase::Requested) {
        aurora::throw_host_exception< std::logic_error >("Audio name resources were not requested");
    }
    mSmrRes = MR::receiveFile("/AudioRes/SMR.szs");
    if (!mSmrRes) {
        aurora::throw_host_exception< std::runtime_error >("Audio initialization received no name resource");
    }
    mInitializePhase = InitializePhase::Received;
}

void AudSystemWrapper::createAudioSystem() {
    receiveResourceForInitialize();
    if (_29) {
        OSSuspendThread(OSGetCurrentThread());
    }
    _2A = true;
    try {
        const auto size = JKRHeap::getSize(mSmrRes, JKRHeap::findFromRoot(mSmrRes));
        if (size <= 0) {
            aurora::throw_host_exception< std::logic_error >("Audio name resource requires a bounded original heap allocation");
        }
        const aurora::allocation::HostAllocationScope host;
        aurora::audio::JAudioSoundArchive archive(
            {static_cast< const u8* >(mSmrRes), static_cast< std::size_t >(size)},
            [](std::string_view) -> std::vector< u8 > {
                aurora::throw_host_exception< std::logic_error >("Disabled audio output does not load wave banks");
            });
        mSoundNameBytes = archive.native_sound_name_table();
        if (mSoundNameBytes.size() < 16) {
            aurora::throw_host_exception< std::runtime_error >("Audio initialization received no sound-name table");
        }
        mSoundNameTable.init(mSoundNameBytes.data());
        AudSoundNameConverter::validateTable(&mSoundNameTable);
        createSoundNameConverter();

        auto* heap = JKRHeap::findFromRoot(this);
        const MR::CurrentHeapRestorer current(heap);
        const aurora::allocation::ClientAllocationScope game({true, true});
        mSceneMgr = std::make_unique< AudSceneMgr >(nullptr);
        mBgmMgr = std::make_unique< AudBgmMgr >();
        mSoundObjHolder = std::make_unique< AudSoundObjHolder >(heap, AudParams::numInspectableSoundObj);
        // The wrapper's finalizer owns this object. Host object storage keeps
        // JKRDisposer from destroying it first during bulk heap retirement;
        // its original constructor still allocates arrays in the selected heap.
        void* storage;
        {
            const aurora::allocation::HostAllocationScope objectStorage;
            storage = ::operator new(sizeof(AudSoundObject));
        }
        try {
            mSystemSeObject.reset(new (storage) AudSoundObject(nullptr, 10, heap));
        } catch (...) {
            ::operator delete(storage);
            throw;
        }
        mInitializePhase = InitializePhase::Initialized;
    } catch (...) {
        releaseResources();
        _2A = false;
        throw;
    }
    _2A = false;
    MR::removeFileConsideringLanguage("/AudioRes/SMR.szs");
    mSmrRes = nullptr;
}

void AudSystemWrapper::createSoundNameConverter() {
    if (mSoundNameConverter) {
        return;
    }
    if (mInitializePhase != InitializePhase::Received || mSoundNameBytes.empty()) {
        aurora::throw_host_exception< std::logic_error >("Audio name publication requires its received sound-name table");
    }
    AudSoundNameConverter::validateTable(&mSoundNameTable);
    const MR::CurrentHeapRestorer current(JKRHeap::findFromRoot(this));
    const aurora::allocation::ClientAllocationScope game({true, true});
    mPreviousNameTable = JAUSoundNameTable::sInstance;
    JAUSoundNameTable::sInstance = &mSoundNameTable;
    try {
        mSoundNameConverter = std::make_unique< AudSoundNameConverter >();
    } catch (...) {
        JAUSoundNameTable::sInstance = mPreviousNameTable;
        throw;
    }
    mPreviousNameConverter = AudSingletonHolder< AudSoundNameConverter >::exchange(mSoundNameConverter.get());
}

void AudSystemWrapper::releaseResources() noexcept {
    const aurora::allocation::HostAllocationScope host;
    mSystemSeObject.reset();
    mSoundObjHolder.reset();
    mBgmMgr.reset();
    mSceneMgr.reset();
    if (mSoundNameConverter && AudSingletonHolder< AudSoundNameConverter >::get() == mSoundNameConverter.get()) {
        AudSingletonHolder< AudSoundNameConverter >::exchange(mPreviousNameConverter);
    }
    mSoundNameConverter.reset();
    if (JAUSoundNameTable::sInstance == &mSoundNameTable) {
        JAUSoundNameTable::sInstance = mPreviousNameTable;
    }
    mPreviousNameConverter = nullptr;
    mPreviousNameTable = nullptr;
    mSoundNameTable.init(nullptr);
    mSoundNameBytes.clear();
}

void AudSystemWrapper::updateRhythm() {
    // No rhythm owner exists while output is disabled.
}

void AudSystemWrapper::movement() {
    if (mInitializePhase != InitializePhase::Initialized) {
        return;
    }
    mBgmMgr->movement();
    mSoundObjHolder->update();
}

void AudSystemWrapper::stopAllSound(u32) {
    // Disabled starts create no voices requiring a stop acknowledgement.
}

bool AudSystemWrapper::isLoadDoneWaveDataAtSystemInit() const {
    return mInitializePhase == InitializePhase::Initialized && mSceneMgr->isLoadDoneSystemInit();
}

void AudSystemWrapper::loadStaticWaveData() {
    if (mInitializePhase == InitializePhase::Initialized) {
        mSceneMgr->loadStaticResource();
        mStaticWaveRequested = true;
    }
}

bool AudSystemWrapper::isLoadDoneStaticWaveData() const {
    return mInitializePhase == InitializePhase::Initialized && mStaticWaveRequested && mSceneMgr->isLoadDoneStaticResource();
}

void AudSystemWrapper::loadStageWaveData(const char* sceneName, const char* stageName, bool isPlayerLuigi) {
    if (mInitializePhase != InitializePhase::Initialized) {
        return;
    }
    if (isPlayerLuigi) {
        mSceneMgr->setPlayerModeLuigi();
    } else {
        mSceneMgr->setPlayerModeMario();
    }
    mScenarioWaveRequested = false;
    mSceneMgr->loadStageResource(sceneName, stageName);
    mStageWaveRequested = true;
}

bool AudSystemWrapper::isLoadDoneStageWaveData() const {
    return mInitializePhase == InitializePhase::Initialized && mStageWaveRequested && mSceneMgr->isLoadDoneStageResource();
}

void AudSystemWrapper::loadScenarioWaveData(const char* sceneName, const char* stageName, s32 scenarioNo) {
    if (mInitializePhase == InitializePhase::Initialized) {
        mSceneMgr->loadScenarioResource(sceneName, stageName, scenarioNo);
        mScenarioWaveRequested = true;
    }
}

bool AudSystemWrapper::isLoadDoneScenarioWaveData() const {
    return mInitializePhase == InitializePhase::Initialized && mScenarioWaveRequested && mSceneMgr->isLoadDoneScenarioResource();
}

bool AudSystemWrapper::isPermitToReset() const {
    return !_2A;
}

void AudSystemWrapper::prepareReset() {
    if (mInitializePhase != InitializePhase::Initialized) {
        _29 = true;
    } else {
        mResetRequested = true;
    }
}

void AudSystemWrapper::requestReset(bool) {
    prepareReset();
    stopAllSound(10);
}

bool AudSystemWrapper::isResetDone() {
    return _29 || mResetRequested || mInitializePhase != InitializePhase::Initialized;
}

void AudSystemWrapper::resumeReset() {
    _29 = false;
    mResetRequested = false;
}
'''
write(p,s)

p='src/Game/AudioLib/AudSoundNameConverter.hpp'
s=Path(p).read_text().replace('    AudSoundNameConverter();','    AudSoundNameConverter();\n    ~AudSoundNameConverter();\n    static void validateTable(const JAUSoundNameTable*);')
write(p,s)
p='src/Game/AudioLib/AudSoundNameConverter.cpp'
s=Path(p).read_text().replace('#include <cstring>', '#include <aurora/exception.hpp>\n#include <array>\n#include <cstring>\n#include <limits>\n#include <stdexcept>')
s=s.replace('''    init();
}

JAISoundID AudSoundNameConverter::getSoundID''','''    try {
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

JAISoundID AudSoundNameConverter::getSoundID''',1)
s=s.replace('''    return getSoundID(pName, JGadget::getHashCode(pName));''','''    return pName ? getSoundID(pName, JGadget::getHashCode(pName)) : JAISoundID(-1);''',1)
s=s.replace('''    bool isSE;
    if (pName[0] == 'S' && pName[1] == 'E')''','''    if (!pName || std::strlen(pName) < 2) {
        return -1;
    }
    bool isSE;
    if (pName[0] == 'S' && pName[1] == 'E')''',1)
s=s.replace('''    s32 startingOffset;
    if (isSE) {
        startingOffset = mGroupItemOffsets[getSeSoundCategory(pName)];
    } else {
        startingOffset = mGroupItemOffsets[getOtherSoundCategory(pName)];
    }
''','''    if (isSE && std::strlen(pName) < 5) {
        return -1;
    }
    const s32 category = isSE ? getSeSoundCategory(pName) : getOtherSoundCategory(pName);
    if (category < 0 || category >= 17) {
        return -1;
    }
    const s32 startingOffset = mGroupItemOffsets[category];
''')
s=s.replace('''    const JAUSoundNameTable* table = JAUSoundNameTable::getInstance();
    initDataTable(table);''','''    const JAUSoundNameTable* table = JAUSoundNameTable::getInstance();
    validateTable(table);
    initDataTable(table);''')
write(p,s)

p='src/Game/AudioLib/AudSoundObjHolder.hpp'
s=Path(p).read_text().replace('    AudSoundObjHolder(JKRHeap* pHeap, s32 capacity);', '    AudSoundObjHolder(JKRHeap* pHeap, s32 capacity);\n    ~AudSoundObjHolder();')
write(p,s)
p='src/Game/AudioLib/AudSoundObjHolder.cpp'
s=Path(p).read_text().replace('#include <JSystem/JKernel/JKRHeap.hpp>', '#include <JSystem/JKernel/JKRHeap.hpp>\n#include <aurora/exception.hpp>\n#include <stdexcept>')
s=s.replace('''    mCapacity = capacity;
    mSize = 0;
    mArray = new (pHeap, 0) AudSoundObject*[capacity];
}''','''    if (capacity < 0) {
        aurora::throw_host_exception< std::invalid_argument >("Sound-object holder capacity must be nonnegative");
    }
    mCapacity = capacity;
    mSize = 0;
    mArray = new (pHeap, 0) AudSoundObject*[capacity]();
}

AudSoundObjHolder::~AudSoundObjHolder() {
    for (s32 i = 0; i < mSize; i++) {
        if (mArray[i]->mNativeHolder == this) {
            mArray[i]->mNativeHolder = nullptr;
        }
    }
    delete[] mArray;
}''')
s=s.replace('''    if (mSize < mCapacity) {
        mArray[mSize] = pSound;
        mSize++;
    }''','''    if (!pSound || pSound->mNativeHolder == this || mSize >= mCapacity) {
        return;
    }
    if (pSound->mNativeHolder) {
        pSound->mNativeHolder->remove(pSound);
    }
    mArray[mSize] = pSound;
    mSize++;
    pSound->mNativeHolder = this;''')
s=s.replace('''        mSize--;
    }
}''','''        mSize--;
        mArray[mSize] = nullptr;
    }
    if (pSound && pSound->mNativeHolder == this) {
        pSound->mNativeHolder = nullptr;
    }
}''',1)
write(p,s)

p='src/Game/AudioLib/AudSceneMgr.cpp'
s=Path(p).read_text().replace('#include "Game/Speaker/SpkSystem.hpp"', '#include "Game/Speaker/SpkSystem.hpp"\n#include "Game/System/AudSystemWrapper.hpp"').replace('#include <JSystem/JAudio2/JAUSectionHeap.hpp>', '#include <JSystem/JAudio2/JAUSectionHeap.hpp>\n#include <cstring>')
voids=['loadStaticResource()', 'eraseLastBgmWaveSet()', 'eraseLastSeWaveSet()', 'eraseLastSeScenarioWaveSet()', 'loadWaveSet(const s8* pWaveSet, s32 numItems)']
for signature in voids:
    key='void AudSceneMgr::'+signature+' {'
    s=s.replace(key,key+'\n    if (!mSectionHeap && AudSystemWrapper::isOutputDisabled()) {\n        return;\n    }\n')
for signature in ['isLoadDoneSystemInit()', 'isLoadDoneStaticResource()', 'isLoadDoneStageResource()', 'isLoadDoneScenarioResource()', 'loadPlayerResource()', 'isPlayerResourceLoaded()']:
    key='bool AudSceneMgr::'+signature+' {'
    s=s.replace(key,key+'\n    if (!mSectionHeap && AudSystemWrapper::isOutputDisabled()) {\n        return true;\n    }\n')
s=s.replace('''            mSectionHeap->eraseWaveArc(34, 2);
            mSectionHeap->eraseWaveArc(34, 4);''','''            if (mSectionHeap) {
                mSectionHeap->eraseWaveArc(34, 2);
                mSectionHeap->eraseWaveArc(34, 4);
            }''')
s=s.replace('''void AudSceneMgr::startScene() {
    _4 = 0;''','''void AudSceneMgr::startScene() {
    _4 = 0;
    if (auto* wrapper = AudSystemWrapper::getCurrent()) {
        wrapper->setTriggerSePermitted(true);
        wrapper->setLevelSePermitted(true);
    }
    if (AudSystemWrapper::isOutputDisabled()) {
        _1D = false;
        return;
    }''')
write(p,s)
for stem in ['DisabledAudioBackend','DisabledObjectAudioService','JAudioCategoryVolumeOwnership','JAudioLimitedSoundOwnership']:
    for ext in ['cpp','hpp']:
        write(f'src/compat/{stem}.{ext}',None)
