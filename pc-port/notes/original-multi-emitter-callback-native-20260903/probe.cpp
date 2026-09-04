#include "runtime/ParticleResourceOwnership.hpp"
#include "runtime/ArchiveMountService.hpp"
#include "runtime/RuntimeServices.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "Game/Effect/AutoEffectInfo.hpp"
#include "Game/Effect/EffectSystemUtil.hpp"
#include "Game/Effect/MultiEmitter.hpp"
#include "Game/Effect/MultiEmitterCallBack.hpp"
#include "Game/Effect/MultiEmitterParticleCallBack.hpp"
#include "Game/Effect/ParticleResourceHolder.hpp"
#include "Game/Effect/SingleEmitter.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "JSystem/JParticle/JPAEmitter.hpp"
#include "JSystem/JParticle/JPAEmitterManager.hpp"
#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string_view>

namespace aurora { extern AuroraConfig g_config; }
using namespace smgpc;

static void near(float actual, float expected, float tolerance = 0.00003F) {
    if (std::abs(actual - expected) > tolerance || !std::isfinite(actual)) {
        std::cerr << "actual=" << actual << " expected=" << expected << '\n';
        std::abort();
    }
}
static void vector_near(const TVec3f& actual, float x, float y, float z) {
    near(actual.x,x); near(actual.y,y); near(actual.z,z);
}
static void scale_near(const JPABaseEmitter& emitter, float x, float y, float z) {
    vector_near(emitter.mGlobalScl,x,y,z);
    near(emitter.mGlobalPScl.x,x); near(emitter.mGlobalPScl.y,y);
}
static void color(const GXColor& actual, unsigned r, unsigned g, unsigned b, unsigned a) {
    assert(actual.r == r && actual.g == g && actual.b == b && actual.a == a);
}

// Independent double-precision composition of elementary rotation matrices.
// The input angle is truncated to signed16 by the original PPC conversion;
// the SDK trigonometric table then discards its bottom two bits.
using Matrix = std::array<std::array<double,3>,3>;
static Matrix product(const Matrix& a, const Matrix& b) {
    Matrix result{};
    for (unsigned r=0;r<3;++r) for (unsigned c=0;c<3;++c)
        for (unsigned k=0;k<3;++k) result[r][c] += a[r][k]*b[k][c];
    return result;
}
static double quantized_radians(float degrees) {
    const float scaled = degrees * 182.04445F;
    const auto truncated = static_cast<std::int64_t>(scaled);
    const auto bits = static_cast<std::uint16_t>(truncated);
    return double(bits >> 2) * (2.0 * std::numbers::pi / 16384.0);
}
static Matrix expected_rotation(const TVec3f& degrees) {
    const double x=quantized_radians(degrees.x), y=quantized_radians(degrees.y), z=quantized_radians(degrees.z);
    const double sx=std::sin(x),cx=std::cos(x),sy=std::sin(y),cy=std::cos(y),sz=std::sin(z),cz=std::cos(z);
    const Matrix rx{{{1,0,0},{0,cx,-sx},{0,sx,cx}}};
    const Matrix ry{{{cy,0,sy},{0,1,0},{-sy,0,cy}}};
    const Matrix rz{{{cz,-sz,0},{sz,cz,0},{0,0,1}}};
    return product(product(rz,ry),rx);
}

