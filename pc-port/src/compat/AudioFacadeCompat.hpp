#pragma once

namespace smgpc::runtime {
    class AudioEventService;
}

namespace smgpc::compat {

    [[nodiscard]] smgpc::runtime::AudioEventService *try_active_audio_event_service();
    [[nodiscard]] smgpc::runtime::AudioEventService &require_active_audio_event_service();
    void synchronize_audio_facade_state();

    class ScopedAudioEventServiceOverride final {
    public:
        explicit ScopedAudioEventServiceOverride(smgpc::runtime::AudioEventService &service);
        ~ScopedAudioEventServiceOverride();

        ScopedAudioEventServiceOverride(const ScopedAudioEventServiceOverride &) = delete;
        ScopedAudioEventServiceOverride &operator=(const ScopedAudioEventServiceOverride &) = delete;

    private:
        smgpc::runtime::AudioEventService *_previous = nullptr;
    };

}  // namespace smgpc::compat
