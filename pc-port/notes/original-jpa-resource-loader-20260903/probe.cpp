#include "resource/JpcResource.hpp"
#include "resource/RarcArchive.hpp"
#include "render/effects/EffectResource.hpp"
#include <JSystem/JParticle/JPADynamicsBlock.hpp>
#include <JSystem/JParticle/JPABaseShape.hpp>
#include <JSystem/JParticle/JPAChildShape.hpp>
#include <JSystem/JParticle/JPAExtraShape.hpp>
#include <JSystem/JParticle/JPATexture.hpp>
#include <cassert>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
using namespace smgpc::resource;
static std::uint32_t be32(std::span<const std::uint8_t> b,std::size_t o) {
 return (std::uint32_t(b[o])<<24)|(std::uint32_t(b[o+1])<<16)|(std::uint32_t(b[o+2])<<8)|b[o+3];
}
int main(int argc,char**argv) {
 try {
  if(argc!=2)return 2;
  auto archive=std::make_shared<RarcArchive>(RarcArchive::from_file(argv[1]));
  auto bytes=archive->file_data("particles.jpc");
  std::ofstream output(std::string(argv[1])+".jpc",std::ios::binary); output.write((const char*)bytes.data(),bytes.size());output.close();
  auto metadata=smgpc::render::effects::EffectResourceLibrary::from_archive(*archive);
  std::shared_ptr<const JpcResource> retained;
  const void* raw=bytes.data();
  {
   auto first=register_jpc_source(bytes,archive);
   auto second=register_jpc_source(bytes,archive);
   retained=resolve_jpc_source(raw);
   assert(retained==resolve_jpc_source(raw));
  }
  bool rejected=false;try{(void)resolve_jpc_source(raw);}catch(const std::runtime_error&){rejected=true;}assert(rejected);
  archive.reset(); // all host data remains valid after source and aliases retire
  std::map<std::uint32_t,std::size_t> tags;
  std::size_t scalar_checks=0;
  for(const auto& r:retained->resources()) {
   const auto* old=metadata.resource_by_user_index(r.user_index);assert(old);
   assert(r.blocks.size()==old->block_count && r.field_count==old->field_block_count && r.key_count==old->key_block_count);
   for(const auto& b:r.blocks) {
    ++tags[b.tag];assert(reinterpret_cast<std::uintptr_t>(b.bytes.data())%16==0);
    auto source=retained->source_bytes().subspan(b.source_offset,b.bytes.size());
    if(b.tag==0x42454d31) {
     auto* p=reinterpret_cast<const JPADynamicsBlockData*>(b.bytes.data());
     assert(old->dynamics && p->mRate==old->dynamics->rate && p->mLifeTime==old->dynamics->lifetime);
     assert(p->mEmitterRot.x==old->dynamics->emitter_rotation.x && p->mResUserWork==old->dynamics->resource_user_work);
     assert(p->mEmitterScl.z==old->dynamics->emitter_scale.z && p->mMoment==old->dynamics->moment);
     // Every original dynamics word, including floats, must preserve its bits.
     for(std::size_t i=4;i<0x68;i+=4) {std::uint32_t value;std::memcpy(&value,b.bytes.data()+i,4);assert(value==be32(source,i));++scalar_checks;}
    }
    if(b.tag==0x42535031) {
     auto*p=reinterpret_cast<const JPABaseShapeData*>(b.bytes.data());assert(old->base_shape);
     assert(p->mBaseSizeX==old->base_shape->base_size_x && p->mBlendModeCfg==old->base_shape->blend_mode_config);
     assert(p->mClrPrm.r==old->base_shape->prm_color[0] && p->mClrEnv.a==old->base_shape->env_color[3]);
     if(p->mFlags&0x01000000) {auto*f=reinterpret_cast<const float*>(b.bytes.data()+0x34);for(int j=0;j<10;++j)assert(std::bit_cast<std::uint32_t>(f[j])==be32(source,0x34+j*4));}
    }
    if(b.tag==0x53535031) {auto*p=reinterpret_cast<const JPAChildShapeData*>(b.bytes.data());assert(old->child_shape);assert(p->mLife==old->child_shape->lifetime);}
    if(b.tag==0x54444231) {auto*p=reinterpret_cast<const std::uint16_t*>(b.bytes.data()+8);for(std::size_t j=0;j<r.texture_reference_count;++j)assert(p[j]==old->texture_indices[j]);}
   }
  }
  for(std::size_t i=0;i<retained->textures().size();++i) {
   const auto&b=retained->textures()[i];auto*p=reinterpret_cast<const JPATextureData*>(b.bytes.data());const auto*old=metadata.texture(i);assert(old);
   assert(p->mResTIMG.mWidth==old->width&&p->mResTIMG.mHeight==old->height&&p->mName==old->name);
   const auto raw=retained->source_bytes().subspan(b.source_offset,b.bytes.size());
   assert(std::memcmp(raw.data()+0x40,b.bytes.data()+0x40,b.bytes.size()-0x40)==0);
  }
  auto bad=std::vector<std::uint8_t>(retained->source_bytes().begin(),retained->source_bytes().end());
  std::size_t invalid=0;
  auto reject=[&](std::span<const std::uint8_t>b){try{JpcResource r(b);}catch(const std::runtime_error&){++invalid;return;}throw std::logic_error("malformed source accepted");};
  reject(std::span<const std::uint8_t>(bad).first(15));
  auto save=bad[4];bad[4]=0;reject(bad);bad[4]=save;
  save=bad[0x14];bad[0x14]^=0x7f;reject(bad);bad[0x14]=save;
  // The SDK user index is an identity, not its record's array position.
  bad[0x10]=0xfd;bad[0x11]=0xe8;
  {JpcResource reidentified(bad);assert(reidentified.resources()[0].user_index==65000);}
  bad[0x10]=retained->source_bytes()[0x10];bad[0x11]=retained->source_bytes()[0x11];
  reject(std::span<const std::uint8_t>(bad).first(bad.size()-128));
  auto offset=retained->resources()[0].blocks[0].source_offset+4;
  for(int i=0;i<4;++i)bad[offset+i]=0xff;reject(bad);
  std::cout<<"resources="<<retained->resources().size()<<" textures="<<retained->textures().size()<<" dynamics_word_checks="<<scalar_checks<<" malformed_rejections="<<invalid<<"\n";
  for(auto[t,n]:tags){char s[]{char(t>>24),char(t>>16),char(t>>8),char(t),0};std::cout<<s<<"="<<n<<" ";}std::cout<<"\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}
}
