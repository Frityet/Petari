#pragma once

#include <memory>

class DemoStartRequestHolder;

namespace smgpc::compat {

    // Own the allocations made by the original holder independently of the
    // scene arena. Requester, nerve, executor and string pointers stay borrowed,
    // as in the original record; this object does not start or dispatch demos.
    class DemoStartRequestOwner final {
    public:
        DemoStartRequestOwner();
        ~DemoStartRequestOwner();
        DemoStartRequestOwner(const DemoStartRequestOwner&) = delete;
        DemoStartRequestOwner& operator=(const DemoStartRequestOwner&) = delete;

        [[nodiscard]] DemoStartRequestHolder& get() noexcept;
        [[nodiscard]] const DemoStartRequestHolder& get() const noexcept;

    private:
        struct HolderDeleter {
            void operator()(DemoStartRequestHolder*) const noexcept;
        };

        std::unique_ptr<DemoStartRequestHolder, HolderDeleter> _holder;
    };

} // namespace smgpc::compat
