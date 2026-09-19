#include "Game/AudioLib/AudAnmSoundObject.hpp"
#include "Game/AudioLib/AudSoundId.hpp"
#include "Game/AudioLib/AudSoundNameConverter.hpp"
#include "Game/AudioLib/AudSpeakerWrap.hpp"
#include "Game/AudioLib/AudSceneMgr.hpp"
#include "Game/AudioLib/AudParams.hpp"
#include "Game/AudioLib/AudWrap.hpp"
#include "JSystem/JAudio2/JAUSoundTable.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "Game/GameAudio/AudTalkSoundData.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "compat/DisabledObjectAudio.hpp"
#include "compat/DisabledAudioBackend.hpp"
#include "Game/System/AudSystemWrapper.hpp"
#include "JSystem/JKernel/JKRSolidHeap.hpp"
#include <aurora/guest_thread.hpp>
#include "compat/DisabledObjectAudioService.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "runtime/JAudioPlaybackService.hpp"
#include <JSystem/JAudio2/JAIStream.hpp>
#include <resource/Yaz0.hpp>
#include <aurora/audio.hpp>
#include <array>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace {
const std::filesystem::path fixture = std::getenv("SMGPC_RETAIL_FILES_ROOT") ? std::getenv("SMGPC_RETAIL_FILES_ROOT") : "notes/original-audio-category-volume-20260907/fixture";
std::vector<u8> read(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    assert(input.good());
    return {std::istreambuf_iterator<char>(input), {}};
}
}
#include "compat/NativePcmSound.hpp"
#include "compat/JaiStreamPlayback.hpp"
#include <JSystem/JAudio2/JAIStream.hpp>
#include <aurora/j_audio_stream.hpp>
#include <array>
#include <cassert>
#include <cstdio>
#include <fstream>

void test_original_limited_sound_records() {
    smgpc::compat::JAudioLimitedSoundOwnership limits;
    const JAISoundID first(0x60001), second(0x60002), third(0x60003);
    assert(!limits.contains(first) && !limits.contains(second));
    limits.register_sound(first, 2);
    limits.register_sound(second, 3);
    limits.register_sound(third, 20);
    assert(limits.contains(first) && limits.contains(second) && !limits.contains(third));
    limits.update();
    limits.register_sound(first, 100);
    limits.update();
    assert(!limits.contains(first) && limits.contains(second)); // Duplicate did not extend expiry.
    limits.register_sound(third, 0);
    assert(limits.contains(third)); // Zero delay still occupies its slot until the next update.
    limits.update();
    assert(!limits.contains(second) && !limits.contains(third));
    limits.register_sound(first, -2);
    assert(limits.contains(first));
    limits.update();
    assert(!limits.contains(first));
    limits.register_sound(first, 10);
    limits.register_sound(second, 10);
    limits.clear();
    assert(!limits.contains(first) && !limits.contains(second));
}

