#pragma once

#include <cstdint>

class NameObj;
class TalkDirector;

namespace smgpc::compat {

    // Original Talk helpers contain only Game-heap allocations. The existing
    // scene arena reclaims those helpers and the NameObj graph owns their
    // LayoutActors/controllers. This boundary only removes borrowed identities.
    class TalkDirectorLifetime final {
    public:
        void capture_after_init(TalkDirector&);
        void begin_retirement() noexcept;
        void release_name_obj(const NameObj*) noexcept;

    private:
        TalkDirector* _director = nullptr;
        std::uint64_t _generation = 0;
        bool _retiring = false;
    };

} // namespace smgpc::compat
