#include "Game/Map/HitInfo.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/TriangleFilter.hpp"
#include "Game/Map/KCollision.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/HitInfoCompat.hpp"
#include "resource/KCollisionResource.hpp"
#include "scene/StageCollisionService.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
    using Bytes = std::vector<std::uint8_t>;
    using Collision = smgpc::scene::StageCollisionService;
    using Registration = smgpc::scene::StageCollisionRegistrationState;
    constexpr auto identity = std::array<float, 12>{1,0,0,0, 0,1,0,0, 0,0,1,0};
    void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
    void put16(Bytes& bytes, std::size_t at, unsigned value) {
        bytes.at(at) = value >> 8; bytes.at(at + 1) = value;
    }
    void put32(Bytes& bytes, std::size_t at, std::uint32_t value) {
        bytes.at(at) = value >> 24; bytes.at(at + 1) = value >> 16;
        bytes.at(at + 2) = value >> 8; bytes.at(at + 3) = value;
    }
    void put_float(Bytes& bytes, std::size_t at, float value) { put32(bytes, at, std::bit_cast<std::uint32_t>(value)); }
    void put_vector(Bytes& bytes, std::size_t at, std::array<float,3> value) {
        for (std::size_t i=0; i<3; ++i) put_float(bytes, at+i*4, value[i]);
    }
    Bytes kcl(const std::vector<unsigned>& order = {3,1,3,2}) {
        // Three nested positive prisms at Z=0, with a deliberately repeated,
        // non-file-order leaf. The leaf order is observable before sorting.
        constexpr auto first_prism = std::size_t{116};
        constexpr auto tree = first_prism + 48;
        Bytes bytes(tree + 8 + order.size()*2);
        put32(bytes,0,56); put32(bytes,4,68); put32(bytes,8,first_prism-16); put32(bytes,12,tree);
        put_float(bytes,16,1); put_vector(bytes,20,{-16,-16,-16});
        for (auto at : {32,36,40}) put32(bytes,at,0xffffffe0U);
        put32(bytes,44,5);
        put_vector(bytes,56,{0,0,0}); put_vector(bytes,68,{0,0,1});
        put_vector(bytes,80,{-1,0,0}); put_vector(bytes,92,{0,-1,0}); put_vector(bytes,104,{0.5F,0.5F,0});
        for (unsigned i=0;i<3;++i) {
            auto at=first_prism+i*16;
            put_float(bytes,at,2); put16(bytes,at+8,1); put16(bytes,at+10,2); put16(bytes,at+12,3);
        }
        put32(bytes,tree,0x80000004U);
        put16(bytes,tree+4,0x1234);
        for (std::size_t i=0;i<order.size();++i) put16(bytes,tree+6+i*2,order[i]);
        return bytes;
    }
    std::vector<std::uint32_t> indices(const Collision& collision, const std::vector<smgpc::scene::StageCollisionHit>& hits) {
        std::vector<std::uint32_t> result;
        for (auto& hit:hits) result.push_back(collision.surface(hit.triangle_index)->prism_index);
        return result;
    }
    void original_octree_and_boundary_contract() {
        Collision collision;
        require(collision.add_kcl(kcl(),identity),"KCL registration failed");
        const TVec3f start(1,1,5), offset(0,0,-10);
        require(indices(collision,collision.line_hits(start,offset)) == std::vector<std::uint32_t>{2,0,2,1},
                "All hits must preserve leaf order and repeated occurrences");
        require(indices(collision,collision.line_hits(start,offset,2)) == std::vector<std::uint32_t>{2,0},"Capacity must precede distance sorting");
        require(collision.line_hits(start,TVec3f(0,0,-5)).size()==4,"An exact endpoint remains an all-hit arrow hit");
        require(collision.line_hits(TVec3f(1,1,-5),TVec3f(0,0,10)).empty(),"Back faces must remain rejected");
        require(collision.line_hits(TVec3f(1,1,0),offset).empty(),"A line beginning on a face is rejected");
        require(collision.line_hits(start,TVec3f(0,0,0)).empty(),"A zero line is empty");
        require(collision.line_hits(start,offset,0).empty(),"Zero capacity must not enter KCL traversal");
        require(collision.line_hits(TVec3f(9,9,5),offset).empty(),"Part sphere and actual prism boundaries must reject misses");
        bool rejected=false;
        try { (void)collision.line_hits(start,offset,33); } catch (const std::invalid_argument&) { rejected=true; }
        require(rejected,"Beyond-original capacities must be rejected");
    }
    void transforms_and_stable_sorted_results() {
        Collision collision;
        auto matrix=identity; matrix[0]=0; matrix[1]=-2; matrix[4]=3; matrix[5]=0; matrix[10]=4;
        matrix[3]=10;matrix[7]=20;matrix[11]=30;
        require(collision.add_kcl(kcl(),matrix,"affine"),"Affine KCL registration failed");
        collision.activate();
        const TVec3f start(8,23,50),offset(0,0,-40);
        require(MR::getNearPolyOnLineSort(start,start,offset,nullptr)==4,"Affine original line must produce four hits");
        TVec3f position; Triangle triangle;
        for (u32 i=0;i<4;++i) {
            require(MR::getSortedPoly(&position,&triangle,i),"Sorted index missing");
            require(std::abs(position.x-8)<1e-5F && std::abs(position.y-23)<1e-5F && std::abs(position.z-30)<1e-5F,
                    "Original local hit must transform back through the actual matrix");
            require(collision.surface(triangle.mIdx)->prism_index==std::array{2U,0U,2U,1U}[i],"Distance ties must retain original encounters");
            require(MR::getSortedPoly(i)->mIdx==triangle.mIdx,"Both sorted getters must share one persistent result buffer");
        }
        require(!MR::getSortedPoly(nullptr,nullptr,4) && MR::getSortedPoly(4)==nullptr,"Sorted getters must enforce current count");
        const auto retained=MR::getSortedPoly(0)->mIdx;
        require(MR::getNearPolyOnLineSort(start,start,TVec3f(0,0,1),nullptr)==0,"Upward query must miss");
        require(MR::getSortedPoly(0)->mIdx==retained,"Retail zero-total-hit early return must retain the previous sorted buffer");
    }
    void dynamic_transform_refit_and_original_point_velocity() {
        Collision collision;
        auto registration = std::make_shared<Registration>();
        require(collision.register_kcl(kcl({1}), identity, "moving source", registration, {}, nullptr, 2).accepted,
                "Dynamic fixture requires an actual registered KCL source");
        collision.build();
        collision.activate();
        const TVec3f point(1, 1, 0);
        auto hit = collision.line_hits(TVec3f(1, 1, 5), TVec3f(0, 0, -10));
        require(hit.size() == 1, "Initial source must populate original decoded KCL cache");
        auto triangle = smgpc::compat::make_collision_triangle(collision, hit[0].triangle_index);
        const auto copied = triangle;
        auto* base = triangle.getBaseMtx();
        auto* inverse = triangle.getBaseInvMtx();
        auto* previous = triangle.getPrevBaseMtx();
        TVec3f velocity(7, 8, 9);
        MR::calcVelocityMovingPoint(&triangle, point, &velocity);
        require(velocity.squared() == 0.0F, "Original equal-matrix branch must write zero velocity");

        // Nonuniform scale and rotation deliberately distinguish inverse-base
        // point velocity from the forward force-move formula.
        const auto current = std::array<float, 12>{0, -2, 0, 10, 3, 0, 0, 20, 0, 0, 4, 30};
        collision.update_registered_transform(*registration, current, identity);
        require(copied.getBaseMtx() == base && copied.getBaseInvMtx() == inverse && copied.getPrevBaseMtx() == previous,
                "Refit must preserve borrowed matrix addresses and copied Triangle identities");
        const TVec3f moved_point(8, 23, 30);
        MR::calcVelocityMovingPoint(&copied, moved_point, &velocity);
        require(velocity.epsilonEquals(TVec3f(7, 22, 30), 0.00001F),
                "Original point velocity must transform the current world point back into the previous frame");
        TVec3f force_move;
        copied.calcForceMovePower(&force_move, point);
        require(force_move.epsilonEquals(TVec3f(7,22,30), 0.00001F),
                "Forward force movement must advance the previous world point through the actual transforms");
        copied.calcForceMovePower(&force_move, moved_point);
        require(!force_move.epsilonEquals(velocity, 0.00001F),
                "Forward force movement and backward point velocity must remain distinct under rotation and scale");
        auto alias = moved_point;
        MR::calcVelocityMovingPoint(&copied, alias, &alias);
        require(alias.epsilonEquals(velocity, 0.00001F), "Original output may alias the input point");
        smgpc::scene::StageCollisionHit nearest;
        require(!collision.line_cast(TVec3f(1, 1, 5), TVec3f(0, 0, -10)), "Refit must retire old BVH geometry");
        require(collision.line_cast(TVec3f(8, 23, 50), TVec3f(0, 0, -40), &nearest) && nearest.triangle_index == triangle.mIdx,
                "Refit must publish new BVH geometry with the same stable prism identity");
        require(collision.line_hits(TVec3f(8, 23, 50), TVec3f(0, 0, -40)).size() == 1,
                "Original octree query must use updated transform and cached broad-phase radius");
        require(collision.sphere_contacts(TVec3f(8, 23, 30.5F), 1.0F).size() == 3,
                "Sphere contacts must refit all three physical source prisms, independently of the one-entry octree leaf");
        std::array<Triangle, 4> areas;
        require(MR::createAreaPolygonList(areas.data(), 4, TVec3f(7,22,30), TVec3f(9,24,30)) == 1 && areas[0].mIdx == triangle.mIdx,
                "Original two-point area query must use transformed local bounds and original KCL order");
        require(MR::createAreaPolygonList(areas.data(), 4, TVec3f(9,24,30), TVec3f(7,22,30)) == 1,
                "Two-point area queries must retain reversed-endpoint and collapsed-axis behavior");
        collision.update_registered_transform(*registration, current, current);
        MR::calcVelocityMovingPoint(&copied, moved_point, &velocity);
        require(velocity.squared() == 0.0F, "Owner-supplied reset must clear previous-frame point velocity");
        auto singular = current; singular[10] = 0;
        bool rejected = false;
        try { collision.update_registered_transform(*registration, singular, current); }
        catch (const std::invalid_argument&) { rejected = true; }
        require(rejected && collision.line_cast(TVec3f(8,23,50), TVec3f(0,0,-40)),
                "Invalid updates must preserve prior geometry and borrowed transforms");
        auto other = std::make_shared<Registration>();
        rejected = false;
        try { collision.update_registered_transform(*other, identity, identity); }
        catch (const std::logic_error&) { rejected = true; }
        require(rejected, "An unrelated registration must not move another owner's collision");
        registration->set_enabled(false);
        collision.update_registered_transform(*registration, identity, identity);
        require(!collision.line_cast(TVec3f(1,1,5), TVec3f(0,0,-10)), "Updating a disabled owner must not re-enable it");
        registration->set_enabled(true);
        require(collision.line_cast(TVec3f(1,1,5), TVec3f(0,0,-10)), "Re-entry must expose the exact newly committed geometry");
        registration->release_owner();
        rejected = false;
        try { collision.update_registered_transform(*registration, current, current); }
        catch (const std::logic_error&) { rejected = true; }
        require(rejected, "A released owner must never regain transform authority");
    }

    void reference_distance_zone_order_and_sensor_capacity() {
        Collision collision;
        auto far=identity; far[11]=-2;
        auto near=identity; near[11]=2;
        auto marker_a=std::array<std::byte,8>{},marker_b=std::array<std::byte,8>{};
        auto* sensor_a=reinterpret_cast<HitSensor*>(marker_a.data());
        auto* sensor_b=reinterpret_cast<HitSensor*>(marker_b.data());
        require(collision.register_kcl(kcl({1}),far,"far",std::make_shared<Registration>(), {},sensor_a,3).accepted,"Far source registration failed");
        require(collision.register_kcl(kcl({1}),near,"near",std::make_shared<Registration>(), {},sensor_b,1).accepted,"Near source registration failed");
        collision.activate();
        const TVec3f start(1,1,5), offset(0,0,-10);
        auto hits=collision.line_hits(start,offset);
        require(hits.size()==2 && collision.surface(hits[0].triangle_index)->sensor==sensor_b,"Zone order must precede registration and BVH order");
        require(MR::getNearPolyOnLineSort(TVec3f(1,1,-4),start,offset,nullptr)==2,"Both surfaces must sort");
        require(MR::getSortedPoly(0)->mSensor==sensor_a,"Sort reference must be independent of the ray origin");
        require(MR::getNearPolyOnLineSort(start,start,offset,sensor_b)==1 && MR::getSortedPoly(0)->mSensor==sensor_a,
                "Excluded sensor must be removed before sort, after collision hit gathering");
        require(::Collision::getStrikeInfoNumMap()==2 && ::Collision::getStrikeInfoMap(0)->mParentTriangle.mSensor==sensor_b,
                "Sorted queries must also update the shared original unsorted strike buffer");
        Collision capped;
        require(capped.register_kcl(kcl(std::vector<unsigned>(32,1)),identity,"excluded",std::make_shared<Registration>(), {},sensor_a,0).accepted,"Cap source registration failed");
        require(capped.register_kcl(kcl({1}),identity,"after cap",std::make_shared<Registration>(), {},sensor_b,0).accepted,"After-cap registration failed");
        collision.deactivate();
        capped.activate();
        require(MR::getNearPolyOnLineSort(start,start,offset,sensor_a)==0 && MR::getSortedPoly(0)==nullptr,
                "Excluding the first 32 hits must not admit later parts or retain an old count");
    }
    void direct_strike_filter_capacity_and_callback_domain() {
        Collision collision;
        require(collision.add_kcl(kcl({3,1}),identity,"first"),"First filter source registration failed");
        require(collision.add_kcl(kcl({2}),identity,"second"),"Second filter source registration failed");
        collision.activate();
        auto runtime=smgpc::compat::JkrHeapRuntime::create(2U<<20);
        auto domain=smgpc::compat::JkrAllocationDomain::create(runtime,1U<<20);
        struct Filter : TriangleFilterBase {
            const Collision& collision;
            JKRHeap* heap;
            mutable unsigned calls=0;
            Filter(const Collision& c, JKRHeap* h):collision(c),heap(h){}
            bool isInvalidTriangle(const Triangle* triangle) const override {
                ++calls;
                auto* original_allocation=new int(7);
                const bool original_domain=JKRHeap::findFromRoot(original_allocation)==heap;
                delete original_allocation;
                require(original_domain,"Original triangle filter must regain its Game allocation domain");
                return collision.surface(triangle->mIdx)->prism_index==2;
            }
        } filter(collision,&domain->heap());
        {
            smgpc::compat::JkrAllocationScope game(domain);
            require(::Collision::checkStrikeLineToMap(TVec3f(1,1,5),TVec3f(0,0,-10),1,nullptr,&filter)==1,
                    "Rejected part hits must leave capacity for following parts");
        }
        require(filter.calls==2,"Filtering must happen after the per-part KCL limit, without admitting the next prism in that part");
        const auto* info=::Collision::getStrikeInfoMap(0);
        require(info!=nullptr && collision.surface(info->mParentTriangle.mIdx)->source_name=="second",
                "Original filtered encounter/capacity order differs");
        require(std::abs(info->_60-5)<1e-5F && std::abs(info->mHitPos.z)<1e-5F,
                "Original HitInfo must report world distance and position");
        require(::Collision::getStrikeInfoMap(1)==nullptr,"Direct strike count boundary differs");
        require(::Collision::checkStrikeLineToMap(TVec3f(1,1,5),TVec3f(0,0,10),0,nullptr,nullptr)==0 &&
                ::Collision::getStrikeInfoNumMap()==0,"A direct miss must clear the unsorted strike count");
    }
    void enable_remove_reentry_and_heap_lifetime() {
        Collision collision;
        std::array<std::shared_ptr<Registration>,3> registrations;
        for(unsigned i=0;i<3;++i) {
            registrations[i]=std::make_shared<Registration>();
            require(collision.register_kcl(kcl({i+1}),identity,std::to_string(i),registrations[i],{},nullptr,2).accepted,"Ordered owner registration failed");
        }
        const TVec3f start(1,1,5),offset(0,0,-10);
        require(indices(collision,collision.line_hits(start,offset))==std::vector<std::uint32_t>{0,1,2},"Initial parts order differs");
        registrations[0]->set_enabled(false);
        require(indices(collision,collision.line_hits(start,offset))==std::vector<std::uint32_t>{2,1},"Part removal must swap the last part into its position");
        registrations[0]->set_enabled(true);
        require(indices(collision,collision.line_hits(start,offset))==std::vector<std::uint32_t>{2,1,0},"Part reentry must append");
        registrations[1]->release_owner();
        require(indices(collision,collision.line_hits(start,offset))==std::vector<std::uint32_t>{2,0},"Released owners must be excluded");
        // The query's original KCL/cache/vector allocations must survive the
        // calling Game arena. The actor's own next allocation stays in Game.
        Collision retained;
        auto retained_registration = std::make_shared<Registration>();
        require(retained.register_kcl(kcl(), identity, "retained", retained_registration).accepted,
                "Retained source registration failed");
        retained.build();
        retained.activate();
        {
            auto runtime = smgpc::compat::JkrHeapRuntime::create(2U<<20);
            auto domain = smgpc::compat::JkrAllocationDomain::create(runtime,1U<<20);
            smgpc::compat::JkrAllocationScope game(domain);
            auto* heap=&domain->heap();
            const auto before=heap->getFreeSize();
            require(MR::getNearPolyOnLineSort(start,start,offset,nullptr)==4,"Query in Game domain failed");
            retained.update_registered_transform(*retained_registration, identity, identity);
            require(before==heap->getFreeSize(),"Persistent native query/refit allocations leaked into Game");
            auto* value=new int(42);
            require(JKRHeap::findFromRoot(value)==heap,"Query must restore original Game allocation routing");
            delete value;
        }
        require(MR::getSortedPoly(0)->isValid() && retained.line_hits(start,offset).size()==4,"Query metadata must survive retired calling arena");
    }
}
int main() {
    try {
        dynamic_transform_refit_and_original_point_velocity();
        original_octree_and_boundary_contract();
        transforms_and_stable_sorted_results();
        reference_distance_zone_order_and_sensor_capacity();
        direct_strike_filter_capacity_and_callback_domain();
        enable_remove_reentry_and_heap_lifetime();
        std::cout<<"PASS: original all-hit KCL ordering, boundaries, sorting, ownership and capacity\n";
        return 0;
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
