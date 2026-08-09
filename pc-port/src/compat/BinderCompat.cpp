#include "Game/LiveActor/Binder.hpp"

#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "compat/BinderCompat.hpp"
#include "scene/StageCollisionService.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>

namespace {
    constexpr auto cNoContact = -99999.0F;
    constexpr auto cWallDot = 0.34202015F;
    constexpr auto cDegreesToRadians = 3.14159265358979323846F / 180.0F;

    [[nodiscard]] auto& binder_owners() {
        static auto owners = std::unordered_map<const Binder*, const LiveActor*>{};
        return owners;
    }

    [[nodiscard]] bool normalize(TVec3f& value) {
        const auto square_length = value.dot(value);
        if (!(square_length > 1.0e-12F) || !std::isfinite(square_length)) {
            value.zero();
            return false;
        }
        value.scale(1.0F / std::sqrt(square_length));
        return true;
    }

    [[nodiscard]] TVec3f cross(const TVec3f& lhs, const TVec3f& rhs) {
        return TVec3f{lhs.y * rhs.z - lhs.z * rhs.y,
                      lhs.z * rhs.x - lhs.x * rhs.z,
                      lhs.x * rhs.y - lhs.y * rhs.x};
    }

    [[nodiscard]] TVec3f rotation_up(const TVec3f& rotation) {
        const auto rx = rotation.x * cDegreesToRadians;
        const auto ry = rotation.y * cDegreesToRadians;
        const auto rz = rotation.z * cDegreesToRadians;
        const auto sx = std::sin(rx);
        const auto cx = std::cos(rx);
        const auto sy = std::sin(ry);
        const auto cy = std::cos(ry);
        const auto sz = std::sin(rz);
        const auto cz = std::cos(rz);
        return TVec3f{cz * sy * sx - sz * cx,
                      sz * sy * sx + cz * cx,
                      cy * sx};
    }

    [[nodiscard]] TVec3f binder_up(const Binder& binder) {
        auto up = TVec3f{binder._C[0][1], binder._C[1][1], binder._C[2][1]};
        const auto owner = binder_owners().find(&binder);
        if (owner == binder_owners().end()) {
            (void)normalize(up);
            return up;
        }

        const auto& actor = *owner->second;
        if (std::abs(actor.mScale.y) > 1.0e-8F) {
            up.scale(1.0F / actor.mScale.y);
        } else {
            auto side = TVec3f{binder._C[0][0], binder._C[1][0], binder._C[2][0]};
            auto front = TVec3f{binder._C[0][2], binder._C[1][2], binder._C[2][2]};
            if (std::abs(actor.mScale.x) > 1.0e-8F) {
                side.scale(1.0F / actor.mScale.x);
            }
            if (std::abs(actor.mScale.z) > 1.0e-8F) {
                front.scale(1.0F / actor.mScale.z);
            }
            up = cross(front, side);
        }
        if (!normalize(up)) {
            up = rotation_up(actor.mRotation);
            (void)normalize(up);
        }
        return up;
    }

    void fill_hit_info(HitInfo& destination, const smgpc::scene::StageCollisionContact& contact,
                       std::uint32_t) {
        destination = HitInfo{};
        destination.mParentTriangle.mIdx = contact.triangle_index;
        if (auto* collision = smgpc::scene::StageCollisionService::active()) {
            if (const auto surface = collision->surface(contact.triangle_index)) {
                destination.mParentTriangle.mSensor = surface->sensor;
            }
        }
        destination.mParentTriangle.mNormals[0].set(contact.normal);
        destination.mParentTriangle.mPos[0].set(contact.position);
        destination.mParentTriangle.mPos[1].set(contact.position);
        destination.mParentTriangle.mPos[2].set(contact.position);
        destination._60 = contact.penetration;
        destination.mHitPos.set(contact.position);
        destination._88 = 1U;
    }
}  // namespace

namespace smgpc::compat {
    void register_binder_owner(Binder* binder, const LiveActor* actor) {
        if (binder == nullptr || actor == nullptr) {
            throw std::invalid_argument("Binder ownership requires a real Binder and LiveActor.");
        }
        binder_owners().insert_or_assign(binder, actor);
    }

    void release_binder_owner(const Binder* binder) noexcept {
        binder_owners().erase(binder);
    }
}  // namespace smgpc::compat

namespace MR {
    void setBinderOffsetVec(LiveActor* actor, const TVec3f* offset, bool local_space) {
        if (actor == nullptr || actor->mBinder == nullptr) {
            throw std::invalid_argument("setBinderOffsetVec requires a real actor Binder.");
        }
        actor->mBinder->mOffsetVec = offset;
        actor->mBinder->_1EC._4 = local_space;
    }
}  // namespace MR

Binder::Binder(MtxPtr matrix, const TVec3f* position, const TVec3f* gravity, f32 radius,
               f32 offset, u32 plane_capacity)
    : BinderParent(matrix), _10(position), _14(gravity), mRadius(radius), _1C(offset),
      mOffsetVec(nullptr), _24(plane_capacity), mPlaneNum(0), mPlaneInfos(nullptr),
      mFixReactionVector(), mGroundInfo(), _C8(cNoContact), mWallInfo(), _158(cNoContact),
      mRoofInfo(), _1E8(cNoContact), _1EC{} {
    if (position == nullptr || gravity == nullptr) {
        throw std::invalid_argument("Binder requires live position and gravity vectors.");
    }
    if (!std::isfinite(radius) || radius < 0.0F || !std::isfinite(offset)) {
        throw std::invalid_argument("Binder dimensions must be finite and non-negative.");
    }
    if (_24 != 0U) {
        mPlaneInfos = new HitInfo[_24];
    }
    clear();
    _1EC._0 = true;
    _1EC._1 = true;
}

