#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <JSystem/JGeometry/TVec.hpp>

namespace smgpc::resource {
    class RarcArchive;
}

namespace smgpc::compat {
    struct FixedPositionResourceData {
        std::optional< std::string > joint_name;
        TVec3f translation;
        TVec3f rotation;
    };

    [[nodiscard]] FixedPositionResourceData load_fixed_position_resource(const smgpc::resource::RarcArchive& archive, std::string_view resource_name);
}  // namespace smgpc::compat
