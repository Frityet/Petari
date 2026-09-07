from pathlib import Path
import re, json
root=Path.cwd()
reference=root/'decomp/src/Game/Util/CameraUtil.cpp'
source=reference.read_text()
# The original general parameter is pointer-width in the native port.
source=source.replace('reinterpret_cast< s32 >(pAnimData)', 'reinterpret_cast< intptr_t >(pAnimData)')
if '#include <stdint.h>' not in source: source=source.replace('#include <cstdio>', '#include <cstdio>\n#include <stdint.h>')
reference.write_text(source)
full=root/'src/Game/Util/CameraUtil.cpp'
full_text=full.read_text().replace('reinterpret_cast< s32 >(pAnimData)', 'reinterpret_cast< intptr_t >(pAnimData)')
if '#include <stdint.h>' not in full_text: full_text=full_text.replace('#include <cstdio>', '#include <cstdio>\n#include <stdint.h>')
full.write_text(full_text)

def functions(text,name):
    result=[]
    pattern=re.compile(r'^    (?:[\w:*<>&]+\s+)+\b'+re.escape(name)+r'\([^;]*?\)\s*\{',re.M)
    for m in pattern.finditer(text):
        start=m.start(); depth=1; pos=m.end()
        while depth:
            if text[pos]=='{': depth+=1
            elif text[pos]=='}': depth-=1
            pos+=1
        result.append((start,pos,text[start:pos]))
    return result
names=['startStartPosCamera','endStartPosCamera','isStartPosCameraEnd','isStartAnimCameraEnd','resetCameraMan','pauseOnCameraDirector','pauseOffCameraDirector','declareEventCamera','declareEventCameraAnim','startEventCamera','startEventCameraNoTarget','startEventCameraTargetPlayer','startEventCameraAnim','endEventCamera','endEventCameraAtLanding','isEventCameraActive','isAnimCameraEnd','getAnimCameraFrame','getEventCameraFrames','declareEventCameraProgrammable','startGlobalEventCameraNoTarget','endGlobalEventCamera','setProgrammableCameraParam','setProgrammableCameraParamFovy']
p=root/'src/compat/CameraUtilCompat.cpp'; text=p.read_text(); edits=[]
for name in names:
    actual=functions(text,name); original=functions(source,name)
    assert len(actual)==len(original)>0,(name,len(actual),len(original))
    for (a,b,_),(_,_,body) in zip(actual,original): edits.append((a,b,body))
# This helper belongs to original PlayerUtil, but already has the native provider here.
a,b,_=functions(text,'setCameraTargetToPlayer')[0]
body=functions((root/'decomp/src/Game/Util/PlayerUtil.cpp').read_text(),'setCameraTargetToPlayer')[0][2]
edits.append((a,b,body))
for a,b,body in sorted(edits,reverse=True): text=text[:a]+body+text[b:]
includes=['Game/Camera/CameraAnim.hpp','Game/Camera/CameraContext.hpp','Game/Camera/CameraDirector.hpp','Game/Camera/CameraParamChunk.hpp','Game/Player/MarioAccess.hpp','Game/Util/ObjUtil.hpp','camera/CameraDirectorRuntime.hpp']
for include in includes:
    line='#include "'+include+'"\n'
    if line not in text: text=line+text
# Reference getAnimCameraFrame uses the original camera-type helper.
needle='namespace MR {\n'
pos=text.index(needle)+len(needle)
if not functions(text,'isCameraType'): text=text[:pos]+'\n'+functions(source,'isCameraType')[0][2]+'\n'+text[pos:]
p.write_text(text)
append_names=['getCameraHolder','declareGlobalEventCamera','declareGlobalEventCameraAbyss','declareGlobalEventCameraDead','declareBlackHoleCamera','setGameCameraTargetToPlayer','setGameCameraTarget','startGlobalEventCamera','hasStartAnimCamera','startStartAnimCamera','getStartAnimCameraFrame','endStartAnimCamera','isCameraInterpolatingNearlyEnd','resetCameraLocalOffset','overlayWithPreviousScreen','isSubjectiveCameraOnForObjClipping','getCameraWatchPos','zoomInTargetGameCamera','zoomOutTargetGameCamera','startTalkCamera','endTalkCamera','pauseOnAnimCamera','pauseOffAnimCamera']
p=root/'src/compat/OriginalCameraOwnerUtil.cpp'; text=p.read_text()
assert not functions(text,append_names[0]),'script already applied'
text=text.replace('#include "Game/Camera/CameraRegisterHolder.hpp"','#include "Game/Camera/CameraRegisterHolder.hpp"\n#include "Game/Camera/CameraParamChunk.hpp"\n#include "Game/Camera/CameraPoseParam.hpp"\n#include "Game/Camera/CameraTargetArg.hpp"\n#include "Game/LiveActor/ActorCameraInfo.hpp"')
text+='\nnamespace MR {\n'+ '\n\n'.join(functions(source,name)[0][2] for name in append_names)+'\n}\n'
p.write_text(text)
(root/'notes/original-camera-director-lifecycle-20260907/wrapper-imports.json').write_text(json.dumps({'replaced':names+['setCameraTargetToPlayer'],'added':append_names,'reference':'decomp/src/Game/Util/CameraUtil.cpp','architecture_change':'Animation resource pointer uses intptr_t, original 32-bit Wii ABI unchanged.'},indent=2)+'\n')
