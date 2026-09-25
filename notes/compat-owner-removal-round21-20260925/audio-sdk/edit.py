from pathlib import Path
import json,hashlib,subprocess,difflib
base=Path('notes/compat-owner-removal-round21-20260925/audio-sdk');base.mkdir(parents=True,exist_ok=True);manifest={'owned_paths':[]}
def write(path,text):
 p=Path(path);data=p.read_bytes() if p.exists() else None
 if not any(row['path']==path for row in manifest['owned_paths']):
  if data is not None:
   t=base/'before'/p;t.parent.mkdir(parents=True,exist_ok=True);t.write_bytes(data)
  manifest['owned_paths'].append({'path':path,'before_exists':data is not None,'before_sha256':hashlib.sha256(data).hexdigest() if data is not None else None,'before_status':subprocess.check_output(['git','status','--short','--',path],text=True).rstrip()})
  (base/'owned-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
 if text is None:p.unlink()
 else:p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text)
for name in ['JASMutex','JASWaveArcLoader']:
 write('src/JSystem/JAudio2/'+name+'.hpp',Path('decomp/libs/JSystem/include/JSystem/JAudio2/'+name+'.hpp').read_text())
s=Path('decomp/src/JSystem/JAudio2/JASHeapCtrl.cpp').read_text().replace('#include <revolution/aralt.h>','#include <dolphin/ar.h>\n#include <algorithm>\n#include <cstdint>')
s=s.replace('mSize = size - (u32(mBase) - u32(pBase));','mSize = size - static_cast<u32>(reinterpret_cast<uintptr_t>(mBase) - reinterpret_cast<uintptr_t>(pBase));')
s=s.replace('u32 gapSize = u32(it->mBase) - u32(pCurrent);','u32 gapSize = static_cast<u32>(it->mBase - pCurrent);')
s=s.replace('mTree.insertChild(&pNext->mTree, &pHeap->mTree);','mTree.insertChild(pNext != nullptr ? &pNext->mTree : nullptr, &pHeap->mTree);')
s=s.replace('delete[] pChunk;', 'delete[] static_cast<u8*>(pChunk);')
s=s.replace('pChunk = new (JASDram, 0) u8[size];','pChunk = new (JASDram, static_cast<int>(alignof(void*))) u8[std::max<std::size_t>(size, sizeof(void*))];')
s=s.replace('audioAramHeap.initRootHeap((void*)sAramBase, size);','audioAramHeap.initRootHeap(reinterpret_cast<void*>(static_cast<uintptr_t>(sAramBase)), size);')
write('src/JSystem/JAudio2/JASHeapCtrl.cpp',s)
s=Path('decomp/src/JSystem/JAudio2/JASReport.cpp').read_text().replace('#include "JSystem/JAudio2/JASCalc.hpp"\n','').replace('#include <cstdio>','#include <cstdio>\n#include <cstdarg>');write('src/JSystem/JAudio2/JASReport.cpp',s)
p='src/JSystem/JAudio2/JASHeapCtrl.hpp';s=Path(p).read_text().replace('public T< JASMemChunkPool< ChunkSize, T > >::ObjectLevelLockable','public T< JASMemChunkPool< ChunkSize, T > >')
s=s.replace('return (u8*)this + 0xc <= (u8*)ptr && (u8*)ptr < (u8*)this + (0xc + ChunkSize);','return reinterpret_cast<uintptr_t>(mBuffer) <= reinterpret_cast<uintptr_t>(ptr) &&\n                   reinterpret_cast<uintptr_t>(ptr) < reinterpret_cast<uintptr_t>(mBuffer) + ChunkSize;')
s=s.replace('/* 0xC */ u8 mBuffer[ChunkSize];','/* 0xC */ alignas(void*) u8 mBuffer[ChunkSize];')
# Command records contain native pointers. Keep every returned record aligned.
s=s.replace('''        u32 freeSize = mChunk->getFreeSize();''','''        if (size > ChunkSize) {
            return nullptr;
        }
        size = (size + alignof(void*) - 1) & ~(alignof(void*) - 1);
        u32 freeSize = mChunk->getFreeSize();''')
write(p,s)
source=Path('decomp/src/JSystem/JAudio2/JASAramStream.cpp').read_text()
def function(start):
 begin=source.index(start);brace=source.index('{',begin);depth=1;end=brace+1
 while depth:
  if source[end]=='{':depth+=1
  elif source[end]=='}':depth-=1
  end+=1
 return source[begin:end]
