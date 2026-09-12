#pragma once

#include "Game/AudioLib/AudSoundObject.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include <memory>
#include <cstdint>
#include <vector>

namespace smgpc::runtime { class JAudioPlaybackService; }

namespace aurora::audio {
class OriginalAudioNameLifetime;
// Runtime owner of the actual disabled system object. The retained process
// heap outlives the SDK object and its handles, including during unwind.
class DisabledObjectAudioService final {
public:
    explicit DisabledObjectAudioService(std::shared_ptr<smgpc::compat::JkrHeapRuntime> heaps,
                                        smgpc::runtime::JAudioPlaybackService* playback = nullptr);
    // The original process heap owns this variant; its finalizer must retire
    // the service before the heap storage is reused. No lease is retained.
    DisabledObjectAudioService(JKRHeap& heap, std::vector<std::uint8_t> native_names);
    ~DisabledObjectAudioService();
    DisabledObjectAudioService(const DisabledObjectAudioService&) = delete;
    DisabledObjectAudioService& operator=(const DisabledObjectAudioService&) = delete;
    AudSoundObject* system_object() noexcept { return &_system_object; }

private:
    std::shared_ptr<smgpc::compat::JkrHeapRuntime> _heaps;
    AudSoundObject _system_object;
    std::unique_ptr<OriginalAudioNameLifetime> _names;
    DisabledObjectAudioService* _previous;
};

std::unique_ptr<DisabledObjectAudioService> make_disabled_object_audio_service(
    std::shared_ptr<smgpc::compat::JkrHeapRuntime> heaps,
    smgpc::runtime::JAudioPlaybackService* playback = nullptr);
// Borrowed only for the currently active service lifetime, like AudWrap's
// ordinary system owner. It never creates an owner as a query side effect.
AudSoundObject* disabled_system_sound_object() noexcept;
}
