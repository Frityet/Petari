#include "Game/Util/TriangleFilter.hpp"

namespace MR {
    TriangleFilterFunc* createTriangleFilterFunc(TriangleFunc func) {
        return new TriangleFilterFunc(func);
    }
};  // namespace MR