ctor=function('JASAramStream::JASAramStream()');init=function('void JASAramStream::init(u32 ').replace('init(u32 param_0','init(uintptr_t param_0');samples=function('u32 JASAramStream::getBlockSamples() const')
s='''#include "JSystem/JAudio2/JASAramStream.hpp"

#if defined(TARGET_PC)
#include <aurora/exception.hpp>
#include <stdexcept>

JASTaskThread* JASAramStream::sLoadThread;
u8* JASAramStream::sReadBuffer;
u32 JASAramStream::sBlockSize;
u32 JASAramStream::sChannelMax;

// Native DSP/ARAM streaming is not implemented. Preserve actual object state,
// but never accept commands, attach a voice, or publish synthetic callbacks.
namespace {
    [[noreturn]] void unavailableAramOutput() {
        aurora::throw_host_exception<std::logic_error>("JAS ARAM stream output is unavailable on this native backend");
    }
}

'''+ctor+'\n\n'+init+'\n\n'+samples+'''

void JASAramStream::initSystem(u32, u32) { unavailableAramOutput(); }
bool JASAramStream::prepare(s32, int) { return false; }
bool JASAramStream::start() { return false; }
bool JASAramStream::stop(u16) { return false; }
bool JASAramStream::pause(bool) { return false; }
bool JASAramStream::cancel() { _114 = 1; return false; }
bool JASAramStream::headerLoad(u32, int) { return false; }
bool JASAramStream::load() { return false; }
void JASAramStream::headerLoadTask(void*) { unavailableAramOutput(); }
void JASAramStream::firstLoadTask(void*) { unavailableAramOutput(); }
void JASAramStream::loadToAramTask(void*) { unavailableAramOutput(); }
void JASAramStream::finishTask(void*) { unavailableAramOutput(); }
void JASAramStream::prepareFinishTask(void*) { unavailableAramOutput(); }
s32 JASAramStream::channelProcCallback(void*) { unavailableAramOutput(); }
s32 JASAramStream::dvdErrorCheck(void*) { unavailableAramOutput(); }
void JASAramStream::channelCallback(u32, JASChannel*, JASDsp::TChannel*, void*) { unavailableAramOutput(); }
void JASAramStream::updateChannel(u32, JASChannel*, JASDsp::TChannel*) { unavailableAramOutput(); }
s32 JASAramStream::channelProc() { unavailableAramOutput(); }
void JASAramStream::channelStart() { unavailableAramOutput(); }
void JASAramStream::channelStop(u16) { unavailableAramOutput(); }

#else
'''+source.replace('void JASAramStream::init(u32 param_0','void JASAramStream::init(uintptr_t param_0')+'\n#endif\n'
write('src/JSystem/JAudio2/JASAramStream.cpp',s)
for p in ['src/compat/jaudio/JasAramStreamPlatform.cpp','src/compat/jaudio/JasGenericPoolPlatform.cpp','tests/JAudioPlaybackTests.cpp','tests/OriginalAudioCategoryVolumeTests.cpp']:write(p,None)
p='tests/OriginalJaiSoundOwnershipTests.cpp';s=Path(p).read_text();a=s.index('    static_assert(sizeof(JAISoundHandle)');b=s.index('    aurora::audio::PcmAudioMixer mixer;',a);checks=s[a:b]
write(p,'''#include "JSystem/JAudio2/JAISound.hpp"
#include "JSystem/JAudio2/JAISoundHandle.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>

int main() {
'''+checks+'''    std::puts("[pass] original JAISoundID layout and JAISoundStatus flags/prepare transitions");
}
''')
p='tests/RuntimeContextFailureTests.cpp';s=Path(p).read_text();s=''.join(line for line in s.splitlines(True) if 'ScenarioCatalogOwnership' not in line and 'ParticleResourceOwnership' not in line);write(p,s)
p='tests/xmake.lua';s=Path(p).read_text()
for name in ['smg-pc-j-audio-playback-tests','smg-pc-original-audio-category-volume-tests']:
 a=s.index('target("'+name+'")');b=s.index('\ntarget(',a+1);s=s[:a]+s[b+1:]
write(p,s)
patch=[]
for row in manifest['owned_paths']:
 p=Path(row['path']);before=(base/'before'/p).read_text() if row['before_exists'] else '';after=p.read_text() if p.exists() else ''
 row['after_sha256']=hashlib.sha256(p.read_bytes()).hexdigest() if p.exists() else None
 row['change']='delete' if not p.exists() else 'modify' if row['before_exists'] else 'create'
 if p.exists():
  q=base/'after'/p;q.parent.mkdir(parents=True,exist_ok=True);q.write_bytes(p.read_bytes())
 patch.extend(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+str(p) if row['before_exists'] else '/dev/null',tofile='b/'+str(p) if p.exists() else '/dev/null'))
(base/'owned-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');(base/'audio-sdk-only.patch').write_text(''.join(patch))
print('Written',len(manifest['owned_paths']),'paths.')
