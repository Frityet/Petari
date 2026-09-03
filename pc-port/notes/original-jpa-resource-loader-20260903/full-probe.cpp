#include "resource/JpcResource.hpp"
#include "resource/RarcArchive.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include <JSystem/JKernel/JKRHeap.hpp>
#include <JSystem/JParticle/JPAResourceManager.hpp>
#include <JSystem/JParticle/JPAResource.hpp>
#include <JSystem/JParticle/JPAEmitterManager.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <JSystem/JParticle/JPADynamicsBlock.hpp>
#include <cassert>
#include <iostream>
int main(int argc,char**argv) {
 if(argc!=2)return 2;
 auto archive=std::make_shared<smgpc::resource::RarcArchive>(smgpc::resource::RarcArchive::from_file(argv[1]));
 auto runtime=smgpc::compat::JkrHeapRuntime::create(32*1024*1024);
 auto domain=smgpc::compat::JkrAllocationDomain::create(runtime,16*1024*1024);
 JPAResourceManager* manager;
 {
  auto registration=smgpc::resource::register_jpc_source(archive->file_data("particles.jpc"),archive);
  smgpc::compat::JkrAllocationScope scope(domain);
  manager=new (&domain->heap(),0) JPAResourceManager(archive->file_data("particles.jpc").data(),&domain->heap());
 }
 archive.reset();
 std::weak_ptr<const smgpc::resource::JpcResource> backing=manager->mNativeResource;
 assert(manager->mResNum==3327 && manager->mTexNum==225);
 {
  smgpc::compat::JkrAllocationScope scope(domain);
  JPAEmitterManager emitters(32,4,&domain->heap(),1,1);
  emitters.entryResourceManager(manager,0);
  auto*resource=manager->mpResArr[44];
  auto*emitter=emitters.createSimpleEmitterID(TVec3f(1,2,3),resource->mUsrIdx,0,0,nullptr,nullptr);
  assert(emitter && emitter->mpRes==resource && emitter->mRate==resource->getDyn()->getRate());
  assert(emitter->mGlobalTrs.x==1 && emitter->mGlobalTrs.y==2 && emitter->mGlobalTrs.z==3);
  emitters.forceDeleteEmitter(emitter);
 }
 domain.reset();assert(backing.expired());runtime.reset();
 std::cout<<"Actual original manager loaded 3327 resources and 225 textures; actual emitter construction and heap retirement passed\n";
}