void test_disabled_audio_scene_state() {
    test_original_limited_sound_records();
    const aurora::os::GuestThreadExecutionScope execution;
    const auto require_absent = [] {
        bool rejected = false;
        try { (void)AudWrap::getSceneMgr(); }
        catch (const std::logic_error&) { rejected = true; }
        assert(rejected && aurora::audio::disabled_audio_scene_manager() == nullptr);
    };
    require_absent();
    const auto heaps = smgpc::compat::JkrHeapRuntime::create(2U * 1024U * 1024U);
    const auto initial_free = heaps->root_heap().getFreeSize();
    for (int cycle = 0; cycle != 3; ++cycle) {
        {
            aurora::audio::DisabledObjectAudioService output(heaps);
            auto* scene = AudWrap::getSceneMgr();
            assert(scene == output.scene_manager());
            assert(scene->mSectionHeap == nullptr && scene->_4 == 0 && scene->mSeWaveSetId == 0 &&
                   scene->mSeScenarioWaveSetId == -1 && scene->mBgmWaveSetId == 0 &&
                   scene->mPlayerMode == 0 && scene->mPrevPlayerMode == 0 && !scene->mIsNewPlayerMode);
            assert(!MR::isCubeBgmChangeInvalid());
            assert(MR::isPermitSE());
            MR::submitTrigSE();
            assert(!MR::isPermitSE());
            MR::permitTrigSE();
            MR::submitLevelSE();
            assert(!MR::isPermitSE());
            MR::permitLevelSE();
            assert(MR::isPermitSE());
            MR::setSoundVolumeSetting(3, 4);
            for (int step = 1; step <= 4; ++step) {
                output.update_scene_controls();
                const auto expected = AudParams::scCtgVolume[0][6] +
                    (AudParams::scCtgVolume[3][6] - AudParams::scCtgVolume[0][6]) * (step / 4.0f);
                assert(std::abs(output.sound_category_gain(0x60000) - expected) < 0.00001f);
            }
            MR::setSoundVolumeSetting(2, 0);
            MR::recoverSoundVolumeSetting(0);
            assert(output.sound_category_gain(0x60000) == AudParams::scCtgVolume[3][6]);
            MR::recoverSoundVolumeSetting(0);
            assert(output.sound_category_gain(0x60000) == AudParams::scCtgVolume[0][6]);
            MR::setSoundVolumeSetting(3, 0);
            MR::submitSE();
            MR::setCubeBgmChangeInvalid();
            scene->_4 = 1;
            assert(scene->_1D && MR::isCubeBgmChangeInvalid());
            scene->setPlayerModeLuigi();
            scene->startScene();
            assert(scene->_4 == 0 && !MR::isCubeBgmChangeInvalid() && scene->isPlayerModeLuigi());
            assert(MR::isPermitSE() && output.sound_category_gain(0x60000) == AudParams::scCtgVolume[0][6]);
            output.register_limited_sound(JAISoundID(0x60001), 2);
            scene->startScene();
            assert(output.is_limited_sound(JAISoundID(0x60001))); // Scene volume reset preserves the original limiter.
            output.update_scene_controls();
            assert(output.is_limited_sound(JAISoundID(0x60001)));
            output.update_scene_controls();
            assert(!output.is_limited_sound(JAISoundID(0x60001)));
            scene->setPlayerModeMario();
            assert(scene->isPlayerModeMario() && !scene->isPlayerModeLuigi());
            MR::setCubeBgmChangeInvalid();
            {
                aurora::audio::DisabledObjectAudioService nested(heaps);
                assert(AudWrap::getSceneMgr() == nested.scene_manager() && !MR::isCubeBgmChangeInvalid());
                bool rejected = false;
                try { scene->startScene(); }
                catch (const std::logic_error&) { rejected = true; }
                assert(rejected && scene->_1D);
                MR::setCubeBgmChangeInvalid();
                AudWrap::getSceneMgr()->startScene();
                assert(!MR::isCubeBgmChangeInvalid());
            }
            assert(AudWrap::getSceneMgr() == scene && MR::isCubeBgmChangeInvalid());
            scene->startScene();
            assert(!MR::isCubeBgmChangeInvalid() && output.system_object()->getNumHandles() == 0);
        }
        require_absent();
        assert(heaps->root_heap().getFreeSize() == initial_free);
    }
    std::puts("[pass] disabled audio scene owner: original flags, SoundUtil mutations, scene resets, player mode, nested publication and three heap retirements");
}

void test_original_talk_sound_dispatch() {
    using aurora::audio::DisabledObjectAudio;
    assert(!DisabledObjectAudio::enabled());
    const auto initial_requests = DisabledObjectAudio::declined_requests();
    for (u8 sound_no : {u8(0), u8(0x3f), u8(0xa2), u8(0xff)}) {
        assert(AudTalkSoundData::getSoundIDFromTalkSoundNo(sound_no).isAnonymous());
        MR::startTalkSound(sound_no, nullptr);
    }
    assert(DisabledObjectAudio::declined_requests() == initial_requests);
    const auto require_absent_owner_rejection = [] {
        const auto before = DisabledObjectAudio::declined_requests();
        bool rejected = false;
        try { MR::startTalkSound(7, nullptr); }
        catch (const std::logic_error&) { rejected = true; }
        assert(rejected && DisabledObjectAudio::declined_requests() == before);
    };
    require_absent_owner_rejection();
    assert(u32(AudTalkSoundData::getSoundIDFromTalkSoundNo(7)) == SE_SV_RABBIT_TALK_NORMAL);
    assert(u32(AudTalkSoundData::getSoundIDFromTalkSoundNo(0xa1)) == SE_SV_CARETAKER_ANGRY_FAST);

    const auto heaps = smgpc::compat::JkrHeapRuntime::create(2U * 1024U * 1024U);
    for (int generation = 0; generation < 2; ++generation) {
        const auto before = DisabledObjectAudio::declined_requests();
        {
            aurora::audio::DisabledObjectAudioService audio(heaps);
            assert(aurora::audio::disabled_system_sound_object() == audio.system_object());
            MR::startTalkSound(7, nullptr);
            assert(DisabledObjectAudio::declined_requests() == before + 1);
            assert(audio.system_object()->getNumHandles() == 0);

            LiveActor actor("OriginalTalkSoundFixture");
            AudAnmSoundObject sound(&actor.mPosition, 2, &heaps->root_heap());
            actor.mSoundObject = &sound;
            sound.setMapCode(7);
            MR::startTalkSound(7, &actor);
            assert(DisabledObjectAudio::declined_requests() == before + 2);
            assert(sound.getMapCode() == 0 && !sound.isPlayingID(SE_SV_RABBIT_TALK_NORMAL));
            assert(MR::startSoundObjectLevel(&sound, JAISoundID(SE_SV_RABBIT_TALK_NORMAL), 5) == nullptr);
            assert(MR::startSoundObjectLevelParam(&sound, JAISoundID(SE_SV_RABBIT_TALK_NORMAL), 1, 2, 5) == nullptr);
            assert(DisabledObjectAudio::declined_requests() == before + 4);
            actor.mSoundObject = nullptr;
        }
        assert(aurora::audio::disabled_system_sound_object() == nullptr);
        require_absent_owner_rejection();
    }
    std::puts("[pass] original talk ID table, system/actor dispatch and null disabled handles across owner generations");
}

