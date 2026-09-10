#include "JasStreamPcmBackend.hpp"
#include <aurora/allocation.hpp>
#include <aurora/endian.hpp>
#include <aurora/exception.hpp>
#include <aurora/j_audio_stream.hpp>
#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <stdexcept>

namespace aurora::audio {
namespace {
thread_local JasStreamPcmBackend* active_backend;
}
struct JasStreamPcmBackend::Impl {
    struct Stream {
        std::optional<JAudioStreamRecipe> recipe;
        VoiceToken token;
        bool prepare_pending = false;
        bool finish_pending = false;
        bool stopping = false;
    };
    PcmAudioMixer& mixer;
    StreamLoader loader;
    std::map<JASAramStream*, Stream> streams;
    Impl(PcmAudioMixer& mixer, StreamLoader loader) : mixer(mixer), loader(std::move(loader)) {}
    bool controls(JASAramStream& stream, const Stream& entry) {
        std::array<PcmLayerControls, JASAramStream::CHANNEL_MAX> layers;
        for (std::size_t i = 0; i < entry.recipe->channel_count; ++i) {
            layers[i] = {std::max(0.0F, stream.mVolume * stream.mChannelVolume[i]), std::clamp(stream.mChannelPan[i], 0.0F, 1.0F)};
        }
        return mixer.try_update_voice_controls(entry.token, stream.mPitch, stream._0AE != 0,
            std::span(layers).first(entry.recipe->channel_count));
    }
};
JasStreamPcmBackend::JasStreamPcmBackend(PcmAudioMixer& mixer, StreamLoader loader) {
    const allocation::HostAllocationScope host;
    if (!loader) {
        throw_host_exception<std::logic_error>("JAS stream backend requires an explicit loader");
    }
    impl_ = std::make_unique<Impl>(mixer, std::move(loader));
}
JasStreamPcmBackend::~JasStreamPcmBackend() {
    shutdown();
}
JasStreamPcmBackend::Binding::Binding(JasStreamPcmBackend& owner) : previous_(active_backend) { active_backend = &owner; }
JasStreamPcmBackend::Binding::~Binding() { active_backend = previous_; }
JasStreamPcmBackend& JasStreamPcmBackend::active() {
    if (active_backend == nullptr) { throw_host_exception<std::logic_error>("JAS stream has no active PCM owner"); }
    return *active_backend;
}
void JasStreamPcmBackend::initialize(JASAramStream& stream) {
    const allocation::HostAllocationScope host;
    if (impl_->streams.contains(&stream)) {
        throw_host_exception<std::logic_error>("A live JAS stream cannot be reinitialized");
    }
    // init alone submits no work; original JAI may retire its owner before prepare.
}
bool JasStreamPcmBackend::prepare(JASAramStream& stream, s32 entry, int) {
    const allocation::HostAllocationScope host;
    if (impl_->streams.contains(&stream) || stream._114 != 0) { return false; }
    auto bytes = impl_->loader(entry);
    if (bytes.empty()) { return false; }
    auto recipe = decode_jaudio_stream(bytes, 0);
    if (recipe.channel_count > JASAramStream::CHANNEL_MAX) {
        throw_host_exception<std::runtime_error>("JAS stream exceeds its six original channels");
    }
    stream._158 = bytes[9];
    stream.mChannelNum = recipe.channel_count;
    stream._164 = recipe.sample_rate;
    stream.mLoop = recipe.looping;
    stream.mLoopStart = recipe.loop_start;
    stream.mLoopEnd = recipe.loop_end;
    stream.mVolume = bytes[0x28] / 127.0F;
    auto& state = impl_->streams.try_emplace(&stream).first->second;
    state.recipe = std::move(recipe);
    state.prepare_pending = true;
    return true;
}
bool JasStreamPcmBackend::cancel(JASAramStream& stream) {
    auto& state = impl_->streams.at(&stream);
    state.finish_pending = true;
    return true;
}
void JasStreamPcmBackend::update() {
    const allocation::HostAllocationScope host;
    std::vector<JASAramStream*> snapshot;
    snapshot.reserve(impl_->streams.size());
    for (const auto& [stream, _] : impl_->streams) { snapshot.push_back(stream); }
    for (auto* stream : snapshot) {
        auto it = impl_->streams.find(stream);
        if (it == impl_->streams.end()) { continue; }
        auto& state = it->second;
        if (state.prepare_pending && !state.finish_pending) {
            state.prepare_pending = false;
            stream->_0AC = true;
            // Retail prepareFinishTask invokes callback1; it does not mark playback started.
            if (stream->mCallback != nullptr) {
                const allocation::ClientAllocationScope callback_scope;
                stream->mCallback(JASAramStream::CB_STOP, stream, stream->mCallbackData);
            }
        }
        OSMessage message;
        if (stream->_0AC && !state.finish_pending) {
            while (OSReceiveMessage(&stream->_000, &message, OS_MESSAGE_NOBLOCK)) {
                const auto command = reinterpret_cast<uintptr_t>(message);
                switch (command & 0xFFU) {
                case 0:
                    if (!state.token && state.recipe) {
                        auto spec = state.recipe->voice;
                        spec.pitch_multiplier = stream->mPitch;
                        for (std::size_t i = 0; i < spec.layers.size(); ++i) {
                            spec.layers[i].gain = std::max(0.0F, stream->mVolume * stream->mChannelVolume[i]);
                            spec.layers[i].pan = std::clamp(stream->mChannelPan[i], 0.0F, 1.0F);
                        }
                        state.token = impl_->mixer.start_voice(spec);
                    }
                    break;
                case 1:
                    if (!state.stopping && state.token) {
                        const auto release = static_cast<u16>(command >> 16U);
                        if ((release >> 14U) != 0) {
                            throw_host_exception<std::logic_error>("Nonlinear JAS direct-release curves are not yet connected to PCM");
                        }
                        // JAS oscillators advance48000/DACrate per80-sample DSP subframe.
                        // Paused release retires immediately in original JASChannel::updateDSPChannel.
                        if (stream->_0AE != 0 || release == 0) { impl_->mixer.stop_voice(state.token); }
                        else { impl_->mixer.fade_out_voice(state.token, release / 600.0); }
                        state.stopping = true;
                    }
                    break;
                case 2: stream->_0AE |= 1; break;
                case 3: stream->_0AE &= ~1; break;
                }
            }
        }
        if (state.token && !state.finish_pending) {
            if (!impl_->controls(*stream, state)) { state.finish_pending = true; }
        }
        if (state.finish_pending) {
            if (state.token) { impl_->mixer.stop_voice(state.token); }
            auto callback = stream->mCallback;
            auto* context = stream->mCallbackData;
            stream->mCallback = nullptr;
            impl_->streams.erase(it);
            // Retail finishTask invokes callback0 once, after the backend has stopped.
            if (callback != nullptr) {
                const allocation::ClientAllocationScope callback_scope;
                callback(JASAramStream::CB_START, stream, context);
            }
        }
    }
}
void JasStreamPcmBackend::shutdown() {
    const allocation::HostAllocationScope host;
    for (auto& [stream, state] : impl_->streams) { state.finish_pending = true; }
    update();
}
VoiceToken JasStreamPcmBackend::voice(const JASAramStream& stream) const {
    const auto it = impl_->streams.find(const_cast<JASAramStream*>(&stream));
    return it == impl_->streams.end() ? VoiceToken{} : it->second.token;
}
std::size_t JasStreamPcmBackend::stream_count() const { return impl_->streams.size(); }
}
