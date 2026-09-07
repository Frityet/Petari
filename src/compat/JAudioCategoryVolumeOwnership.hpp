#pragma once

#include "Game/AudioLib/AudSystemVolumeController.hpp"
#include <JSystem/JAudio2/JAISoundParams.hpp>

#include <array>
#include <cstdint>

namespace smgpc::compat {

    // Retains the original preset controller and the sixteen category parameter
    // records which its AudSystem/JAISeMgr backend owns on Wii.
    class JAudioCategoryVolumeOwnership final {
    public:
        JAudioCategoryVolumeOwnership();
        ~JAudioCategoryVolumeOwnership();
        JAudioCategoryVolumeOwnership(const JAudioCategoryVolumeOwnership &) = delete;
        JAudioCategoryVolumeOwnership &operator=(const JAudioCategoryVolumeOwnership &) = delete;

        void reset();
        void update();
        [[nodiscard]] AudSystemVolumeController &controller();
        [[nodiscard]] const AudSystemVolumeController &controller() const;
        [[nodiscard]] float sound_gain(std::uint32_t sound_id) const;
        [[nodiscard]] const JAISoundParamsMove &category(std::size_t index) const;
        void set_inner(std::int32_t volume_set, std::uint32_t steps);
        [[nodiscard]] static JAudioCategoryVolumeOwnership &require(
            const AudSystemVolumeController *controller);

    private:
        AudSystemVolumeController _controller;
        std::array<JAISoundParamsMove, 16> _categories;
        JAudioCategoryVolumeOwnership *_next = nullptr;
        static thread_local JAudioCategoryVolumeOwnership *_head;
    };

}  // namespace smgpc::compat