void test_native_and_stream_owners() {
    static_assert(sizeof(JAISoundHandle) == sizeof(JAISound*));
    JAISoundID id(2, 13, 0x1234);
    assert(u32(id) == 0x020D1234 && id.getGroupID() == 13 && id.getWaveID() == 0x1234);
    JAISoundStatus_ status;
    status.init();
    status.pause(true);
    assert(status._0.value == 0x40);
    status._0.value = 0x80;
    assert(status.isMute() && !status.isPaused());
    status.setAnimationState(3);
    u8 flags = 0;
    std::memcpy(&flags, &status.mState.flags, sizeof(flags));
    assert(flags == 0x30 && status.getAnimationState() == 3);
    status.init();
    assert(status.lockWhenPrepared() == 1 && status.getState() == JAISoundStatus_::State_LOCK_PREPARE);
    assert(status.unlockIfLocked() == 1 && status.getState() == JAISoundStatus_::State_PREPARE);
    status.setReadyLocked();
    assert(status.unlockIfLocked() == 1 && status.getState() == JAISoundStatus_::State_READY);
    aurora::audio::PcmAudioMixer mixer;
    aurora::audio::PcmVoiceSpec spec;
    aurora::audio::PcmLayer layer;
    layer.samples = std::make_shared<const std::vector<float>>(64, 0.25F);
    layer.sample_rate = 48000;
    layer.loop_end = 64;
    spec.layers.push_back(layer);
    JAISoundHandle handle;
    {
        smgpc::compat::NativePcmSound pcm(mixer, JAISoundID(0x00010001), spec);
        pcm.attachHandle(&handle);
        assert(handle.mSound == &pcm && pcm.mHandle == &handle);
        assert(pcm.asSe() == nullptr && pcm.asSeq() == nullptr && pcm.asStream() == nullptr);
        assert(pcm.getTrack() == nullptr && pcm.getTempoMgr() == nullptr);
        assert(pcm.getChild(0) == pcm.getChild(0));
        pcm.getAuxiliary().moveVolume(0.0F, 2);
        pcm.advance();
        assert(pcm.getAuxiliary().mParams.mVolume == 0.5F);
        pcm.pause(true); pcm.advance();
        assert(pcm.getAuxiliary().mParams.mVolume == 0.5F);
        const auto before = mixer.voice_rendered_frames(pcm.token()).value();
        std::array<float, 32> output;
        mixer.render_interleaved(output);
        assert(mixer.voice_rendered_frames(pcm.token()).value() == before);
        pcm.pause(false); pcm.advance();
        assert(pcm.getAuxiliary().mParams.mVolume == 0.0F);
        mixer.render_interleaved(output);
        for (float sample : output) assert(sample == 0.0F);
        pcm.setLifeTime(1, false); pcm.advance();
        assert(!pcm.isStopping() && pcm.mLifeTime == 0);
        pcm.updateLifeTime(1); pcm.advance(); assert(!pcm.isStopping());
        pcm.advance(); assert(pcm.isStopping() && pcm.releasing());
        mixer.render_interleaved(output); pcm.reconcile_completion();
        assert(pcm.isDead() && !handle.isSoundAttached());
    }
    {
        smgpc::compat::NativePcmSound first(mixer, JAISoundID(0x00010001), spec);
        smgpc::compat::NativePcmSound second(mixer, JAISoundID(0x00010001), spec);
        JAISoundHandle owner;
        first.attachHandle(&owner);
        second.attachHandle(&owner);
        assert(first.mHandle == nullptr && first.isStopping());
        assert(owner.mSound == &second && second.mHandle == &owner && !second.isStopping());
        {
            JAISoundHandle temporary;
            second.attachHandle(&temporary);
            assert(owner.mSound == nullptr && second.mHandle == &temporary);
        }
        assert(second.mHandle == nullptr && !second.isStopping() && mixer.is_voice_active(second.token()));
        second.attachHandle(&owner);
        second.stop(3);
        assert(owner.mSound == &second && !second.isStopping());
        second.pause(true);
        second.advance();
        assert(second.mFader.mTransition.mRemainingSteps == 3);
        second.pause(false);
        second.advance();
        assert(second.mFader.mTransition.mRemainingSteps == 2);
        second.advance();
        second.advance();
        assert(second.isStopping() && second.mFader.isOut());
        std::array<float, 32> output;
        mixer.render_interleaved(output);
        second.reconcile_completion();
        assert(second.isDead() && !owner.isSoundAttached());
    }
    if (!std::filesystem::is_regular_file(fixture / "AudioRes/Stream/SMG_title_strm.ast")) {
        std::puts("[skip] original stream owner retail fixture unavailable");
        return;
    }
    auto load = [](std::string_view) {
        std::ifstream file(fixture / "AudioRes/Stream/SMG_title_strm.ast", std::ios::binary);
        assert(file.good());
        return std::vector<u8>{std::istreambuf_iterator<char>(file), {}};
    };
    for (int generation = 0; generation < 3; ++generation) {
        smgpc::compat::JaiStreamPlayback streams(mixer, load);
        aurora::audio::JAudioSoundMetadata metadata;
        metadata.sound_id = 0x02000001;
        metadata.kind = aurora::audio::JAudioSoundKind::Stream;
        metadata.stream_path = "actual-fixture";
        metadata.volume = 255;
        metadata.channel_control = 0xE;
        JAISoundHandle a, b, c, rejected;
        auto* first = streams.start(metadata, a, true);
        assert(first && a.mSound == first);
        assert(first->getChild(0)->mMove.mParams.mPan == 0.0F);
        assert(first->getChild(1)->mMove.mParams.mPan == 1.0F);
        assert(streams.start(metadata, b, false));
        assert(streams.start(metadata, c, false));
        assert(streams.start(metadata, rejected, false) == nullptr && !rejected.isSoundAttached());
        for (int i = 0; i < 8; ++i) streams.advance();
        assert(first->isPrepared() && !streams.token(first));
        assert(streams.token(b.mSound) && streams.token(c.mSound));
        first->unlockIfLocked(); streams.mix(); assert(streams.token(first));
        JAISoundHandle canonical;
        first->attachHandle(&canonical);
        assert(!a.isSoundAttached() && first->mHandle == &canonical);
        streams.reset();
        assert(!canonical.isSoundAttached() && !b.isSoundAttached() && !c.isSoundAttached());
        assert(streams.find(first) == nullptr);
    }
    std::puts("[pass] native PCM subclass retains real JAISound type, original params/lifetime and reciprocal handle semantics");
    std::puts("[pass] original stream wrapper metadata, actual three-slot pool exhaustion, canonical handle transfer and three process owner generations");
}

