#include "compat/DisabledObjectAudioService.hpp"
#include "Game/AudioLib/AudSoundNameConverter.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "JSystem/JAudio2/JAUSoundTable.hpp"
#include "JSystem/JKernel/JKRSolidHeap.hpp"
#include "runtime/JAudioPlaybackService.hpp"
#include <aurora/exception.hpp>
#include <array>
#include <limits>
#include <stdexcept>
#include <utility>

namespace aurora::audio {
namespace {
// Original audio owners are process singletons. All publication/retirement
// occurs under the guest CPU gate, including original audio initialization workers.
DisabledObjectAudioService* active_service = nullptr;
}
// The native byte owner and actual SDK table precede the original constructor.
// Its allocations belong to an actual retained JKR heap; constructor failure
// therefore reclaims the original arrays without a partial singleton escaping.
class OriginalAudioNameLifetime final {
public:
    OriginalAudioNameLifetime(std::shared_ptr<smgpc::compat::JkrHeapRuntime> heaps,
                              smgpc::runtime::JAudioPlaybackService& playback)
        : _bytes(playback.native_sound_name_table()), _table(false) {
        const auto budget = validate_table();
        _domain = smgpc::compat::JkrAllocationDomain::create(std::move(heaps), budget);
        smgpc::compat::JkrAllocationScope guest(_domain);
        construct_converter();
    }
    OriginalAudioNameLifetime(JKRHeap& heap, std::vector<std::uint8_t> native_names)
        : _bytes(std::move(native_names)), _table(false) {
        validate_table();
        // This owner is attached to the same original heap's finalizer. It
        // must not retain that heap through a lease back to itself.
        MR::CurrentHeapRestorer current(&heap);
        aurora::allocation::ClientAllocationScope guest({true, true});
        construct_converter();
    }
    ~OriginalAudioNameLifetime() {
        AudSingletonHolder<AudSoundNameConverter>::exchange(_previous_converter);
        JAUSoundNameTable::sInstance = _previous_table;
        delete[] _converter->mSoundNameData;
        delete[] _converter->mGroupItemOffsets;
        delete _converter;
    }
private:
    std::size_t validate_table() {
        _table.init(_bytes.data());
        std::size_t items = 0;
        constexpr std::array<int, 3> groups = {14, 2, 1};
        if (_table.mTable.mRoot->mSectionNumber != groups.size()) invalid_table();
        for (u8 section = 0; section < groups.size(); ++section) {
            if (_table.getNumGroups_inSection(section) != groups[section]) invalid_table();
            for (u8 group = 0; group < groups[section]; ++group) {
                const int count = _table.getNumItems_inGroup(section, group);
                // The original converter's loop index is u16 and hashes every
                // named entry. Reject unsupported resources before construction.
                if (count < 0 || count >= 65536) invalid_table();
                items += static_cast<std::size_t>(count);
                for (int item = 0; item < count; ++item) {
                    JAISoundID id;
                    id.set(section, group, item);
                    if (_table.getName(id) == nullptr) invalid_table();
                }
            }
        }
        if (items > static_cast<std::size_t>(std::numeric_limits<s32>::max())) invalid_table();
        // Heap header plus the constructor's object and two arrays, each
        // conservatively rounded to the SDK heap's 32-byte boundary.
        const auto aligned = [](std::size_t bytes) { return (bytes + 31U) & ~std::size_t(31U); };
        const auto budget = aligned(sizeof(JKRSolidHeap)) + aligned(sizeof(AudSoundNameConverter)) +
                            aligned(17U * sizeof(u32)) + aligned(items * sizeof(AudSoundNameData));
        return budget;
    }
    void construct_converter() {
        _previous_table = JAUSoundNameTable::sInstance;
        JAUSoundNameTable::sInstance = &_table;
        try {
            _converter = new AudSoundNameConverter;
        } catch (...) {
            JAUSoundNameTable::sInstance = _previous_table;
            throw;
        }
        _previous_converter = AudSingletonHolder<AudSoundNameConverter>::exchange(_converter);
    }
    [[noreturn]] static void invalid_table() {
        aurora::throw_host_exception<std::runtime_error>(
            "The sound-name resource does not satisfy the original converter's category/count contract");
    }
    std::vector<std::uint8_t> _bytes;
    std::shared_ptr<smgpc::compat::JkrAllocationDomain> _domain;
    JAUSoundNameTable _table;
    JAUSoundNameTable* _previous_table = nullptr;
    AudSoundNameConverter* _converter = nullptr;
    AudSoundNameConverter* _previous_converter = nullptr;
};

DisabledObjectAudioService::DisabledObjectAudioService(std::shared_ptr<smgpc::compat::JkrHeapRuntime> heaps,
                                                       smgpc::runtime::JAudioPlaybackService* playback)
    : _heaps(std::move(heaps)), _system_object(nullptr, 0, &_heaps->root_heap()), _playback(playback),
      _names(playback ? std::make_unique<OriginalAudioNameLifetime>(_heaps, *playback) : nullptr),
      _previous(active_service) {
    active_service = this;
}
DisabledObjectAudioService::DisabledObjectAudioService(JKRHeap& heap, std::vector<std::uint8_t> native_names)
    : _system_object(nullptr, 0, &heap),
      _names(std::make_unique<OriginalAudioNameLifetime>(heap, std::move(native_names))),
      _previous(active_service) {
    active_service = this;
}
DisabledObjectAudioService::~DisabledObjectAudioService() {
    active_service = _previous;
}
void DisabledObjectAudioService::set_trigger_sound_permitted(bool permitted) {
    _trigger_sound_permitted = permitted;
    if (_playback) _playback->set_trigger_sound_permitted(permitted);
}
void DisabledObjectAudioService::set_level_sound_permitted(bool permitted) {
    _level_sound_permitted = permitted;
    if (_playback) _playback->set_level_sound_permitted(permitted);
}
bool DisabledObjectAudioService::is_sound_permitted() const {
    return _playback ? _playback->is_sound_permitted() : _trigger_sound_permitted && _level_sound_permitted;
}
void DisabledObjectAudioService::set_sound_volume_setting(s32 volume_set, u32 steps) {
    if (_playback) _playback->set_sound_volume_setting(volume_set, steps);
    else _volumes.controller().setSeVolumeSetTrig(volume_set, steps);
}
void DisabledObjectAudioService::recover_sound_volume_setting(u32 steps) {
    if (_playback) _playback->recover_sound_volume_setting(steps);
    else _volumes.controller().recoverSeVolumeSet(steps);
}
float DisabledObjectAudioService::sound_category_gain(u32 sound_id) const {
    return _playback ? _playback->sound_category_gain(sound_id) : _volumes.sound_gain(sound_id);
}
void DisabledObjectAudioService::register_limited_sound(JAISoundID sound_id, s32 delay) {
    // With disabled output there is no playing voice to stop. A concrete
    // playback owner performs the original stop-by-ID before registration.
    if (_playback) _playback->register_limited_sound(sound_id, delay);
    else _limited_sounds.register_sound(sound_id, delay);
}
bool DisabledObjectAudioService::is_limited_sound(JAISoundID sound_id) const {
    return _playback ? _playback->is_limited_sound(sound_id) : _limited_sounds.contains(sound_id);
}
void DisabledObjectAudioService::reset_scene_controls() {
    _volumes.reset();
    _trigger_sound_permitted = true;
    _level_sound_permitted = true;
    if (_playback) _playback->reset_output_controls();
}
void DisabledObjectAudioService::update_scene_controls() {
    // RuntimeContext advances its concrete PCM backend at its own frame edge.
    if (!_playback) {
        _volumes.update();
        _limited_sounds.update();
    }
}
std::unique_ptr<DisabledObjectAudioService> make_disabled_object_audio_service(
    std::shared_ptr<smgpc::compat::JkrHeapRuntime> heaps,
    smgpc::runtime::JAudioPlaybackService* playback) {
    smgpc::compat::JkrHostAllocationScope host;
    return std::make_unique<DisabledObjectAudioService>(std::move(heaps), playback);
}
AudSoundObject* disabled_system_sound_object() noexcept {
    return active_service == nullptr ? nullptr : active_service->system_object();
}
AudSceneMgr* disabled_audio_scene_manager() noexcept {
    return active_service == nullptr ? nullptr : active_service->scene_manager();
}
DisabledObjectAudioService* disabled_object_audio_service() noexcept { return active_service; }
}

// Original state construction and player selection from AudSceneMgr.cpp. The
// disabled backend owns this object; its deliberately absent wave heap is never
// exposed as a loaded hardware resource.
AudSceneMgr::AudSceneMgr(JAUSectionHeap* pSectionHeap)
    : _4(), mSeWaveSetId(), mSeScenarioWaveSetId(-1), mBgmWaveSetId(), mPlayerMode(), mPrevPlayerMode(), mIsNewPlayerMode(), _1D() {
    mSectionHeap = pSectionHeap;
}

void AudSceneMgr::setPlayerModeMario() {
    mPlayerMode = PlayerMode_Mario;
}

void AudSceneMgr::setPlayerModeLuigi() {
    mPlayerMode = PlayerMode_Luigi;
}

void AudSceneMgr::startScene() {
    if (aurora::audio::disabled_audio_scene_manager() != this)
        aurora::throw_host_exception<std::logic_error>("Audio scene start requires its active disabled backend owner");
    // Keep the original scene-local flags and actual native output controls.
    // Voice/effect owners and Wii-speaker reconnection remain absent under
    // the explicit disabled output policy.
    _4 = 0;
    _1D = false;
    aurora::audio::disabled_object_audio_service()->reset_scene_controls();
}
