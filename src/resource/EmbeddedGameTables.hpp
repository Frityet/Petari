#pragma once
#include "resource/JMapResource.hpp"

namespace smgpc::resource {
    // Owns decoded host metadata and publishes the genuine immutable program
    // resource identity for the original unsized JMapInfo::attach API.
    class EmbeddedGameTables final {
    public:
        EmbeddedGameTables();
    private:
        JMapResource _galaxy_id;
        JMapSourceRegistration _galaxy_id_alias;
    };
}
