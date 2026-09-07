#include "Game/Util/FixedPosition.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/FixedPositionCompat.hpp"

#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeContext.hpp"

#include <cmath>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace {
    constexpr f32 cDegToRad = 3.14159265358979323846F / 180.0F;
    constexpr f32 cRadToDeg = 180.0F / 3.14159265358979323846F;
    constexpr f32 cMinAxisLength = 0.000001F;

    static_assert(sizeof(smgpc::render::J3dMatrix3x4) == sizeof(Mtx));
    static_assert(std::is_standard_layout_v< smgpc::render::J3dMatrix3x4 >);







    [[nodiscard]] TVec3f read_vector(const smgpc::resource::BcsvTable& table, std::string_view prefix) {
        auto result = TVec3f{};
        if (const auto x = table.get_float(0U, std::string(prefix) + "X"); x.has_value()) {
            result.x = *x;
        }
        if (const auto y = table.get_float(0U, std::string(prefix) + "Y"); y.has_value()) {
            result.y = *y;
        }
        if (const auto z = table.get_float(0U, std::string(prefix) + "Z"); z.has_value()) {
            result.z = *z;
        }
        return result;
    }
}  // namespace

namespace smgpc::compat {
    FixedPositionResourceData load_fixed_position_resource(const smgpc::resource::RarcArchive& archive, std::string_view resource_name) {
        if (resource_name.empty()) {
            throw std::invalid_argument("FixedPosition resource name is empty");
        }

        const auto file_name = std::string(resource_name) + ".bcsv";
        const auto* entry = archive.find_resource(file_name);
        if (entry == nullptr) {
            throw std::runtime_error("FixedPosition resource does not exist: " + file_name);
        }

        const auto table = smgpc::resource::BcsvTable::from_bytes(archive.file_data(*entry));
        if (table.entry_count() == 0U) {
            throw std::runtime_error("FixedPosition resource has no rows: " + file_name);
        }

        auto joint_name = table.get_string(0U, "JointName");
        if (joint_name.has_value() && joint_name->empty()) {
            joint_name.reset();
        }

        return FixedPositionResourceData{
            .joint_name = std::move(joint_name),
            .translation = read_vector(table, "Trans"),
            .rotation = read_vector(table, "Rotate"),
        };
    }
}  // namespace smgpc::compat

void FixedPosition::copyRotate(TVec3f* pRotate) const {
    if (pRotate == nullptr) {
        return;
    }

    mMtx.getEulerXYZ(*pRotate);
    pRotate->scale(cRadToDeg);
}
