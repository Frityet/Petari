#include "Game/LiveActor/Binder.hpp"

#include "Game/LiveActor/LiveActor.hpp"

#include <stdexcept>

// The complete original Binder implementation lives in Game/LiveActor.
// Native scene teardown owns its heap-backed plane array.
Binder::~Binder() {
    delete[] mPlane;
}

namespace MR {
    void setBinderOffsetVec(LiveActor* actor, const TVec3f* offset, bool local_space) {
        if (actor == nullptr || actor->mBinder == nullptr) {
            throw std::invalid_argument("setBinderOffsetVec requires a real actor Binder.");
        }
        actor->mBinder->mOffsetVec = offset;
        actor->mBinder->_1EC._4 = local_space;
    }
}  // namespace MR
