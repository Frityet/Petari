#include "compat/DisabledAudioBackend.hpp"
#include "compat/DisabledObjectAudio.hpp"
#include "compat/DisabledObjectAudioService.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include <aurora/exception.hpp>
#include <aurora/j_audio_sound_archive.hpp>
#include <stdexcept>
#include <vector>

namespace smgpc::compat {
DisabledAudioBackend::DisabledAudioBackend(JKRHeap& heap) : _owner_heap(heap) {
    static_assert(!aurora::audio::DisabledObjectAudio::enabled(),
                  "The disabled backend requires the shared explicit disabled-output policy");
}
DisabledAudioBackend::~DisabledAudioBackend() = default;

void DisabledAudioBackend::request_initialize() {
    if (_phase != Phase::Created)
        aurora::throw_host_exception<std::logic_error>("Disabled audio initialization was already requested");
    _phase = Phase::Requested;
}
void DisabledAudioBackend::receive_initialize() {
    if (_phase != Phase::Requested)
        aurora::throw_host_exception<std::logic_error>("Disabled audio received resources without its request");
    _phase = Phase::Received;
}
void DisabledAudioBackend::initialize(std::span<const std::uint8_t> baa) {
    if (_phase != Phase::Received)
        aurora::throw_host_exception<std::logic_error>("Disabled audio initialization requires its received name resource");
    JkrHostAllocationScope host;
    aurora::audio::JAudioSoundArchive archive(baa, [](std::string_view) -> std::vector<std::uint8_t> {
        aurora::throw_host_exception<std::logic_error>("The disabled audio backend does not load wave banks");
    });
    auto objects = std::make_unique<aurora::audio::DisabledObjectAudioService>(
        _owner_heap, archive.native_sound_name_table());
    _objects = std::move(objects);
    _phase = Phase::Initialized;
}
bool DisabledAudioBackend::initialized() const noexcept { return _phase == Phase::Initialized; }
void DisabledAudioBackend::request_banks(BankGroup group) {
    if (!initialized()) return;
    // A new stage invalidates the previous scenario's bank request even though
    // neither request needs physical bank storage under this output policy.
    if (group == BankGroup::Stage) _banks[static_cast<std::size_t>(BankGroup::Scenario)] = false;
    _banks[static_cast<std::size_t>(group)] = true;
}
bool DisabledAudioBackend::banks_complete(BankGroup group) const noexcept {
    return initialized() && _banks[static_cast<std::size_t>(group)];
}
void DisabledAudioBackend::prepare_reset() noexcept { _reset_requested = true; }
void DisabledAudioBackend::request_reset() noexcept { _reset_requested = true; }
bool DisabledAudioBackend::reset_complete() const noexcept {
    // There is no output worker or queued voice to acknowledge under this policy.
    return _reset_requested || !initialized();
}
void DisabledAudioBackend::resume_reset() noexcept { _reset_requested = false; }
void DisabledAudioBackend::stop_all() noexcept {
    // The backend owns no voices. No audible/hardware completion is asserted.
}
}