void test_retail_service() {
    if (!std::filesystem::is_regular_file(fixture / "KrKorean/AudioRes/SMR.szs")) {
        std::puts("[skip] retail service fixture unavailable");
        return;
    }

    setenv("SDL_AUDIODRIVER", "dummy", 1);
    auto mixer = std::make_unique<aurora::audio::PcmAudioMixer>(48000,
        aurora::audio::PlaybackDevicePolicy::AllowExplicitTestSink);
    auto* pcm = mixer.get();
    auto archive = [] {
        auto baa = smgpc::resource::decompress_yaz0(read(fixture / "KrKorean/AudioRes/SMR.szs"));
        return std::make_unique<aurora::audio::JAudioSoundArchive>(baa, [](std::string_view name) {
            auto path = fixture / "KrKorean/AudioRes/Waves" / name;
            if (!std::filesystem::is_regular_file(path)) path = fixture / "AudioRes/Waves" / name;
            return read(path);
        });
    };
    smgpc::runtime::JAudioPlaybackService service(archive, [](std::string_view path) {
        return read(fixture / std::filesystem::path(path).relative_path());
    }, std::move(mixer));
    using Lane = smgpc::runtime::BgmLane;
    auto* title = service.start_bgm(Lane::Stage, "STM_TITLE", true);
    assert(title && title->mSound && title->mSound->asStream());
    assert(service.owns_sound(title->mSound) && service.has_active_bgm(Lane::Stage));
    auto* stream = title->mSound;
    assert(!service.bgm_backend_token(Lane::Stage));
    for (u64 frame = 0; frame < 8; ++frame) { service.begin_frame(frame); service.end_frame(); }
    assert(service.is_bgm_prepared(Lane::Stage) && !stream->isPlaying());
    assert(!service.bgm_backend_token(Lane::Stage) && service.active_voice_count() == 0);
    JAISoundHandle canonical;
    service.bind_bgm_handle(Lane::Stage, canonical);
    assert(!title->isSoundAttached() && canonical.mSound == stream && stream->mHandle == &canonical);
    assert(service.bgm_handle(Lane::Stage) == &canonical);
    service.bind_bgm_handle(Lane::Stage, canonical);
    assert(canonical.mSound == stream && !stream->isStopping());
    service.unlock_bgm(Lane::Stage);
    const auto token = aurora::audio::VoiceToken{service.bgm_backend_token(Lane::Stage)};
    assert(token && pcm->is_voice_active(token) && stream->isPlaying());
    assert(service.is_bgm_prepared(Lane::Stage)); // Original prepared predicate includes PLAYING.
    service.pause_bgm(Lane::Stage, true);
    assert(stream->isPaused() && pcm->voice_paused(token).value());
    service.set_bgm_bus_gain(Lane::Stage, 0.25F);
    assert(stream->getAuxiliary().mParams.mVolume == 0.25F);
    service.pause_bgm(Lane::Stage, false);
    auto* sub = service.start_bgm(Lane::Sub, "STM_PROLOGUE_01", true);
    assert(sub && sub->mSound && sub->mSound != stream);
    for (u64 frame = 8; frame < 16; ++frame) { service.begin_frame(frame); service.end_frame(); }
    assert(service.is_bgm_prepared(Lane::Sub) && !service.bgm_backend_token(Lane::Sub));
    service.unlock_bgm(Lane::Sub);
    assert(service.bgm_backend_token(Lane::Sub) != token.value);
    service.stop_bgm(Lane::Stage, 0);
    assert(!canonical.isSoundAttached() && !service.has_active_bgm(Lane::Stage));
    assert(sub->isSoundAttached());
    service.reset_scene();
    assert(!sub->isSoundAttached() && service.active_voice_count() == 0);
    service.begin_frame(20);
    auto* wind = service.start_level_sound("SE_AT_LV_ASTRO_DOME_WIND_1", 100, -1);
    assert(wind && wind->mSound && !wind->mSound->asSe() && service.owns_sound(wind->mSound));
    auto* native_sound = wind->mSound;
    const auto wind_token = aurora::audio::VoiceToken{service.sound_backend_token(native_sound)};
    assert(wind_token && pcm->voice_pitch_multiplier(wind_token).value() == 1.5F);
    service.set_sound_volume_setting(1, 0);
    assert(pcm->voice_bus_gain_multiplier(wind_token).value() == 0.3F);
    service.set_trigger_sound_permitted(false);
    service.set_level_sound_permitted(false);
    service.reset_output_controls();
    assert(service.is_sound_permitted());
    assert(wind->mSound == native_sound && service.owns_sound(native_sound));
    assert(pcm->is_voice_active(wind_token) && service.active_voice_count() == 1);
    assert(pcm->voice_bus_gain_multiplier(wind_token).value() == AudParams::scCtgVolume[0][6]);
    service.end_frame();
    service.begin_frame(21);
    assert(service.start_level_sound("SE_AT_LV_ASTRO_DOME_WIND_1", 100, -1)->mSound == native_sound);
    service.end_frame();
    assert(native_sound->mLifeTime == 0 && !native_sound->isStopping());
    service.begin_frame(22); service.end_frame();
    assert(native_sound->isStopping());
    auto* shot = service.start_sound_effect("SE_SY_GAME_START", -1, -1);
    assert(shot && shot->mSound && !shot->mSound->asSe() && service.owns_sound(shot->mSound));
    const auto shot_token = aurora::audio::VoiceToken{service.sound_backend_token(shot->mSound)};
    assert(shot_token && pcm->is_voice_active(shot_token));
    service.stop_sound_effect("SE_SY_GAME_START", 0);
    assert(!shot->isSoundAttached());
    service.begin_frame(23);
    service.reset_scene();
    assert(!wind->isSoundAttached() && service.active_voice_count() == 0);
    service.end_frame();

    service.begin_frame(24);
    auto* limited_wind = service.start_level_sound("SE_AT_LV_ASTRO_DOME_WIND_1", 100, -1);
    const auto wind_id = limited_wind->mSound->mSoundID;
    auto* limited_wind_sound = dynamic_cast<smgpc::compat::NativePcmSound*>(limited_wind->mSound);
    auto* first_shot = service.start_sound_effect("SE_SY_GAME_START", -1, -1);
    auto* second_shot = service.start_sound_effect("SE_SY_GAME_START", -1, -1);
    const auto shot_id = first_shot->mSound->mSoundID;
    auto* first_shot_sound = dynamic_cast<smgpc::compat::NativePcmSound*>(first_shot->mSound);
    auto* second_shot_sound = dynamic_cast<smgpc::compat::NativePcmSound*>(second_shot->mSound);
    service.register_limited_sound(wind_id, 2);
    service.register_limited_sound(shot_id, 3);
    assert(!limited_wind->isSoundAttached() && limited_wind_sound);
    assert(limited_wind_sound->isDead() || limited_wind_sound->releasing());
    assert(!first_shot->isSoundAttached() && !second_shot->isSoundAttached());
    assert(first_shot_sound && (first_shot_sound->isDead() || first_shot_sound->releasing()));
    assert(second_shot_sound && (second_shot_sound->isDead() || second_shot_sound->releasing()));
    assert(service.start_level_sound("SE_AT_LV_ASTRO_DOME_WIND_1", 100, -1) == nullptr);
    assert(service.start_sound_effect("SE_SY_GAME_START", -1, -1) == nullptr); // System SE is limited too.
    auto* limited_stream = service.start_bgm(Lane::Stage, "STM_TITLE", true);
    const auto stream_id = limited_stream->mSound->mSoundID;
    service.register_limited_sound(stream_id, 4);
    assert(!limited_stream->isSoundAttached() && !service.has_active_bgm(Lane::Stage));
    assert(!service.is_limited_sound(stream_id)); // A full table still stops matching voices before rejecting a slot.
    service.reset_output_controls();
    service.reset_scene();
    assert(service.is_limited_sound(wind_id) && service.is_limited_sound(shot_id));
    service.end_frame();
    service.begin_frame(25);
    service.register_limited_sound(wind_id, 100);
    assert(service.is_limited_sound(wind_id));
    service.end_frame();
    service.begin_frame(26);
    assert(!service.is_limited_sound(wind_id) && service.is_limited_sound(shot_id));
    assert(service.start_level_sound("SE_AT_LV_ASTRO_DOME_WIND_1", 100, -1) != nullptr);
    service.end_frame();
    service.begin_frame(27);
    assert(!service.is_limited_sound(shot_id));
    assert(service.start_sound_effect("SE_SY_GAME_START", -1, -1) != nullptr);
    service.reset_scene();
    service.end_frame();
    std::puts("[pass] actual retail playback service uses original stream manager and canonical original sound/handle ABI");
    std::puts("[pass] Stage/Sub prepare-unlock, original states, independent handles, native PCM level/SE params/lifetime and category control");
    std::puts("[pass] original limited-sound slots stop actual level/SE/stream IDs, suppress new SE, preserve reset state and expire without duplicate refresh");
}

