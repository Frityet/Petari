#include "JaiStreamPlayback.hpp"
#include "compat/jaudio/JasStreamPcmBackend.hpp"
#include <JSystem/JAudio2/JAIStreamMgr.hpp>
#include <JSystem/JAudio2/JAIStreamDataMgr.hpp>
#include <JSystem/JAudio2/JAISoundChild.hpp>
#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>
#include <map>
#include <limits>
#include <stdexcept>

void retire_jas_pool(JASGenericMemPool&);
namespace smgpc::compat {
namespace {
// Genuine original pools, retained jointly by native manager owners. Their
// host backing deliberately survives Game scene heaps; AudSystem migration
// will instead supply its actual JAS heap/initializer as the owner.
struct Pools {
    Pools() {
        aurora::allocation::HostAllocationScope host;
        if (JASDram != nullptr || JAIStream::memPool_.getTotalMemCount() != 0 ||
            JAISoundChild::memPool_.getTotalMemCount() != 0) {
            aurora::throw_host_exception<std::logic_error>("JAI stream pools already have a different owner");
        }
        JAIStream::newMemPool(3); // AudParams::numFreeStream
        JAISoundChild::newMemPool(160); // AudParams::numFreeSoundChild
    }
    ~Pools() {
        retire_jas_pool(JAIStream::memPool_);
        retire_jas_pool(JAISoundChild::memPool_);
    }
};
std::weak_ptr<Pools> pool_owner;
std::shared_ptr<Pools> retain_pools() {
    auto owner = pool_owner.lock();
    if (!owner) { owner = std::make_shared<Pools>(); pool_owner = owner; }
    return owner;
}
}
struct JaiStreamPlayback::Storage : JAIStreamDataMgr, JAIStreamAramMgr {
    std::shared_ptr<Pools> pools;
    Loader loader;
    std::vector<aurora::audio::JAudioSoundMetadata> entries;
    std::map<uintptr_t, std::unique_ptr<u8[]>> aram;
    aurora::audio::JasStreamPcmBackend backend;
    JAIStreamMgr manager{false};
    Storage(aurora::audio::PcmAudioMixer& mixer, Loader load)
        : pools(retain_pools()), loader(std::move(load)), backend(mixer, [this](s32 entry) {
            return loader(entries.at(static_cast<std::size_t>(entry)).stream_path);
        }) {
        manager.setStreamDataMgr(this);
        manager.setStreamAramMgr(this);
    }
    s32 getStreamFileEntry(JAISoundID id) override {
        for (std::size_t i = 0; i < entries.size(); ++i) if (entries[i].sound_id == u32(id)) return static_cast<s32>(i);
        return -1;
    }
    void* newStreamAram(u32* size) override {
        aurora::allocation::HostAllocationScope host;
        *size = 0x2800 * 36; // AudSystem's 36 authored JAS blocks per stream chunk.
        auto buffer = std::make_unique<u8[]>(*size);
        auto* result = buffer.get();
        aram.emplace(reinterpret_cast<uintptr_t>(result), std::move(buffer));
        return result;
    }
    bool deleteStreamAram(uintptr_t address) override { return aram.erase(address) != 0; }
};
JaiStreamPlayback::JaiStreamPlayback(aurora::audio::PcmAudioMixer& mixer, Loader loader) {
    aurora::allocation::HostAllocationScope host;
    _storage = std::make_unique<Storage>(mixer, std::move(loader));
}
JaiStreamPlayback::~JaiStreamPlayback() { reset(); }
JAIStream* JaiStreamPlayback::start(const aurora::audio::JAudioSoundMetadata& metadata, JAISoundHandle& handle, bool prepared) {
    aurora::allocation::HostAllocationScope host;
    aurora::audio::JasStreamPcmBackend::Binding active(_storage->backend);
    if (metadata.kind != aurora::audio::JAudioSoundKind::Stream || metadata.stream_path.empty()) {
        aurora::throw_host_exception<std::invalid_argument>("Original stream owner requires actual STRM metadata");
    }
    if (_storage->getStreamFileEntry(JAISoundID(metadata.sound_id)) < 0) {
        if (_storage->entries.size() >= static_cast<std::size_t>(std::numeric_limits<s32>::max())) {
            aurora::throw_host_exception<std::length_error>("JAI stream entry catalog exceeds its original signed index");
        }
        _storage->entries.push_back(metadata);
    }
    // The original method returns false even after successful attachment.
    _storage->manager.startSound(JAISoundID(metadata.sound_id), &handle, nullptr);
    if (!handle.isSoundAttached()) return nullptr;
    auto* sound = handle->asStream();
    if (!sound) aurora::throw_host_exception<std::logic_error>("JAI stream manager attached a different sound type");
    // Typed BST resource boundary, using JAUStdSoundInfo's actual field rules.
    sound->mParams.mProperty.mVolume = metadata.volume * (1.0F / 255.0F);
    auto bits = metadata.channel_control;
    for (int i = 0; i < sound->getNumChild() && bits; ++i, bits >>= 2) {
        const auto control = bits & 3;
        if (control == 0) continue;
        auto* child = sound->getChild(i);
        if (child) child->mMove.mParams.mPan = control == 1 ? 0.5F : (control == 2 ? 0.0F : 1.0F);
    }
    if (prepared) sound->lockWhenPrepared();
    return sound;
}
void JaiStreamPlayback::advance() {
    aurora::audio::JasStreamPcmBackend::Binding active(_storage->backend);
    _storage->manager.calc();
    _storage->manager.mixOut();
    _storage->backend.update();
}
void JaiStreamPlayback::mix() {
    aurora::audio::JasStreamPcmBackend::Binding active(_storage->backend);
    _storage->manager.mixOut();
    _storage->backend.update();
}
void JaiStreamPlayback::reconcile() { _storage->backend.update(); _storage->manager.freeDeadStream_(); }
void JaiStreamPlayback::reset() {
    aurora::audio::JasStreamPcmBackend::Binding active(_storage->backend);
    _storage->manager.stop();
    _storage->backend.shutdown();
    _storage->manager.calc();
    if (_storage->manager.isActive() || !_storage->aram.empty()) {
        aurora::throw_host_exception<std::logic_error>("Original JAI streams did not retire before their native owner");
    }
}
JAIStream* JaiStreamPlayback::find(const JAISound* sound) const {
    for (auto* item = _storage->manager.getStreamList()->getFirst(); item; item = item->getNext()) {
        if (static_cast<JAISound*>(item->getObject()) == sound) return item->getObject();
    }
    return nullptr;
}
aurora::audio::VoiceToken JaiStreamPlayback::token(const JAISound* sound) const {
    auto* stream = find(sound);
    return stream ? _storage->backend.voice(stream->inner_.aramStream) : aurora::audio::VoiceToken{};
}
}
