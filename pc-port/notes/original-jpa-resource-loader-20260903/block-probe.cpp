#include "resource/JpcResource.hpp"
#include "resource/RarcArchive.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include <JSystem/JParticle/JPABaseShape.hpp>
#include <JSystem/JParticle/JPAChildShape.hpp>
#include <JSystem/JParticle/JPADynamicsBlock.hpp>
#include <JSystem/JParticle/JPAExTexShape.hpp>
#include <JSystem/JParticle/JPAExtraShape.hpp>
#include <JSystem/JParticle/JPAKeyBlock.hpp>
#include <JSystem/JParticle/JPATexture.hpp>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
using namespace smgpc::resource;
int main(int argc,char**argv) {
 if(argc!=3)return 2;
 auto archive=RarcArchive::from_file(argv[1]);JpcResource data(archive.file_data("particles.jpc"));
 auto runtime=smgpc::compat::JkrHeapRuntime::create(16*1024*1024);
 auto domain=smgpc::compat::JkrAllocationDomain::create(runtime,8*1024*1024);
 std::ifstream f(argv[2],std::ios::binary);std::vector<unsigned char>expected((std::istreambuf_iterator<char>(f)),{}),colors;
 std::size_t constructed=0,tables=0,keyValues=0;
 {
  smgpc::compat::JkrAllocationScope scope(domain);
  for(const auto&r:data.resources()) for(const auto&b:r.blocks) {
   const auto*p=b.bytes.data();
   switch(b.tag) {
   case 0x42454d31: {JPADynamicsBlock block(p);assert(block.mpData==(const JPADynamicsBlockData*)p);++constructed;break;}
   case 0x42535031: {
    JPABaseShape block(p,&domain->heap());assert(block.mpData==(const JPABaseShapeData*)p);++constructed;
    for(bool env:{false,true}) if(env?block.isEnvAnm():block.isPrmAnm()) {
     ++tables;for(int frame=0;frame<=block.getClrAnmMaxFrm();++frame) {
      GXColor color;if(env)block.getEnvClr(frame,&color);else block.getPrmClr(frame,&color);
      smgpc::compat::JkrHostAllocationScope host;
      colors.insert(colors.end(),{color.r,color.g,color.b,color.a});
     }
    }
    break;
   }
   case 0x45535031: {JPAExtraShape block(p);assert(block.getScaleInTiming()==((const JPAExtraShapeData*)p)->mScaleInTiming);++constructed;break;}
   case 0x53535031: {JPAChildShape block(p);assert(block.mpData==(const JPAChildShapeData*)p);++constructed;break;}
   case 0x45545831: {JPAExTexShape block(p);assert(block.mpData==(const JPAExTexShapeData*)p);++constructed;break;}
   case 0x4b464131: {
    JPAKeyBlock block(p);assert(block.mDataStart==p);++constructed;
    const auto*keys=(const float*)(p+0xc);
    for(std::size_t i=0;i<p[9];++i) {assert(block.calc(keys[i*4])==keys[i*4+1]);++keyValues;}
    break;
   }
   }
  }
  for(const auto&b:data.textures()) {JPATexture texture(b.bytes.data());assert(texture.mpData==(const JPATextureData*)b.bytes.data());++constructed;}
 }
 assert(colors==expected);
 domain.reset();runtime.reset();
 std::cout<<"Actual original block constructors="<<constructed<<" color_tables="<<tables<<" exact_retail_color_bytes="<<colors.size()<<" authored_key_values="<<keyValues<<"; CPU-only heap lifetime passed\n";
}
