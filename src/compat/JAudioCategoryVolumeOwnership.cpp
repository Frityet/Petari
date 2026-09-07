#include "compat/JAudioCategoryVolumeOwnership.hpp"

#include "Game/AudioLib/AudParams.hpp"
#include <JSystem/JAudio2/JAISound.hpp>
#include <aurora/exception.hpp>

#include <stdexcept>

namespace smgpc::compat {

    thread_local JAudioCategoryVolumeOwnership *JAudioCategoryVolumeOwnership::_head = nullptr;

    JAudioCategoryVolumeOwnership::JAudioCategoryVolumeOwnership()
        : _controller(nullptr), _next(_head) {
        _head = this;
        reset();
    }

    JAudioCategoryVolumeOwnership::~JAudioCategoryVolumeOwnership() {
        auto **entry = &_head;
        while (*entry != nullptr && *entry != this) {
            entry = &(*entry)->_next;
        }
        if (*entry == this) {
            *entry = _next;
        }
    }

    void JAudioCategoryVolumeOwnership::reset() {
        for (auto &category : _categories) {
            category.init();
        }
        _controller.init();
        _controller.mVolumeSetDelay = -1;
        // AudSystem::initVolumeSetting applies preset zero after controller init.
        _controller.setSeVolumeSetTrig(0, 0);
    }

    void JAudioCategoryVolumeOwnership::update() {
        _controller.update();
        for (auto &category : _categories) {
            category.calc();
        }
    }

    AudSystemVolumeController &JAudioCategoryVolumeOwnership::controller() {
        return _controller;
    }

    const AudSystemVolumeController &JAudioCategoryVolumeOwnership::controller() const {
        return _controller;
    }

    float JAudioCategoryVolumeOwnership::sound_gain(std::uint32_t sound_id) const {
        // Retail JAUStdSoundInfo::getCategory reads JAISoundID's group byte.
        return category(JAISoundID(sound_id).getGroupID()).mParams.mVolume;
    }

    const JAISoundParamsMove &JAudioCategoryVolumeOwnership::category(std::size_t index) const {
        if (index >= _categories.size()) {
            aurora::throw_host_exception<std::out_of_range>("JAudio sound category is outside the sixteen SE categories");
        }
        return _categories[index];
    }

    void JAudioCategoryVolumeOwnership::set_inner(std::int32_t volume_set, std::uint32_t steps) {
        if (volume_set < 0 || volume_set >= 8) {
            aurora::throw_host_exception<std::out_of_range>("JAudio volume preset is outside the original preset table");
        }
        for (std::size_t index = 0; index < _categories.size(); ++index) {
            _categories[index].moveVolume(AudParams::scCtgVolume[volume_set][index], steps);
        }
    }

    JAudioCategoryVolumeOwnership &JAudioCategoryVolumeOwnership::require(
        const AudSystemVolumeController *controller) {
        for (auto *owner = _head; owner != nullptr; owner = owner->_next) {
            if (&owner->_controller == controller) {
                return *owner;
            }
        }
        aurora::throw_host_exception<std::logic_error>(
            "AudSystemVolumeController has no retained native category owner");
    }

}  // namespace smgpc::compat
