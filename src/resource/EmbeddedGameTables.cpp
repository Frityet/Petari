#include "resource/EmbeddedGameTables.hpp"
#include <revolution/types.h>

extern const u8 GalaxyIDBCSV[0xD20];
extern const u8 StoryEventBCSV[0x1E0];

namespace smgpc::resource {
    EmbeddedGameTables::EmbeddedGameTables()
        : _galaxy_id(std::span{GalaxyIDBCSV}),
          _galaxy_id_alias(_galaxy_id.register_source(std::span{GalaxyIDBCSV})),
          _story_event(std::span{StoryEventBCSV}),
          _story_event_alias(_story_event.register_source(std::span{StoryEventBCSV})) {
    }
}
