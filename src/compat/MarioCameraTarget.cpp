#include <aurora/allocation.hpp>
#include "resource/TextEncoding.hpp"
#include "compat/MarioCameraTarget.hpp"

#include "Game/Player/MarioActor.hpp"

namespace {
    // These stable Game names outlive every owner that borrows their bytes.
    std::string encode_owner_name(std::string_view name) {
        aurora::allocation::HostAllocationScope host;
        return smgpc::resource::encode_cp932(name);
    }

    const std::string cMarioCameraTargetName = encode_owner_name("マリオ注目");
}  // namespace

namespace smgpc::compat {

    std::unique_ptr<CameraTargetObj> create_mario_camera_target(const MarioActor &actor) {
        auto target = std::make_unique<CameraTargetPlayer>(cMarioCameraTargetName.c_str());
        // CameraTargetHolder::set(const MarioActor*) binds the same field.
        // Movement belongs to the camera phase, after actor initialization.
        target->mActor = &actor;
        return target;
    }

    MtxPtr mario_camera_base_matrix(const MarioActor &actor) {
        // MarioAccess::getBaseMtx uses the actor's forced matrix when active.
        if (actor._EA5) {
            return const_cast<MarioActor &>(actor)._EA8;
        }
        return actor.getBaseMtx();
    }

}  // namespace smgpc::compat
