#include "resource/EmbeddedGameTables.hpp"
#include <revolution/types.h>

extern const u8 GalaxyIDBCSV[0xD20];

namespace smgpc::resource {
    EmbeddedGameTables::EmbeddedGameTables()
        : _galaxy_id(std::span{GalaxyIDBCSV}),
          _galaxy_id_alias(_galaxy_id.register_source(std::span{GalaxyIDBCSV})) {
    }
}
