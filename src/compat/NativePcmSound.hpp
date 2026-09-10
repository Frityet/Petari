#pragma once
#include <JSystem/JAudio2/JAISound.hpp>
#include <JSystem/JAudio2/JAISoundChild.hpp>
#include <aurora/audio.hpp>
#include <array>
#include <optional>

namespace smgpc::compat {
// Native decoded-PCM sound, with the actual original JAISound fields/handle contract.
// This is not a JAISe or a sequence track; inherited asSe()/asSeq() report null.
class NativePcmSound final : public JAISound {
public:
    NativePcmSound(aurora::audio::PcmAudioMixer&, JAISoundID, const aurora::audio::PcmVoiceSpec&);
    ~NativePcmSound();
    NativePcmSound(const NativePcmSound&) = delete;
    NativePcmSound& operator=(const NativePcmSound&) = delete;
    s32 getNumChild() const override;
    JAISoundChild* getChild(int) override;
    void releaseChild(int) override;
    JASTrack* getTrack() override;
    JASTrack* getChildTrack(int) override;
    JAITempoMgr* getTempoMgr() override;
    bool JAISound_tryDie_() override;
    void advance();
    void reconcile_completion();
    void mix();
    [[nodiscard]] aurora::audio::VoiceToken token() const;
    [[nodiscard]] bool releasing() const;
private:
    aurora::audio::PcmAudioMixer& _mixer;
    aurora::audio::PcmVoiceSpec _base;
    aurora::audio::VoiceToken _token;
    std::vector<std::optional<JAISoundChild>> _children;
    std::vector<aurora::audio::PcmLayerControls> _controls;
    bool _releasing = false;
};
}
