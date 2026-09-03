#include "resource/JpcResource.hpp"
#include "resource/RarcArchive.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "JSystem/JParticle/JPAFieldBlock.hpp"
#include "JSystem/JParticle/JPAEmitter.hpp"
#include "JSystem/JParticle/JPAParticle.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace smgpc;
static bool near(float a,float b){return std::fabs(a-b)<0.00005f;}
int main(int argc,char**argv) {
 if(argc!=2)return 2;
 auto archive=resource::RarcArchive::from_file(argv[1]);resource::JpcResource data(archive.file_data("particles.jpc"));
 auto runtime=compat::JkrHeapRuntime::create(16*1024*1024);auto domain=compat::JkrAllocationDomain::create(runtime,8*1024*1024);
 std::size_t fields=0;
 {
  compat::JkrAllocationScope scope(domain);JPAEmitterWorkData work{};
  PSMTXIdentity(work.mGlobalRot);PSMTXIdentity(work.mRotationMtx);
  for(const auto&r:data.resources())for(const auto&b:r.blocks)if(b.tag==0x464c4431){JPAFieldBlock block(b.bytes.data(),&domain->heap());block.prepare(&work);++fields;}
  JPAFieldBlockData raw{};raw.mDir.set(0,2,0);raw.mPos.set(0,0,2);raw.mMag=3;raw.mMagRndm=1;raw.mVal1=2;
  raw.mFlags=1;JPAFieldBlock air((const u8*)&raw,&domain->heap());air.prepare(&work);JPABaseParticle p{};air.calc(&work,&p);assert(near(p.mVelType0.x,0)&&near(p.mVelType0.y,3)&&near(p.mVelType0.z,0));
  raw.mFlags=4;JPAFieldBlock vortex((const u8*)&raw,&domain->heap());vortex.prepare(&work);
  p=JPABaseParticle{};p.mLocalPosition.set(1,4,0);vortex.calc(&work,&p);assert(near(p.mVelType0.z,2.5f));
  p=JPABaseParticle{};p.mLocalPosition.set(3,4,0);vortex.calc(&work,&p);assert(near(p.mVelType0.z,1));
  raw.mFlags=8;raw.mMag=1.5707963267948966f;JPAFieldBlock spin((const u8*)&raw,&domain->heap());spin.prepare(&work);
  p=JPABaseParticle{};p.mLocalPosition.set(1,0,0);spin.calc(&work,&p);assert(near(p.mVelType0.x,-1)&&near(p.mVelType0.y,0)&&near(p.mVelType0.z,-1));
  raw.mFlags=7;raw.mMag=3;JPAFieldBlock convection((const u8*)&raw,&domain->heap());convection.prepare(&work);
  p=JPABaseParticle{};p.mLocalPosition.set(1,1,0);convection.calc(&work,&p);assert(near(p.mVelType0.x,2.1213203f)&&near(p.mVelType0.y,2.1213203f)&&near(p.mVelType0.z,0));
 }
 domain.reset();runtime.reset();std::cout<<"actual_fields="<<fields<<"; Air, Vortex inner/outer, Spin, Convection numeric checks passed\n";
}
