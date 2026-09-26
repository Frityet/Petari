#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

class J3DShape;

namespace smgpc::resource {
    // Lifetime capture for the original reader, including shapes overwritten by
    // later hierarchy commands. No construction or ordering policy lives here.
    class J3dShapeAllocations final {
        struct ShapeDelete { void operator()(J3DShape*) const noexcept; };
        struct Commands {
            std::unique_ptr<std::uint8_t[]> bytes;
            std::size_t size;
        };
        std::vector<Commands> _commands;
        std::vector<std::unique_ptr<J3DShape, ShapeDelete>> _shapes;

    public:
        ~J3dShapeAllocations();

        class Scope final {
            J3dShapeAllocations* _previous;
        public:
            explicit Scope(J3dShapeAllocations&);
            ~Scope();
            Scope(const Scope&) = delete;
            Scope& operator=(const Scope&) = delete;
        };

        static J3DShape* retain(J3DShape*);
        static void retain_commands(std::uint8_t*, std::size_t);
    };
}
