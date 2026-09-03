#include "compat/DisabledObjectAudioService.hpp"
#include <utility>

namespace aurora::audio {
namespace {
thread_local DisabledObjectAudioService* active_service = nullptr;
}
DisabledObjectAudioService::DisabledObjectAudioService(std::shared_ptr<smgpc::compat::JkrHeapRuntime> heaps)
    : _heaps(std::move(heaps)), _system_object(nullptr, 0, &_heaps->root_heap()), _previous(active_service) {
    active_service = this;
}
DisabledObjectAudioService::~DisabledObjectAudioService() {
    active_service = _previous;
}
std::unique_ptr<DisabledObjectAudioService> make_disabled_object_audio_service(
    std::shared_ptr<smgpc::compat::JkrHeapRuntime> heaps) {
    smgpc::compat::JkrHostAllocationScope host;
    return std::make_unique<DisabledObjectAudioService>(std::move(heaps));
}
AudSoundObject* disabled_system_sound_object() noexcept {
    return active_service == nullptr ? nullptr : active_service->system_object();
}
}
