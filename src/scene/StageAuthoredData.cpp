#include "scene/StageAuthoredData.hpp"

#include "runtime/RuntimeServices.hpp"

#include <stdexcept>
#include <utility>

namespace smgpc::scene {

    StageAuthoredData::StageAuthoredData(
        std::string stage_name, s32 scenario_no,
        std::vector<StagePlacementTable> tables,
        std::vector<StagePlacementObject> placements,
        std::vector<StageGeneralPos> general_positions,
        std::optional<StageStartInfo> start_info,
        std::vector<StageHolderOccurrence> holders)
        : _stage_name(std::move(stage_name)), _scenario_no(scenario_no),
          _tables(std::move(tables)), _holders(std::move(holders)), _placements(std::move(placements)),
          _general_positions(std::move(general_positions)),
          _start_info(std::move(start_info)) {
    }

    StageAuthoredData StageAuthoredData::resolve(
        smgpc::runtime::DvdFileSystemService &dvd,
        std::string_view stage_name, s32 scenario_no, s32 start_id,
        s32 start_zone_id) {
        auto holders = std::vector<StageHolderOccurrence>{};
        auto tables = resolve_stage_placement_tables(dvd, stage_name, scenario_no, &holders);
        auto placements = resolve_stage_placement_objects(dvd, tables);
        auto general_positions = select_stage_general_positions(tables);
        auto start_info =
            select_stage_start_info(tables, start_id, start_zone_id);
        return StageAuthoredData(
            std::string(stage_name), scenario_no, std::move(tables),
            std::move(placements), std::move(general_positions),
            std::move(start_info), std::move(holders));
    }

    std::string_view StageAuthoredData::stage_name() const noexcept {
        return _stage_name;
    }

    s32 StageAuthoredData::scenario_no() const noexcept {
        return _scenario_no;
    }

    std::span<const StagePlacementTable>
    StageAuthoredData::tables() const noexcept {
        return _tables;
    }

    std::span<const StageHolderOccurrence>
    StageAuthoredData::holders() const noexcept {
        return _holders;
    }

    std::span<const StagePlacementObject>
    StageAuthoredData::placements() const noexcept {
        return _placements;
    }

    std::span<const StageGeneralPos>
    StageAuthoredData::general_positions() const noexcept {
        return _general_positions;
    }

    const std::optional<StageStartInfo> &
    StageAuthoredData::start_info() const noexcept {
        return _start_info;
    }

    NameObjPlacementContext StageAuthoredData::placement_context(
        std::size_t placement_index) const {
        if (placement_index >= _placements.size()) {
            throw std::out_of_range(
                "Authored placement index is outside the retained stage data.");
        }

        const auto &placement = _placements[placement_index];
        const auto iter = JMapInfoIter(
            &placement.jmap_info, placement.jmap_entry_index);
        if (!iter.isValid()) {
            throw std::logic_error(
                "Authored placement does not retain a valid JMap row.");
        }
        return NameObjPlacementContext{
            .iter = iter,
            .source = NameObjPlacementSource::StagePlacement,
            .stage_name = placement.stage_name,
            .zone_name = placement.zone_name,
            .table_path = placement.table_path,
            .row = placement.jmap_entry_index,
            .local_id = placement.l_id,
        };
    }

    NameObjPlacementContext StageAuthoredData::start_context() const {
        if (!_start_info.has_value()) {
            throw std::logic_error(
                "The retained stage data has no selected StartInfo row.");
        }

        const auto &start = *_start_info;
        const auto iter = start.iter();
        if (!iter.isValid()) {
            throw std::logic_error(
                "The retained stage StartInfo does not contain a valid JMap row.");
        }
        return NameObjPlacementContext{
            .iter = iter,
            .source = NameObjPlacementSource::StageStart,
            .stage_name = start.stage_name,
            .zone_name = start.zone_name,
            .table_path = start.table_path,
            .row = start.jmap_entry_index,
            .local_id = start.start_id,
        };
    }

}  // namespace smgpc::scene
