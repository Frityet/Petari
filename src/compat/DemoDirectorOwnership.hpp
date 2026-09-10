#pragma once

#include "compat/ActorRuntimeRegistry.hpp"

#include <cstddef>
#include <memory>

class DemoCastGroup;
class DemoDirector;
class DemoExecutor;
class NameObj;

namespace smgpc::compat {

    // The scene binding owns every NameObj. This owner retains their Game
    // allocation domains and retires only the original non-NameObj children.
    class DemoDirectorOwnership final {
    public:
        DemoDirectorOwnership();
        ~DemoDirectorOwnership();
        DemoDirectorOwnership(const DemoDirectorOwnership&) = delete;
        DemoDirectorOwnership& operator=(const DemoDirectorOwnership&) = delete;

        // Capture completed construction/init under its original Game scope.
        void capture(DemoDirector&);
        void capture_cast_group(DemoCastGroup&);
        void capture_executor(DemoExecutor&);

        // Prepare before destroying the original NameObjs; reclaim afterward,
        // before releasing their scene domains. Both operations are repeatable.
        void prepare_retirement() noexcept;
        void prepare_rollback(NameObjRuntimeRegistrationMarker) noexcept;
        void reclaim() noexcept;

        // Remove borrowed identities from the actual original containers.
        void release_name_obj(const NameObj*) noexcept;
        [[nodiscard]] std::size_t simple_cast_registration_count(const NameObj* = nullptr) const noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

} // namespace smgpc::compat
