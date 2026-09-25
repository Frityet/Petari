#pragma once

#include "NativeHeapFixture.hpp"
#include "OriginalSceneControllerFixture.hpp"
#include "SceneExecutionFixture.hpp"
#include "Game/Map/CollisionCategorizedKeeper.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Map/CollisionParts.hpp"
#include "Game/Map/KCollision.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "resource/KCollisionResource.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

namespace smgpc::test {
// Explicit synthetic query fixture: real original owners and methods, with
// controlled KCL geometry. This does not claim an authored stage placement.
class OriginalSphereQueryFixture {
public:
    OriginalSphereQueryFixture()
        : heaps(smgpc::test::create_native_root_heap(32U << 20)), original(heaps),
          active(scheduler), domain(smgpc::test::create_native_solid_heap(heaps, 2U << 20)),
          scene(scheduler, domain, nullptr, nullptr, &original.scene, original.controller().mObjHolder) {
        auto* director = static_cast<CollisionDirector*>(MR::createSceneObj(SceneObj_CollisionDirector));
        if (!director) throw std::runtime_error("Sphere queries require the actual CollisionDirector");
        keeper = director->getCategoryKeeper(0);
        // A pure geometry fixture has one explicit original zone. It does not
        // substitute a GalaxyStatusAccessor or pretend to load a scenario.
        keeper->mZones[0] = new CollisionZone(0);
        keeper->mZoneNum = 1;
        keeper->_A0 = true;
    }

    static std::array<float, 12> matrix(float scale = 1.0f, TVec3f translation = TVec3f(0.0f)) {
        return {scale, 0, 0, translation.x, 0, scale, 0, translation.y, 0, 0, scale, translation.z};
    }

    static std::vector<std::uint8_t> triangles(float thickness = 2.0f,
                                              std::vector<std::uint16_t> leaf_order = {1}) {
        const auto count = *std::max_element(leaf_order.begin(), leaf_order.end());
        const auto octree = std::size_t{0x74} + count * 16;
        std::vector<std::uint8_t> bytes(octree + 8 + leaf_order.size() * 2);
        const auto u32 = [&](std::size_t at, std::uint32_t value) {
            for (int i = 0; i < 4; ++i) bytes[at + i] = static_cast<std::uint8_t>(value >> ((3-i)*8));
        };
        const auto u16 = [&](std::size_t at, std::uint16_t value) {
            bytes[at] = value >> 8; bytes[at+1] = value;
        };
        const auto f32 = [&](std::size_t at, float value) { u32(at, std::bit_cast<std::uint32_t>(value)); };
        const auto vec = [&](std::size_t at, TVec3f value) { f32(at,value.x); f32(at+4,value.y); f32(at+8,value.z); };
        u32(0,0x38); u32(4,0x44); u32(8,0x64); u32(12,octree);
        f32(0x10,thickness); vec(0x14,TVec3f(-100.0f));
        u32(0x20,0xffffff00); u32(0x24,0xffffff00); u32(0x28,0xffffff00);
        u32(0x2c,8); u32(0x30,0xffffffff); u32(0x34,0xffffffff);
        vec(0x38,TVec3f(0.0f));
        vec(0x44,TVec3f(0,1,0)); vec(0x50,TVec3f(-1,0,0)); vec(0x5c,TVec3f(0,0,-1));
        constexpr float diagonal = 0.7071067811865475f;
        vec(0x68,TVec3f(diagonal,0,diagonal));
        for (std::size_t i=0;i<count;++i) {
            const auto prism=0x74+i*16;
            f32(prism,10*diagonal); u16(prism+4,0); u16(prism+6,0);
            u16(prism+8,1); u16(prism+10,2); u16(prism+12,3); u16(prism+14,0);
        }
        u32(octree,0x80000004);
        for (std::size_t i=0;i<leaf_order.size();++i) u16(octree+6+i*2,leaf_order[i]);
        return bytes;
    }

    class Part {
    public:
        Part(OriginalSphereQueryFixture& fixture, std::vector<std::uint8_t> bytes,
             std::array<float,12> transform = matrix(), HitSensor* sensor = nullptr)
            : resource(bytes), owner(fixture), parts() {
            parts.mServer->init(resource.native_file(),nullptr);
            parts.mHitSensor=sensor;
            parts.mServer->calcFarthestVertexDistance();
            set_matrices(transform,transform);
            parts._D4=2;
            parts._CC=true;
            parts.mKeeperIndex=0;
            parts.mZone=owner.keeper->mZones[0];
            owner.keeper->addToZone(&parts,0);
        }
        ~Part() {
            owner.keeper->removeFromZone(&parts,0);
        }
        void set_matrices(const std::array<float,12>& current, const std::array<float,12>& previous) {
            std::copy(current.begin(),current.end(),&parts.mBaseMatrix.mMtx[0][0]);
            std::copy(previous.begin(),previous.end(),&parts.mPrevBaseMatrix.mMtx[0][0]);
            parts.mMatrix=parts.mBaseMatrix;
            if (!PSMTXInverse(parts.mBaseMatrix,parts.mInvBaseMatrix))
                throw std::runtime_error("Synthetic collision matrix must be invertible");
            TVec3f scale;
            parts.mBaseMatrix.getScale(scale);
            parts._D8=parts.mServer->mMaxVertexDistance*(scale.x+scale.y+scale.z)/3.0f;
        }
        resource::KCollisionResource resource;
        OriginalSphereQueryFixture& owner;
        CollisionParts parts;
    };

    JKRHeap::Handle heaps;
    OriginalSceneControllerFixture original;
    runtime::SceneScheduler scheduler;
    runtime::SceneSchedulerBinding active;
    JKRHeap::Handle domain;
    SceneExecutionFixture scene;
    CollisionCategorizedKeeper* keeper;
};
}