Binder::~Binder() {
    smgpc::compat::release_binder_owner(this);
    delete[] mPlaneInfos;
}

void Binder::clear() {
    mPlaneNum = 0;
    _C8 = cNoContact;
    _158 = cNoContact;
    _1E8 = cNoContact;
    mFixReactionVector.zero();
}

void Binder::setCollisionPartsFilter(CollisionPartsFilterBase* filter) {
    mCollisionPartsFilter = filter;
}

void Binder::setTriangleFilter(TriangleFilterBase* filter) {
    mTriangleFilter = filter;
}

const Triangle* Binder::getPlane(int index) const {
    if (index < 0 || index >= mPlaneNum || mPlaneInfos == nullptr) {
        return nullptr;
    }
    return &mPlaneInfos[index].mParentTriangle;
}

u32 Binder::copyPlaneArrayAndSortingSensor(HitInfo** infos, u32 capacity) {
    if (infos == nullptr || capacity == 0U) {
        return 0U;
    }
    auto count = std::min(static_cast<u32>(mPlaneNum), capacity);
    for (auto index = 0U; index < count; ++index) {
        infos[index] = &mPlaneInfos[index];
    }
    std::sort(infos, infos + count, compSensor);
    return count;
}

bool Binder::compSensor(const HitInfo* lhs, const HitInfo* rhs) {
    return reinterpret_cast<std::uintptr_t>(rhs->mParentTriangle.mSensor) <
           reinterpret_cast<std::uintptr_t>(lhs->mParentTriangle.mSensor);
}

const TVec3f Binder::bind(const TVec3f& movement) {
    clear();

    auto center = *_10;
    if (mOffsetVec != nullptr) {
        if (_1EC._4 && _C != nullptr) {
            center.x += _C[0][0] * mOffsetVec->x + _C[0][1] * mOffsetVec->y + _C[0][2] * mOffsetVec->z;
            center.y += _C[1][0] * mOffsetVec->x + _C[1][1] * mOffsetVec->y + _C[1][2] * mOffsetVec->z;
            center.z += _C[2][0] * mOffsetVec->x + _C[2][1] * mOffsetVec->y + _C[2][2] * mOffsetVec->z;
        } else {
            center.add(*mOffsetVec);
        }
    } else if (_C != nullptr) {
        auto up = binder_up(*this);
        if (!up.epsilonEquals(TVec3f{}, 1.0e-8F)) {
            center.add(up * _1C);
        }
    } else {
        center.y += _1C;
    }

    auto* collision = smgpc::scene::StageCollisionService::active();
    if (collision == nullptr || collision->empty()) {
        return movement;
    }

    auto gravity = *_14;
    if (!normalize(gravity)) {
        throw std::logic_error("Binder contact classification requires a non-degenerate gravity vector.");
    }

    const auto maximum_contacts = _24 == 0U ? std::size_t{32U} : static_cast<std::size_t>(_24);
    const auto resolved =
        collision->move_sphere(center, movement, mRadius, maximum_contacts, _1EC._3);
    mFixReactionVector.set(resolved.fix_reaction);

    // A zero-capacity retail Binder uses a temporary 32-plane query array but
    // retains only its classified ground/wall/roof records. A nonzero
    // capacity exposes the full same-frame plane array to derived actors.
    if (_24 != 0U) {
        for (const auto& contact : resolved.contacts) {
            if (static_cast<u32>(mPlaneNum) >= _24) {
                break;
            }
            fill_hit_info(mPlaneInfos[mPlaneNum], contact, static_cast<u32>(mPlaneNum));
            if (mTriangleFilter != nullptr &&
                mTriangleFilter->isInvalidTriangle(&mPlaneInfos[mPlaneNum].mParentTriangle)) {
                continue;
            }
            ++mPlaneNum;
        }
    }

    for (auto index = std::size_t{}; index < resolved.contacts.size(); ++index) {
        auto info = HitInfo{};
        fill_hit_info(info, resolved.contacts[index], static_cast<u32>(index));
        if (mTriangleFilter != nullptr && mTriangleFilter->isInvalidTriangle(&info.mParentTriangle)) {
            continue;
        }
        const auto gravity_dot = info.mParentTriangle.mNormals[0].dot(gravity);
        if (std::abs(gravity_dot) < cWallDot) {
            if (info._60 > _158) {
                mWallInfo = info;
                _158 = info._60;
            }
        } else if (gravity_dot < 0.0F) {
            if (info._60 > _C8) {
                mGroundInfo = info;
                _C8 = info._60;
            }
        } else if (info._60 > _1E8) {
            mRoofInfo = info;
            _1E8 = info._60;
        }
    }

    return resolved.displacement;
}

void Binder::setExCollisionParts(CollisionParts* parts) {
    mCollisionParts = parts;
    _1EC._2 = parts != nullptr;
}