static void flags(MultiEmitter& multi, AutoEffectInfo& info) {
    auto& cb=*multi.mCallBack;
    const TVec3f host(1,2,3);
    Mtx matrix{{1,0,0,0},{0,1,0,0},{0,0,1,0}};
    std::size_t cases=0;
    // Check each authored init/follow bit, force bit, off/reset priority,
    // missing host component, and matrix-provided component independently.
    for (unsigned metadata=0;metadata<128;++metadata)
    for (unsigned forces=0;forces<128;++forces)
    for (unsigned available=0;available<8;++available)
    for (bool matrix_host:{false,true})
    for (bool initial:{false,true}) {
        info.mFlag=metadata;
        cb.mFlags=forces;
        cb.mScale=(available&1)?&host:nullptr;
        cb.mRotation=(available&2)?&host:nullptr;
        cb.mTranslation=(available&4)?&host:nullptr;
        cb.mMtx=matrix_host?matrix:nullptr;
        MultiEmitterCallBack::FlagSRT result{false,false,false};
        cb.isFollowSRT(&result,initial);
        const unsigned wanted=initial?(metadata>>3):metadata;
        const unsigned force=((forces&1)?1:0)|((forces&4)?2:0)|((forces&16)?4:0);
        const unsigned permitted=matrix_host?7:available;
        const unsigned expected=(!initial&&(forces&0x42))?0:((wanted|force)&permitted&7);
        assert(result.mScale==bool(expected&1));
        assert(result.mRotation==bool(expected&2));
        assert(result.mTranslation==bool(expected&4));
        ++cases;
    }
    multi._28=nullptr;
    cb.mFlags=0; cb.setHostSRT(&host,&host,&host);
    // Retail followSRT has no initialization for null metadata. Query this
    // branch only with explicit input flags, rather than invoking undefined
    // source behavior or constructing invented production defaults.
    MultiEmitterCallBack::FlagSRT preserved{true,false,true};
    cb.isFollowSRT(&preserved,true);
    assert(preserved.mScale&&!preserved.mRotation&&preserved.mTranslation);
    assert(!MR::Effect::isEffect2D(&multi));
    multi._28=&info;
    for (int order=-2;order<12;++order) {
        info.mDrawOrder=order;
        assert(MR::Effect::isEffect2D(&multi)==(order==6||order==7));
    }
    cb.mFlags=0; multi.forceFollowOn(); multi.forceScaleOn();
    assert(cb.mFlags==0x11);
    multi.forceFollowOff(); assert(cb.mFlags==0x13);
    cb.turnFlagOn(RESET_FOLLOW_CURRENT); cb.resetFollowCurrent(); assert(cb.mFlags==0x13);
    std::cout << "follow_truth_table=" << cases << " null_metadata_initialized_query=pass draw_orders=14 force_api=pass\n";
}

static void colors(MultiEmitter& multi, JPABaseEmitter& emitter) {
    auto& cb=*multi.mCallBack;
    cb.mPrmColor=Color8(211,37,93,5); cb.mEnvColor=Color8(17,241,66,6);
    emitter.mGlobalPrmClr={20,40,60,77}; emitter.mGlobalEnvClr={80,100,120,88};
    multi._2C=0.0F; cb.setColor(&emitter);
    color(emitter.mGlobalPrmClr,211,37,93,77); color(emitter.mGlobalEnvClr,17,241,66,88);
    multi._2C=0.75F;
    for (unsigned value=0;value<=255;++value) {
        const unsigned g=(value+83)&255,b=(value+171)&255;
        emitter.mGlobalPrmClr={static_cast<u8>(value),static_cast<u8>(g),static_cast<u8>(b),77};
        emitter.mGlobalEnvClr={9,22,233,88};
        cb.setColor(&emitter);
        color(emitter.mGlobalPrmClr,value*211/255,g*37/255,b*93/255,77);
        // The original intentionally uses the old *primary* color again.
        color(emitter.mGlobalEnvClr,value*17/255,g*241/255,b*66/255,88);
    }
    multi._2C=0.0F;
    std::cout << "color_direct=pass color_composition=256 alpha_preservation=pass environment_uses_original_primary=pass\n";
}

