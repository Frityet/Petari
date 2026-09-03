#include "Game/System/RenderMode.hpp"
#include "Game/Util/SystemUtil.hpp"

namespace MR {
    // Complete original SystemUtil method; the full TU's GameSystem owner
    // dependencies are selected separately by the native bootstrap.
    bool isScreen16Per9() {
        return isAspectRatioFlag16Per9();
    }
}