void test_original_name_owner() {
    const auto baa = smgpc::resource::decompress_yaz0(read(fixture / "KrKorean/AudioRes/SMR.szs"));
    std::size_t wave_requests = 0;
    const auto wave = [&](std::string_view) {
        ++wave_requests;
        throw std::logic_error("name ownership must not load wave archives");
        return std::vector<u8>{};
    };
    smgpc::runtime::JAudioPlaybackService playback(
        [&] { return std::make_unique<aurora::audio::JAudioSoundArchive>(baa, wave); }, wave,
        std::make_unique<aurora::audio::PcmAudioMixer>());
    assert(!playback.is_device_open());
    const auto heaps = smgpc::compat::JkrHeapRuntime::create(2U * 1024U * 1024U);
    auto* previous = AudSingletonHolder<AudSoundNameConverter>::get();
    auto* previous_table = JAUSoundNameTable::getInstance();
    const auto free_bytes = heaps->root_heap().getFreeSize();
    for (int cycle = 0; cycle != 3; ++cycle) {
        {
            auto owner = aurora::audio::make_disabled_object_audio_service(heaps, &playback);
            auto* converter = AudSingletonHolder<AudSoundNameConverter>::get();
            auto* table = JAUSoundNameTable::getInstance();
            assert(converter && converter != previous && table && table != previous_table);
            for (const char* name : {"SE_SY_GAME_START", "SE_AT_LV_ASTRO_DOME_WIND_1", "STM_PROLOGUE_01"}) {
                assert(u32(converter->getSoundID(name)) == playback.find_sound_id(name).value());
            }
            const auto requests = aurora::audio::DisabledObjectAudio::declined_requests();
            MR::startSystemSE("SE_SY_GAME_START", -1, -1);
            assert(!AudSpeakerWrap::isPlayable(-1));
            MR::startCSSound("CS_SPIN_HIT", "SE_SY_GAME_START", 0);
            assert(aurora::audio::DisabledObjectAudio::declined_requests() == requests + 2);
            {
                auto nested = aurora::audio::make_disabled_object_audio_service(heaps, &playback);
                assert(AudSingletonHolder<AudSoundNameConverter>::get() != converter);
            }
            assert(AudSingletonHolder<AudSoundNameConverter>::get() == converter);
            assert(JAUSoundNameTable::getInstance() == table);
            assert(u32(converter->getSoundID("SE_SY_GAME_START")) == playback.find_sound_id("SE_SY_GAME_START").value());
            // A failing original-table setup must leave the active process's
            // converter and table published, with all provisional heap bytes reclaimed.
            auto corrupt = baa;
            constexpr std::array<u8, 4> magic = {'B', 'S', 'T', 'N'};
            const auto at = std::search(corrupt.begin(), corrupt.end(), magic.begin(), magic.end());
            assert(at != corrupt.end());
            const auto root_field = static_cast<std::size_t>(at - corrupt.begin()) + 12;
            for (int i = 0; i != 4; ++i) corrupt[root_field + i] = 0xff;
            smgpc::runtime::JAudioPlaybackService invalid(
                [&] { return std::make_unique<aurora::audio::JAudioSoundArchive>(corrupt, wave); }, wave,
                std::make_unique<aurora::audio::PcmAudioMixer>());
            const auto before_failure = heaps->root_heap().getFreeSize();
            bool rejected = false;
            try { auto failed = aurora::audio::make_disabled_object_audio_service(heaps, &invalid); }
            catch (const std::runtime_error&) { rejected = true; }
            assert(rejected && heaps->root_heap().getFreeSize() == before_failure);
            assert(AudSingletonHolder<AudSoundNameConverter>::get() == converter);
            assert(JAUSoundNameTable::getInstance() == table);
            assert(!invalid.is_device_open());
        }
        assert(AudSingletonHolder<AudSoundNameConverter>::get() == previous);
        assert(JAUSoundNameTable::getInstance() == previous_table);
        assert(heaps->root_heap().getFreeSize() == free_bytes);
    }
    assert(!playback.is_device_open() && wave_requests == 0);
    std::puts("[pass] original name tables: retail IDs, disabled speaker fallback, nested publication, failed-init rollback, three heap retirements; no wave/device access");
}