static void srt(MultiEmitter& multi, AutoEffectInfo& info, JPABaseEmitter& emitter) {
    auto& cb=*multi.mCallBack;
    TVec3f position(10,20,30),rotation(0,0,90),scale(2,3,4);
    cb.setHostSRT(&position,&rotation,&scale); cb._18.set(1,2,3); cb.mFlags=0;
    info.mFlag=0x3f; info.mDrawOrder=0;
    cb.init(&emitter);
    // Original order is rotate offset, then componentwise scale, then add host.
    vector_near(emitter.mGlobalTrs,6,23,42); scale_near(emitter,2,3,4);
    const std::array<TVec3f,5> rotations{{{17.25F,-31.1F,72.3F},{360,-720,1080},{-0.1F,0.02F,-0.03F},{91.125F,183.375F,-271.25F},{0,0,90}}};
    for (const auto& angle:rotations) {
        rotation=angle; cb.init(&emitter);
        const auto expected=expected_rotation(angle);
        for(unsigned r=0;r<3;++r) {
            for(unsigned c=0;c<3;++c) near(emitter.mGlobalRot[r][c],static_cast<float>(expected[r][c]),0.000003F);
            assert(emitter.mGlobalRot[r][3]==0.0F);
        }
    }
    position.set(100,200,300); rotation.set(0,0,90); scale.set(4,5,6);
    cb.execute(&emitter);
    vector_near(emitter.mGlobalTrs,92,205,318); scale_near(emitter,4,5,6);
    multi.setBaseScale(2.0F); cb.execute(&emitter); scale_near(emitter,8,10,12);
    // Initialization applies base scale even when authored scale-follow is off;
    // updates leave that value alone when the three follow channels are off.
    info.mFlag=8; cb.init(&emitter);
    vector_near(emitter.mGlobalTrs,101,202,303); scale_near(emitter,2,2,2);
    position.set(900,900,900); multi.setBaseScale(3.0F); cb.execute(&emitter);
    vector_near(emitter.mGlobalTrs,101,202,303); scale_near(emitter,2,2,2);
    std::cout << "srt_rotated_scaled_offset=pass signed16_rotations=5 live_host_update=pass base_scale_init_vs_follow=pass\n";
}

static void matrix(MultiEmitter& multi, AutoEffectInfo& info, JPABaseEmitter& emitter) {
    auto& cb=*multi.mCallBack;
    Mtx host{{0,-3,0,10},{2,0,0,20},{0,0,4,30}};
    cb.setHostMtx(host); cb._18.set(1,2,3); cb.mFlags=0;
    info.mFlag=0x3f; info.mDrawOrder=0;
    cb.init(&emitter);
    vector_near(emitter.mGlobalTrs,6,23,42); scale_near(emitter,2,3,4);
    near(emitter.mGlobalRot[0][1],-1); near(emitter.mGlobalRot[1][0],1); near(emitter.mGlobalRot[2][2],1);
    TVec3f override_scale(7,8,9);
    cb.mTranslation=&override_scale; cb.setBaseScale(2);
    cb.execute(&emitter);
    // Override affects global dynamics/particle scale, while matrix offset
    // continues using the scale decomposed from the matrix itself.
    vector_near(emitter.mGlobalTrs,6,23,42); scale_near(emitter,14,16,18);
    host[0][3]=100; host[1][3]=200; host[2][3]=300;
    for (int order:{6,7}) {
        info.mDrawOrder=order; cb.execute(&emitter);
        vector_near(emitter.mGlobalTrs,96,203,312);
        near(emitter.mGlobalRot[0][1],1); near(emitter.mGlobalRot[1][0],-1); near(emitter.mGlobalRot[2][2],1);
    }
    cb.forceFollowOff(); host[0][3]=900; cb.execute(&emitter);
    vector_near(emitter.mGlobalTrs,96,203,312); scale_near(emitter,14,16,18);
    std::cout << "matrix_decomposition=pass independent_scale_override=pass 2d_rotation_orders=2 live_matrix_update=pass force_off_preserves_pose=pass\n";
}

