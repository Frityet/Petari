#pragma once

#include "Game/AudioLib/AudSoundObject.hpp"
#include "Game/AudioLib/AudSceneMgr.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/JAudioCategoryVolumeOwnership.hpp"
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
    AudSceneMgr* scene_manager() noexcept { return &_scene_manager; }
    void set_trigger_sound_permitted(bool permitted);
    void set_level_sound_permitted(bool permitted);
    bool is_sound_permitted() const;
    void set_sound_volume_setting(s32 volume_set, u32 steps);
    void recover_sound_volume_setting(u32 steps);
    float sound_category_gain(u32 sound_id) const;
    void reset_scene_controls();
    void update_scene_controls();

private:
    std::shared_ptr<smgpc::compat::JkrHeapRuntime> _heaps;
    AudSoundObject _system_object;
    // Scene flags and player selection remain actual original fields even when
    // this backend has no JAU wave heap or output device.
    AudSceneMgr _scene_manager{nullptr};
    smgpc::compat::JAudioCategoryVolumeOwnership _volumes;
    smgpc::runtime::JAudioPlaybackService* _playback = nullptr;
    bool _trigger_sound_permitted = true;
    bool _level_sound_permitted = true;
    std::unique_ptr<OriginalAudioNameLifetime> _names;
    DisabledObjectAudioService* _previous;
};

std::unique_ptr<DisabledObjectAudioService> make_disabled_object_audio_service(
    std::shared_ptr<smgpc::compat::JkrHeapRuntime> heaps,
    smgpc::runtime::JAudioPlaybackService* playback = nullptr);
// Borrowed only for the currently active service lifetime, like AudWrap's
// ordinary system owner. It never creates an owner as a query side effect.
AudSoundObject* disabled_system_sound_object() noexcept;
AudSceneMgr* disabled_audio_scene_manager() noexcept;
DisabledObjectAudioService* disabled_object_audio_service() noexcept;
}