void test_disabled_backend_wrapper_owner() {
    const auto baa = smgpc::resource::decompress_yaz0(read(fixture / "KrKorean/AudioRes/SMR.szs"));
    const aurora::os::GuestThreadExecutionScope execution;
    const auto heaps = smgpc::compat::JkrHeapRuntime::create(2U * 1024U * 1024U);
    const auto initial_free = heaps->root_heap().getFreeSize();
    auto* previous_names = AudSingletonHolder<AudSoundNameConverter>::get();
    for (int cycle = 0; cycle != 3; ++cycle) {
        auto domain = smgpc::compat::JkrAllocationDomain::create(heaps, 512U * 1024U);
        AudSystemWrapper* wrapper;
        {
            smgpc::compat::JkrAllocationScope allocations(domain);
            wrapper = new AudSystemWrapper(static_cast<JKRSolidHeap*>(&domain->heap()), &heaps->root_heap());
        }
        assert(wrapper->mAudSystem == nullptr);
        assert(!wrapper->isLoadDoneWaveDataAtSystemInit());
        assert(!wrapper->isLoadDoneStaticWaveData());
        wrapper->loadStaticWaveData();
        assert(!wrapper->isLoadDoneStaticWaveData());
        wrapper->prepareReset();
        assert(wrapper->_29 && wrapper->isResetDone() && wrapper->isPermitToReset());
        wrapper->resumeReset();
        assert(!wrapper->_29);
        auto& backend = *wrapper->mDisabledBackend;
        assert(backend.scene_manager() == nullptr);
        backend.request_initialize();
        backend.receive_initialize();
        struct Work { smgpc::compat::DisabledAudioBackend* output; const std::vector<u8>* baa; } work{&backend, &baa};
        OSThread thread{};
        alignas(32) std::array<u8, 64U * 1024U> stack{};
        const auto initialize = [](void* argument) -> void* {
            auto& work = *static_cast<Work*>(argument);
            work.output->initialize(*work.baa);
            return argument;
        };
        assert(OSCreateThread(&thread, initialize, &work, stack.data() + stack.size(), stack.size(), 14, 0));
        OSResumeThread(&thread);
        void* result = nullptr;
        assert(OSJoinThread(&thread, &result) && result == &work);
        assert(wrapper->isLoadDoneWaveDataAtSystemInit());
        assert(wrapper->mAudSystem == nullptr && !backend.has_output_device());
        assert(AudWrap::getSceneMgr() == backend.scene_manager());
        // Volume ownership was constructed on the original initialization
        // worker; the main guest controls and advances that same real state.
        auto* output = aurora::audio::disabled_object_audio_service();
        assert(output != nullptr);
        const auto limited_id = AudSingletonHolder<AudSoundNameConverter>::get()->getSoundID("SE_SM_LV_TICO_WAIT");
        assert(!output->is_limited_sound(limited_id));
        MR::limitedSound("SE_SM_LV_TICO_WAIT", 1);
        assert(output->is_limited_sound(limited_id));
        wrapper->movement();
        assert(!output->is_limited_sound(limited_id));
        MR::setSoundVolumeSetting(3, 2);
        wrapper->movement();
        assert(std::abs(output->sound_category_gain(0x60000) -
            (AudParams::scCtgVolume[0][6] + AudParams::scCtgVolume[3][6]) * 0.5f) < 0.00001f);
        wrapper->movement();
        assert(output->sound_category_gain(0x60000) == AudParams::scCtgVolume[3][6]);
        MR::recoverSoundVolumeSetting(0);
        assert(output->sound_category_gain(0x60000) == AudParams::scCtgVolume[0][6]);
        MR::setCubeBgmChangeInvalid();
        assert(MR::isCubeBgmChangeInvalid());
        AudWrap::getSceneMgr()->startScene();
        assert(!MR::isCubeBgmChangeInvalid());
        // Main guest sees the actual service published by the original OS worker.
        assert(aurora::audio::disabled_system_sound_object() != nullptr);
        assert(u32(AudSingletonHolder<AudSoundNameConverter>::get()->getSoundID("SE_SY_GAME_START")) == SE_SY_GAME_START);
        const auto requests = aurora::audio::DisabledObjectAudio::declined_requests();
        MR::startSystemSE("SE_SY_GAME_START", -1, -1);
        assert(aurora::audio::DisabledObjectAudio::declined_requests() == requests + 1);
        wrapper->loadStaticWaveData();
        assert(wrapper->isLoadDoneStaticWaveData());
        wrapper->loadStageWaveData("Game", "AnyStage", false);
        assert(backend.scene_manager()->isPlayerModeMario());
        assert(wrapper->isLoadDoneStageWaveData() && !wrapper->isLoadDoneScenarioWaveData());
        wrapper->loadScenarioWaveData("Game", "AnyStage", 2);
        assert(wrapper->isLoadDoneScenarioWaveData());
        wrapper->loadStageWaveData("Game", "AnotherStage", true);
        assert(backend.scene_manager()->isPlayerModeLuigi());
        assert(!wrapper->isLoadDoneScenarioWaveData());
        wrapper->requestReset(false);
        assert(wrapper->isResetDone());
        wrapper->resumeReset();
        assert(!wrapper->isResetDone());
        // No fabricated manual Game teardown: actual heap finalization must
        // destroy its attached native owner before the arena storage is reused.
        domain.reset();
        assert(AudSingletonHolder<AudSoundNameConverter>::get() == previous_names);
        assert(aurora::audio::disabled_system_sound_object() == nullptr);
        assert(aurora::audio::disabled_audio_scene_manager() == nullptr);
        assert(heaps->root_heap().getFreeSize() == initial_free);
    }
    std::puts("[pass] actual wrapper heap lifetime and OS-worker disabled backend: readiness, named requests, bank/reset state, three retirements; no AudSystem/device");
}

int main(int argc, char** argv) {
    if (argc == 2 && std::strcmp(argv[1], "--scene-only") == 0) { test_disabled_audio_scene_state(); return 0; }
    if (argc == 2 && std::strcmp(argv[1], "--backend-only") == 0) { test_disabled_backend_wrapper_owner(); return 0; }
    if (argc == 2 && std::strcmp(argv[1], "--names-only") == 0) { test_original_name_owner(); return 0; }
    test_disabled_audio_scene_state();
    test_original_name_owner();
    test_original_talk_sound_dispatch();
    test_native_and_stream_owners();
    test_retail_service();
}