int main() {
    const char* disc=std::getenv("SMGPC_REAL_DISC");
    if (!disc||!aurora_dvd_open(disc)) throw std::runtime_error("SMGPC_REAL_DISC must name the supplied actual disc");
    struct DiscGuard { ~DiscGuard(){aurora_dvd_close();} } disc_guard;
    DVDInit(); aurora::g_config.mem1Size=24U*1024U*1024U;
    runtime::DvdFileSystemService dvd({}); runtime::ArchiveMountService mounts(dvd);
    auto heaps=compat::JkrHeapRuntime::create(8U*1024U*1024U);
    const auto initial_free=heaps->root_heap().getFreeSize();
    for (unsigned cycle=0;cycle<2;++cycle) {
        {
            runtime::ParticleResourceOwnership resources(heaps,runtime::ParticleResourceOwnership::default_byte_budget,mounts);
            auto& holder=resources.holder();
            const char* resource_name=nullptr; u16 id=0xffff;
            for (int row=0;row<holder.mParticleNames->getNumEntries();++row) {
                const char* name=nullptr; assert(holder.mParticleNames->getValue(row,"name",&name));
                const std::string_view text(name);
                if (text.size()>=2 && text.size()<40 && text[text.size()-1]>='0' && text[text.size()-1]<='9' && text[text.size()-2]>='0' && text[text.size()-2]<='9') {
                    resource_name=name; id=holder.getUserIndex(name); break;
                }
            }
            assert(resource_name && holder.mResourceMgr->getResource(id));
            const auto resource_free=heaps->root_heap().getFreeSize();
            {
                auto scene=compat::JkrAllocationDomain::create(heaps,512U*1024U);
                compat::JkrAllocationScope allocation(scene);
                JPAEmitterManager manager(32,4,&scene->heap(),1,1);
                manager.entryResourceManager(holder.mResourceMgr,0);
                MultiEmitter multi(resource_name);
                assert(multi.mEmitters.size()==1 && multi.mEmitters[0]._4==id);
                assert(JKRHeap::findFromRoot(multi.mCallBack)==&scene->heap());
                assert(JKRHeap::findFromRoot(multi.mParticleCallBack)==&scene->heap());
                // Real metadata constructor/parser; focused tests modify only
                // this owned copy's public flags and draw-order fields.
                AutoEffectInfo info; info.init(JMapInfoIter(holder.mAutoEffectList,0));
                assert(info.mGroupName && info.mEffectName); multi._28=&info;
                auto* emitter=manager.createSimpleEmitterID(TVec3f(0,0,0),id,0,0,multi.mCallBack,multi.mParticleCallBack);
                assert(emitter && emitter->mpRes==holder.mResourceMgr->getResource(id));
                assert(emitter->mpEmtrCallBack==multi.mCallBack && emitter->mpPtclCallBack==multi.mParticleCallBack);
                flags(multi,info); colors(multi,*emitter); srt(multi,info,*emitter); matrix(multi,info,*emitter);
                manager.forceDeleteEmitter(emitter);
                assert(manager.mFreeEmtrList.getNumLinks()==4);
                // Game's MultiEmitter destructor does not own these pointers.
                // The fixture explicitly destroys complete callback objects
                // before the scene allocation cohort, after emitter retirement.
                delete multi.mCallBack; delete multi.mParticleCallBack;
                multi.mCallBack=nullptr; multi.mParticleCallBack=nullptr;
                std::cout << "cycle=" << cycle << " actual_particle=" << resource_name << " resource_id=" << id << " real_jpa_emitter=pass\n";
            }
            assert(heaps->root_heap().getFreeSize()==resource_free);
        }
        assert(runtime::ParticleResourceOwnership::active()==nullptr);
        assert(mounts.size()==0 && heaps->root_heap().getFreeSize()==initial_free);
    }
    std::cout << "scene_before_resource_retirement=pass heap_restoration=pass fresh_owner_cycles=2\n";
}
