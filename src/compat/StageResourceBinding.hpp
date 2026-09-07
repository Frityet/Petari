#pragma once

#include <memory>
#include <span>
#include <revolution/types.h>

class JMapIdInfo;
class JMapInfo;
class JMapInfoIter;

namespace smgpc::runtime {
    class DvdFileSystemService;
}

namespace smgpc::scene {
    struct StageHolderOccurrence;
    struct StagePlacementTable;
}

namespace smgpc::compat {
    // Stage archive identities and decoded JMap strings outlive temporary
    // original readers and remain borrowed until the scene owners retire.
    class StageResourceBinding final {
    public:
        StageResourceBinding(runtime::DvdFileSystemService& dvd,
                             std::span<const scene::StageHolderOccurrence> holders,
                             std::span<const scene::StagePlacementTable> tables);
        ~StageResourceBinding();

        StageResourceBinding(const StageResourceBinding&) = delete;
        StageResourceBinding& operator=(const StageResourceBinding&) = delete;

        void camera_data(void** data, s32* size, s32 zone_id);
        [[nodiscard]] s32 start_count() const;
        void start_camera_id(JMapIdInfo* output, int index) const;
        [[nodiscard]] s32 rail_count(s32 zone_id) const;
        void rail_by_id(JMapInfoIter* path, const JMapInfo** points, s32 rail_id, s32 zone_id) const;
        [[nodiscard]] bool camera_rail_by_index(JMapInfoIter* path, const JMapInfo** points, int index, s32 zone_id) const;
        [[nodiscard]] s32 start_zone(const JMapIdInfo& start_id) const;
        [[nodiscard]] s32 start_camera(const JMapIdInfo& start_id) const;
        void scenario_start_camera(void** data, s32* size, s32 scenario_no);

    private:
        struct State;
        std::unique_ptr<State> _state;
        StageResourceBinding* _previous = nullptr;
    };

    [[nodiscard]] StageResourceBinding& require_stage_resources();
}
