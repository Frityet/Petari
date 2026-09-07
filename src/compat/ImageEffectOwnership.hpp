#pragma once

#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JutTextureConstruction.hpp"
#include <memory>

class NameObj;
class SceneObjHolder;

namespace smgpc::compat {
    // NameObjs retain their original ownership/registration. This scene owner
    // retires the raw state, matrix arrays and SDK textures they allocate.
    class ImageEffectOwnership final {
    public:
        explicit ImageEffectOwnership(SceneObjHolder&);
        ~ImageEffectOwnership();
        static bool handles(int id) noexcept;
        NameObj* construct(int id);
        void capture(int id, NameObj&, JutTextureConstructionScope&) noexcept;
        void capture_shared_textures(JutTextureConstructionScope&) noexcept;
        void prepare_rollback(NameObjRuntimeRegistrationMarker) noexcept;
        void prepare_retirement() noexcept;
        void reclaim_prepared() noexcept;
    private:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };
}
