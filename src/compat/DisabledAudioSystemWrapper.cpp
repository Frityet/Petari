#include "Game/System/AudSystemWrapper.hpp"
#include "Game/Util/FileUtil.hpp"
#include "compat/DisabledAudioBackend.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/JkrHeapFinalizer.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include <aurora/exception.hpp>
#include <stdexcept>

namespace {
using Backend = smgpc::compat::DisabledAudioBackend;
Backend& backend(const AudSystemWrapper& wrapper) {
    if (!wrapper.mDisabledBackend)
        aurora::throw_host_exception<std::logic_error>("The original audio wrapper has no disabled backend owner");
    return *wrapper.mDisabledBackend;
}
void retire_wrapper(void* object) noexcept { static_cast<AudSystemWrapper*>(object)->~AudSystemWrapper(); }
}

// Complete subsystem adapter for the explicitly disabled output policy. The
// original process still constructs this wrapper and executes its async worker.
// AudSystem, rhythm/speaker graphs and GameSystem globals are never fabricated.
AudSystemWrapper::AudSystemWrapper(JKRSolidHeap* audio_heap, JKRHeap* resource_heap)
    : mAudSystem(nullptr), _4(audio_heap), _8(resource_heap), mSmrRes(nullptr),
      mJaiSeqRes(nullptr), mJaiCordRes(nullptr), mJaiMeRes(nullptr), mJaiRemixSeqRes(nullptr),
      mSpkHeap(nullptr), mSpkRes(nullptr), _28(false), _29(false), _2A(false), mDisabledBackend(nullptr) {
    auto* owner_heap = JKRHeap::findFromRoot(this);
    if (!owner_heap || !audio_heap || !resource_heap)
        aurora::throw_host_exception<std::logic_error>("An audio wrapper requires its actual original process heaps");
    smgpc::compat::JkrHostAllocationScope host;
    auto instance = std::make_unique<Backend>(*owner_heap);
    smgpc::compat::register_jkr_heap_finalizer(this, retire_wrapper);
    mDisabledBackend = instance.release();
}
AudSystemWrapper::~AudSystemWrapper() {
    smgpc::compat::unregister_jkr_heap_finalizer(this);
    delete mDisabledBackend;
    mDisabledBackend = nullptr;
}
void AudSystemWrapper::requestResourceForInitialize() {
    // Names remain required for ordinary Game requests; disabled output has no
    // sequence/chord/ME/remix/speaker bank consumers and queues none of them.
    MR::loadAsyncToMainRAM("/AudioRes/SMR.szs", nullptr, _8, JKRDvdRipper::ALLOC_DIRECTION_BACKWARD);
    backend(*this).request_initialize();
}
void AudSystemWrapper::receiveResourceForInitialize() {
    auto& output = backend(*this);
    if (output.phase() == Backend::Phase::Received) return;
    if (output.phase() != Backend::Phase::Requested)
        aurora::throw_host_exception<std::logic_error>("Audio name resources were not requested");
    mSmrRes = MR::receiveFile("/AudioRes/SMR.szs");
    if (!mSmrRes)
        aurora::throw_host_exception<std::runtime_error>("Audio initialization received no name resource");
    output.receive_initialize();
}
void AudSystemWrapper::createAudioSystem() {
    receiveResourceForInitialize();
    if (_29) OSSuspendThread(OSGetCurrentThread());
    _2A = true;
    try {
        // FileRipper already decompressed the requested allocation. Its actual
        // heap block extent bounds the archive; the compressed DVD size does not.
        const auto size = JKRHeap::getSize(mSmrRes, JKRHeap::findFromRoot(mSmrRes));
        if (size <= 0)
            aurora::throw_host_exception<std::logic_error>("Audio name resource requires a bounded original heap allocation");
        backend(*this).initialize({static_cast<const std::uint8_t*>(mSmrRes), static_cast<std::size_t>(size)});
    } catch (...) {
        _2A = false;
        throw;
    }
    _2A = false;
    MR::removeFileConsideringLanguage("/AudioRes/SMR.szs");
    mSmrRes = nullptr;
}
void AudSystemWrapper::createSoundNameConverter() {
    if (!backend(*this).initialized())
        aurora::throw_host_exception<std::logic_error>("Audio name publication requires completed backend initialization");
}
void AudSystemWrapper::updateRhythm() { (void)backend(*this); }
void AudSystemWrapper::movement() { (void)backend(*this); }
void AudSystemWrapper::stopAllSound(u32) { backend(*this).stop_all(); }
bool AudSystemWrapper::isLoadDoneWaveDataAtSystemInit() const { return backend(*this).initialized(); }
void AudSystemWrapper::loadStaticWaveData() { backend(*this).request_banks(Backend::BankGroup::Static); }
bool AudSystemWrapper::isLoadDoneStaticWaveData() const { return backend(*this).banks_complete(Backend::BankGroup::Static); }
void AudSystemWrapper::loadStageWaveData(const char*, const char*, bool) { backend(*this).request_banks(Backend::BankGroup::Stage); }
bool AudSystemWrapper::isLoadDoneStageWaveData() const { return backend(*this).banks_complete(Backend::BankGroup::Stage); }
void AudSystemWrapper::loadScenarioWaveData(const char*, const char*, s32) { backend(*this).request_banks(Backend::BankGroup::Scenario); }
bool AudSystemWrapper::isLoadDoneScenarioWaveData() const { return backend(*this).banks_complete(Backend::BankGroup::Scenario); }
bool AudSystemWrapper::isPermitToReset() const { return !_2A; }
void AudSystemWrapper::prepareReset() {
    if (!backend(*this).initialized()) _29 = true;
    else backend(*this).prepare_reset();
}
void AudSystemWrapper::requestReset(bool) {
    if (!backend(*this).initialized()) _29 = true;
    else { backend(*this).request_reset(); backend(*this).stop_all(); }
}
bool AudSystemWrapper::isResetDone() { return _29 || backend(*this).reset_complete(); }
void AudSystemWrapper::resumeReset() {
    if (_29) _29 = false;
    backend(*this).resume_reset();
}
