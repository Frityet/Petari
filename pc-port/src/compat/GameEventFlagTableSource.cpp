#include "Game/System/GameEventFlagTable.hpp"
#include "Game/Util/HashUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "Game/Util/StringUtil.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <stdexcept>

// The retail flag table itself is real source data and is useful without the
// embedded GalaxyID BCSV.  Fence only the one API which needs that unavailable
// database so it cannot silently answer from an empty replacement.
struct JMapData final {};

namespace smgpc::compat::story_sequence_detail {
    [[noreturn]] inline void unavailable_galaxy_id_database() {
        throw std::runtime_error("GameEventFlagTable galaxy dependency queries require the real GalaxyID BCSV");
    }

    class UnavailableGalaxyIdIter final {
    public:
        template <typename T>
        bool getValue(const char*, T*) const {
            unavailable_galaxy_id_database();
        }
    };

    class UnavailableGalaxyIdInfo final {
    public:
        bool attach(const void*) {
            unavailable_galaxy_id_database();
        }

        template <typename T>
        UnavailableGalaxyIdIter findElement(const char*, T, int) const {
            unavailable_galaxy_id_database();
        }

        s32 searchItemInfo(const char*) const {
            unavailable_galaxy_id_database();
        }
    };
}  // namespace smgpc::compat::story_sequence_detail

extern const JMapData GalaxyIDBCSV{};

#define JMapInfo smgpc::compat::story_sequence_detail::UnavailableGalaxyIdInfo
#define JMapInfoIter smgpc::compat::story_sequence_detail::UnavailableGalaxyIdIter
#include "../../../src/Game/System/GameEventFlagTable.cpp"
#undef JMapInfoIter
#undef JMapInfo
