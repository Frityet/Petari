#pragma once

#include <memory>

class MarioAnimator;

namespace smgpc::compat {
    // Native ownership only. The enclosed MarioAnimator::init remains original.
    class MarioAnimatorConstructionScope final {
    public:
        explicit MarioAnimatorConstructionScope(MarioAnimator&);
        ~MarioAnimatorConstructionScope();
        MarioAnimatorConstructionScope(const MarioAnimatorConstructionScope&) = delete;
        MarioAnimatorConstructionScope& operator=(const MarioAnimatorConstructionScope&) = delete;

    private:
        struct Storage;
        std::unique_ptr<Storage> _storage;
    };
}
