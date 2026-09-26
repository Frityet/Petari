#include "J3dShapeAllocations.hpp"
#include "JSystem/J3DGraphBase/J3DShape.hpp"
#include "JSystem/J3DGraphBase/J3DShapeDraw.hpp"
#include "JSystem/J3DGraphBase/J3DShapeMtx.hpp"
#include <aurora/allocation.hpp>
#include <utility>

namespace smgpc::resource {
    namespace {
        thread_local J3dShapeAllocations* current_owner = nullptr;
    }

    J3dShapeAllocations::Scope::Scope(J3dShapeAllocations& owner)
        : _previous(std::exchange(current_owner, &owner)) {}
    J3dShapeAllocations::Scope::~Scope() { current_owner = _previous; }

    void J3dShapeAllocations::ShapeDelete::operator()(J3DShape* shape) const noexcept {
        if (shape == nullptr) return;
        for (std::size_t i = 0; i < shape->mMtxGroupNum; ++i) {
            delete shape->mShapeMtx[i];
            delete shape->mShapeDraw[i];
        }
        delete[] shape->mShapeMtx;
        delete[] shape->mShapeDraw;
        delete shape;
    }

    J3dShapeAllocations::~J3dShapeAllocations() {
        const auto cached = reinterpret_cast<std::uintptr_t>(J3DShape::sOldVcdVatCmd);
        for (const auto& commands : _commands) {
            const auto begin = reinterpret_cast<std::uintptr_t>(commands.bytes.get());
            if (cached >= begin && cached - begin < commands.size) {
                J3DShape::resetVcdVatCache();
                break;
            }
        }
    }

    J3DShape* J3dShapeAllocations::retain(J3DShape* shape) {
        if (current_owner != nullptr) {
            std::unique_ptr<J3DShape, ShapeDelete> allocation(shape);
            aurora::allocation::HostAllocationScope host;
            current_owner->_shapes.push_back(std::move(allocation));
        }
        return shape;
    }

    void J3dShapeAllocations::retain_commands(std::uint8_t* bytes, std::size_t size) {
        if (current_owner != nullptr) {
            Commands allocation{std::unique_ptr<std::uint8_t[]>(bytes), size};
            aurora::allocation::HostAllocationScope host;
            current_owner->_commands.push_back(std::move(allocation));
        }
    }
}
