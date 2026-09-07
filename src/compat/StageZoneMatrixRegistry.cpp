#include "compat/StageZoneMatrixRegistry.hpp"

#include "Game/Util/SceneUtil.hpp"

#include <exception>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
    thread_local smgpc::compat::StageZoneMatrixBinding* s_active_binding = nullptr;
}

namespace smgpc::compat {
    StageZoneMatrixRegistry::StageZoneMatrixRegistry(
        std::span<const scene::StageHolderOccurrence> holders,
        std::span<const scene::StagePlacementTable> tables) {
        _holders.reserve(holders.size());
        for (const auto& source : holders) {
            if (source.instance_id != _holders.size()) {
                throw std::invalid_argument("Zone matrices require the retained holder occurrence order.");
            }
            if (!source.parent_instance_id.has_value()) {
                if (_root.has_value() || source.zone_id != 0) {
                    throw std::invalid_argument("Zone matrices require one root holder with zone ID zero.");
                }
                _root = source.instance_id;
            }
            auto holder = Holder{.zone_id = source.zone_id, .children = source.children};
            for (auto row = std::size_t{}; row < 3U; ++row) {
                for (auto column = std::size_t{}; column < 4U; ++column) {
                    holder.matrix.mMtx[row][column] = source.zone_transform.matrix[row * 4U + column];
                }
            }
            _holders.push_back(std::move(holder));
        }
        if (!_holders.empty() && !_root.has_value()) {
            throw std::invalid_argument("Zone matrix holder data has no root occurrence.");
        }
        for (const auto& source : holders) {
            for (const auto child : source.children) {
                if (child >= holders.size() || holders[child].parent_instance_id != source.instance_id) {
                    throw std::invalid_argument("Zone matrix holder data has inconsistent child ownership.");
                }
            }
        }
        for (const auto& table : tables) {
            if (table.holder_instance_id >= _holders.size() ||
                _holders[table.holder_instance_id].zone_id != table.zone_id) {
                throw std::invalid_argument("A zone placement table has no matching retained holder.");
            }
            if (table.jmap_info.mData == nullptr) {
                continue;
            }
            const auto [entry, inserted] = _table_holders.emplace(table.jmap_info.mData.get(), table.holder_instance_id);
            if (!inserted && entry->second != table.holder_instance_id) {
                throw std::invalid_argument("One JMap data owner cannot belong to different holder occurrences.");
            }
        }
    }

    TPos3f* StageZoneMatrixRegistry::matrix_for_zone(s32 zone_id) {
        if (!_root.has_value()) {
            throw std::logic_error("The active stage has no retained zone placement holders.");
        }
        auto& root = _holders[*_root];
        if (zone_id == 0) {
            return &root.matrix;
        }
        // StageDataHolder::getStageDataHolderFromZoneId searches immediate
        // children in occurrence order; it does not recurse or deduplicate.
        for (const auto child : root.children) {
            if (_holders[child].zone_id == zone_id) {
                return &_holders[child].matrix;
            }
        }
        throw std::out_of_range("The active root stage has no placed child zone " + std::to_string(zone_id) + '.');
    }

    TPos3f* StageZoneMatrixRegistry::matrix_for_iter(const JMapInfoIter& iter) {
        if (!iter.isValid() || iter.mInfo->mData == nullptr) {
            throw std::invalid_argument("Zone matrix lookup requires a valid retained JMap row.");
        }
        const auto holder = _table_holders.find(iter.mInfo->mData.get());
        if (holder == _table_holders.end()) {
            throw std::out_of_range("The JMap row does not belong to the active stage's holder data.");
        }
        return &_holders[holder->second].matrix;
    }

    StageZoneMatrixBinding::StageZoneMatrixBinding(
        std::span<const scene::StageHolderOccurrence> holders,
        std::span<const scene::StagePlacementTable> tables)
        : _registry(holders, tables), _previous(s_active_binding) {
        s_active_binding = this;
    }

    StageZoneMatrixBinding::~StageZoneMatrixBinding() {
        if (s_active_binding != this) {
            std::terminate();
        }
        s_active_binding = _previous;
    }

    StageZoneMatrixRegistry& StageZoneMatrixBinding::registry() {
        return _registry;
    }

    StageZoneMatrixRegistry& require_stage_zone_matrices() {
        if (s_active_binding == nullptr) {
            throw std::logic_error("Zone placement matrices require an active stage lifetime.");
        }
        return s_active_binding->registry();
    }
}

namespace MR {
    TPos3f* getZonePlacementMtx(const JMapInfoIter& iter) {
        return smgpc::compat::require_stage_zone_matrices().matrix_for_iter(iter);
    }

    TPos3f* getZonePlacementMtx(s32 zone_id) {
        return smgpc::compat::require_stage_zone_matrices().matrix_for_zone(zone_id);
    }
}
