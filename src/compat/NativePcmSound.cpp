#include "NativePcmSound.hpp"
#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>
#include <algorithm>
#include <stdexcept>
#include <limits>

namespace smgpc::compat {
NativePcmSound::NativePcmSound(aurora::audio::PcmAudioMixer& mixer, JAISoundID id,
                             const aurora::audio::PcmVoiceSpec& specification)
    : _mixer(mixer) {
    const aurora::allocation::HostAllocationScope host;
    _base = specification;
    if (_base.layers.size() > static_cast<std::size_t>(std::numeric_limits<s32>::max())) {
        aurora::throw_host_exception<std::length_error>("Native PCM channel count exceeds the JAudio signed index");
    }
    _children.resize(_base.layers.size());
    _controls.resize(_base.layers.size());
    start_JAISound_(id, nullptr, nullptr);
    mParams.mProperty.mVolume = _base.gain_multiplier;
    mParams.mProperty.mPitch = _base.pitch_multiplier;
    _base.gain_multiplier = 1.0F;
    _base.pitch_multiplier = 1.0F;
    _token = _mixer.start_voice(_base);
    mStatus.setPlaying();
    try { mix(); } catch (...) {
        _mixer.stop_voice(_token);
        die_JAISound_();
        throw;
    }
}
NativePcmSound::~NativePcmSound() {
    _mixer.stop_voice(_token);
    die_JAISound_();
}
s32 NativePcmSound::getNumChild() const { return static_cast<s32>(_base.layers.size()); }
JAISoundChild* NativePcmSound::getChild(int index) {
    if (index < 0 || index >= getNumChild()) {
        aurora::throw_host_exception<std::out_of_range>("Native PCM child index is outside the decoded channels");
    }
    auto& child = _children[static_cast<std::size_t>(index)];
    if (!child) child.emplace();
    return &*child;
}
void NativePcmSound::releaseChild(int index) {
    if (index < 0 || index >= getNumChild()) {
        aurora::throw_host_exception<std::out_of_range>("Native PCM child index is outside the decoded channels");
    }
    _children[static_cast<std::size_t>(index)].reset();
}
JASTrack* NativePcmSound::getTrack() { return nullptr; }
JASTrack* NativePcmSound::getChildTrack(int) { return nullptr; }
JAITempoMgr* NativePcmSound::getTempoMgr() { return nullptr; }
bool NativePcmSound::JAISound_tryDie_() {
    if (_mixer.is_voice_active(_token)) {
        if (!_releasing) {
            _mixer.release_voice(_token);
            _releasing = true;
        }
        return false;
    }
    die_JAISound_();
    return true;
}
void NativePcmSound::reconcile_completion() {
    if (!isDead() && !_mixer.is_voice_active(_token)) {
        die_JAISound_();
    }
}
void NativePcmSound::advance() {
    reconcile_completion();
    if (isDead()) return;
    if (calc_JAISound_()) {
        for (auto& child : _children) if (child) child->calc();
    }
    // Original JAISe mixOut stops a newly expired lifetime in this same frame.
    if (isStopping()) JAISound_tryDie_();
    if (!isDead()) mix();
}
void NativePcmSound::mix() {
    if (isDead()) return;
    JASSoundParams parent;
    JASSoundParams output;
    mParams.mixOutAll(parent, &output, isMute() ? 0.0F : mFader.getIntensity());

    for (std::size_t i = 0; i < _base.layers.size(); ++i) {
        float volume = output.mVolume;
        float pan = _base.layers[i].pan + output.mPan - 0.5F;
        if (_children[i]) {
            volume *= _children[i]->mMove.mParams.mVolume;
            pan += _children[i]->mMove.mParams.mPan - 0.5F;
        }
        _controls[i] = {std::max(0.0F, _base.layers[i].gain * volume), std::clamp(pan, 0.0F, 1.0F)};
    }
    if (!_mixer.try_update_voice_controls(_token, output.mPitch, isPaused(),
            _controls)) {
        die_JAISound_();
    }
}
aurora::audio::VoiceToken NativePcmSound::token() const { return isDead() ? aurora::audio::VoiceToken{} : _token; }
bool NativePcmSound::releasing() const { return _releasing; }
}
