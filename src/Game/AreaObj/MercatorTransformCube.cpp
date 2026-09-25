#include "Game/AreaObj/MercatorTransformCube.hpp"
#include <aurora/exception.hpp>
#include <stdexcept>

namespace MR {
    void getDivideMercatorRailPosition(DivideMercatorRailPosInfo *, const LiveActor *, u32, f32, u32) {
        aurora::throw_host_exception<std::logic_error>(
            "Mercator rail division is unavailable because the retail transformation routine has not been decompiled.");
    }
} // namespace MR
