from pathlib import Path

def edit(path, replacements):
 p=Path(path);s=p.read_text()
 for a,b in replacements:
  assert a in s,(path,a)
  s=s.replace(a,b)
 p.write_text(s)

def include(path,text):
 p=Path(path);s=p.read_text();line=text+'\n'
 if line not in s:
  s=line+s;p.write_text(s)

edit('src/Game/AudioLib/AudSoundObject.cpp',[
 ('    AudWrap::getSoundObjHolder()->remove(this);','    if (mNativeHolder != nullptr) {\n        mNativeHolder->remove(this);\n    }\n    delete[] mHashDatas;\n    mHashDatas = nullptr;'),
 ('bool AudSoundObject::isLimitedSound(JAISoundID soundID) {','bool AudSoundObject::isLimitedSound(JAISoundID soundID) {\n    if (AudSystemWrapper::isOutputDisabled()) {\n        return true;\n    }')])
for i in ['#include "Game/System/AudSystemWrapper.hpp"','#include "Game/AudioLib/AudSystem.hpp"','#include "Game/AudioLib/AudWrap.hpp"','#include <JSystem/JAudio2/JASTrack.hpp>']:
 include('src/Game/AudioLib/AudSoundObject.cpp',i)
p=Path('src/Game/AudioLib/AudSoundObject.cpp');s=p.read_text();s+='\nbool AudSoundObject::isEnableStartSound(JAISoundID soundID) {\n    return !AudSystemWrapper::isOutputDisabled() && AudWrap::getSystem()->isEnableStartSound(soundID);\n}\n';p.write_text(s)

for file, signatures in {
 'src/Game/AudioLib/AudBgm.cpp':['JAISoundHandle* AudSingleBgm::start(u32 soundID, bool lock) {','JAISoundHandle* AudMultiBgm::start(u32 soundID, bool lock) {','JAISoundHandle* AudMultiBgm::prepare(u32 id) {'],
 'src/Game/AudioLib/AudBgmMgr.cpp':['JAISoundHandle* AudBgmMgr::start(s32 bgmIndex, u32 soundID, bool lock) {'],
 'src/Game/RhythmLib/AudMeObject.cpp':['AudMeHandle* AudMeObject::startMe(u32 id) {']}.items():
 include(file,'#include "Game/System/AudSystemWrapper.hpp"')
 edit(file,[(sig,sig+'\n    if (AudSystemWrapper::isOutputDisabled()) {\n        return nullptr;\n    }') for sig in signatures])

# The original animation advances its cue indices before attempting a start.
edit('src/Game/AudioLib/AudAnmSoundObject.cpp',[
 ('getHandleUserData((u32)sound)','getHandleUserData(reinterpret_cast<uintptr_t>(sound))'),
 ('setUserData((u32)sound)','setUserData(reinterpret_cast<uintptr_t>(sound))'),
 ('    if (!pStarter->startSound(soundID, handle, &rPos)) {','    if (pStarter == nullptr || !pStarter->startSound(soundID, handle, &rPos)) {')])
include('src/Game/AudioLib/AudAnmSoundObject.cpp','#include <JSystem/JAudio2/JAISoundStarter.hpp>')
edit('src/JSystem/JAudio2/JAISoundHandles.hpp', [('getHandleUserData(u32)', 'getHandleUserData(uintptr_t)')])
edit('src/JSystem/JAudio2/JAISoundHandles.cpp',[('getHandleUserData(u32 addr)', 'getHandleUserData(uintptr_t addr)')])
edit('src/JSystem/JAudio2/JAUSoundAnimator.hpp', [('    JAISound* getSound(int index);','    JAISound* getSound(int index) {\n        return mHandles->getHandle(index)->getSound();\n    }')])
include('src/JSystem/JAudio2/JAUSoundAnimator.cpp','#include "resource/BasResource.hpp"')
edit('src/JSystem/JAudio2/JAUSoundAnimator.cpp',[
 ('JAUSoundAnimator::JAUSoundAnimator(JAISoundHandles* pHandles) : mSoundAnimation(nullptr) {','JAUSoundAnimator::JAUSoundAnimator(JAISoundHandles* pHandles)\n    : mSoundAnimation(nullptr), mLoopSoundIndex(0), mLifeTime(0.0f), mLoopStartSoundIndex(0), mLoopEndSoundIndex(0), mLoopStartFrame(0.0f),\n      mLoopEndFrame(0.0f), mTime(0) {'),
 ('void JAUSoundAnimator::startAnimation(const JAUSoundAnimation* pAnimation, bool reversed, f32 loopStartFrame, f32 loopEndFrame) {','void JAUSoundAnimator::startAnimation(const JAUSoundAnimation* pAnimation, bool reversed, f32 loopStartFrame, f32 loopEndFrame) {\n    pAnimation = smgpc::resource::resolve_bas_animation(pAnimation);')])

edit('src/JSystem/JAudio2/JAUSoundObject.cpp',[
 ('JAUSoundObject::JAUSoundObject(TVec3f* pPos, u8 numHandles, JKRHeap* pHeap)','JAUSoundObject::JAUSoundObject() : JAUSoundObject(nullptr, 0, nullptr) {}\n\nJAUSoundObject::JAUSoundObject(TVec3f* pPos, u8 numHandles, JKRHeap* pHeap)'),
 ('    setPos(*mPos);','    if (mPos != nullptr) {\n        setPos(*mPos);\n    }'),
 ('    JAISoundStarter* starter = JASGlobalInstance< JAISoundStarter >::getInstance();','    JAISoundStarter* starter = JASGlobalInstance< JAISoundStarter >::getInstance();\n    if (starter == nullptr) {\n        return nullptr;\n    }'),
 ('    JAISeMgr* seMgr = JASGlobalInstance< JAISeMgr >::getInstance();','    JAISeMgr* seMgr = JASGlobalInstance< JAISeMgr >::getInstance();\n    if (seMgr == nullptr) {\n        return nullptr;\n    }'),
 ('    JAISoundHandle* handle = &mHandles[index];\n    JASGlobalInstance< JAISoundStarter >::getInstance()->startSound(soundID, handle, mPos);','    JAISoundStarter* starter = JASGlobalInstance< JAISoundStarter >::getInstance();\n    if (starter == nullptr) {\n        return nullptr;\n    }\n    JAISoundHandle* handle = &mHandles[index];\n    starter->startSound(soundID, handle, mPos);'),
 ('    JAISoundInfo* info = JASGlobalInstance< JAISoundInfo >::getInstance();','    JAISoundInfo* info = JASGlobalInstance< JAISoundInfo >::getInstance();\n    if (info == nullptr) {\n        return nullptr;\n    }')])
