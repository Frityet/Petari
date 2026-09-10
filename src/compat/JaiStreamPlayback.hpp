#pragma once
#include <JSystem/JAudio2/JAIStream.hpp>
#include <aurora/audio.hpp>
#include <aurora/j_audio_sound_archive.hpp>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>
namespace smgpc::compat {
class JaiStreamPlayback final {
public:
    using Loader = std::function<std::vector<std::uint8_t>(std::string_view)>;
    JaiStreamPlayback(aurora::audio::PcmAudioMixer&, Loader);
    ~JaiStreamPlayback();
    JaiStreamPlayback(const JaiStreamPlayback&) = delete;
    JaiStreamPlayback& operator=(const JaiStreamPlayback&) = delete;
    JAIStream* start(const aurora::audio::JAudioSoundMetadata&, JAISoundHandle&, bool prepared);
    void advance();
    void mix();
    void reconcile();
    void reset();
    [[nodiscard]] JAIStream* find(const JAISound*) const;
    [[nodiscard]] aurora::audio::VoiceToken token(const JAISound*) const;
private:
    struct Storage;
    std::unique_ptr<Storage> _storage;
};
}
