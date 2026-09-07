#pragma once

#include <memory>
#include <span>
#include <revolution/types.h>

namespace smgpc::runtime {
    class DvdFileSystemService;
}

namespace smgpc::scene {
    struct StageHolderOccurrence;
}

namespace smgpc::compat {
    // Stage archive identities and decoded JMap strings outlive temporary
    // original readers and remain borrowed until the scene owners retire.
    class StageResourceBinding final {
    public:
        StageResourceBinding(runtime::DvdFileSystemService& dvd,
                             std::span<const scene::StageHolderOccurrence> holders);
        ~StageResourceBinding();

        StageResourceBinding(const StageResourceBinding&) = delete;
        StageResourceBinding& operator=(const StageResourceBinding&) = delete;

        void camera_data(void** data, s32* size, s32 zone_id);

    private:
        struct State;
        std::unique_ptr<State> _state;
        StageResourceBinding* _previous = nullptr;
    };

    [[nodiscard]] StageResourceBinding& require_stage_resources();
}
