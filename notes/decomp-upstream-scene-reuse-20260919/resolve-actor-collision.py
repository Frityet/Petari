#!/usr/bin/env python3
"""Explicit per-file reconciliation; reads unchanged merge stages, never stages Git."""
from pathlib import Path
import subprocess, re, json, hashlib
ROOT=Path(__file__).resolve().parents[2]
DECOMP=ROOT/'decomp'
def stage(path,n):
    ref = f':{n}:{path}'
    if path == 'include/Game/LiveActor/MaterialCtrl.hpp':
        ref = ('HEAD' if n == 2 else 'MERGE_HEAD') + ':' + path
    return subprocess.check_output(['git','-C',str(DECOMP),'show',ref],text=True)
def function(s,name):
    pattern=r'(?m)^[ \t]*(?:[\w:<>,*&]+[ \t]+)*'+re.escape(name)+r'\s*\('
    matches=list(re.finditer(pattern,s))
    if len(matches)!=1: raise ValueError((name,len(matches)))
    start=matches[0].start(); begin=s.index('{',matches[0].end()); depth=1; end=begin+1
    while depth:
        if s[end]=='{':depth+=1
        elif s[end]=='}':depth-=1
        end+=1
    return start,end,s[start:end]
def retain(out,old,name):
    a,b,_=function(out,name);_,_,body=function(old,name)
    return out[:a]+body+out[b:]
def append(out,old,name):
    _,_,body=function(old,name)
    return out.rstrip()+'\n\n'+body+'\n'
REASONS={
'include/Game/LiveActor/MaterialCtrl.hpp':'Repair auto-merged duplicate MirrorReflectionMtxSetter class; upstream inline NO_INLINE method already owns addUpdatingTexMtxFromName. Keep the one complete upstream class and all newly added material controllers.',
'include/Game/LiveActor/DynamicJointCtrl.hpp':'Upstream field offset annotations, identical member types.',
'include/Game/LiveActor/ShadowSurfaceCircle.hpp':'Upstream radius offset annotation.',
'include/Game/LiveActor/ShadowSurfaceDrawer.hpp':'Use upstream inline empty destructor, remove old out-of-line duplicate with its cpp.',
'include/Game/LiveActor/ShadowVolumeDrawer.hpp':'Equivalent formatted empty virtual destructor.',
'include/Game/LiveActor/ShadowVolumeModel.hpp':'Use upstream declared destructor and its upstream body owner.',
'include/Game/LiveActor/ShadowVolumeOval.hpp':'Retain explicit recovered destructor declaration/body; adopt upstream member annotation.',
'include/Game/Map/CollisionParts.hpp':'Adopt upstream annotations and u32 area-list declaration matching both recovered bodies (old local header said void).',
'include/Game/MapObj/ChipCounter.hpp':'Upstream exe* declarations replace old direct Nerve executors.',
'include/Game/MapObj/MapPartsRailRotator.hpp':'Upstream enum naming, empty exeDone and member annotations align the full implementation.',
'include/Game/MapObj/MapPartsSeesaw1AxisRotator.hpp':'Use upstream field names, inline isMoving and angle-limit helper with equivalent original behavior.',
'include/Game/MapObj/MapPartsSeesaw2AxisRotator.hpp':'Use upstream field names, inline isMoving and ordinary exeStay instead of direct Nerve body.',
'include/Game/MapObj/PlantGroup.hpp':'Use upstream mHintIndex/mTouchType and explicit matrix constructor parameter; call sites pass nullptr.',
'include/Game/MapObj/StageEffectDataTable.hpp':'Upstream rearranges all existing public APIs, preserving complete table/helper interface.',
'include/Game/MapObj/WarpPod.hpp':'Use upstream field naming and single WarpPodMgr declaration (same header, now before WarpPod).',
'src/Game/Animation/AnmPlayer.cpp':'Upstream shared TVec quaternion header contains setEuler implementation; remove duplicate specialization here.',
'src/Game/AreaObj/CollisionArea.cpp':'Use upstream complete actor/AreaPolygon methods; retain verified hitCheck including negative-sentinel shift guard, plus local DynamicCollisionObj destructor.',
'src/Game/AreaObj/ImageEffectArea.cpp':'Equivalent selection-sort traversal with upstream local names/types.',
'src/Game/AreaObj/LightAreaHolder.cpp':'Equivalent selection-sort traversal and order; upstream direct array access.',
'src/Game/AreaObj/WarpCube.cpp':'Equivalent pairing null/self/ID checks and draw math, reconciled upstream locals.',
'src/Game/Boss/SkeletalFishBaby.cpp':'Upstream J3DJoint getter/include spelling, same joint-index test.',
'src/Game/Boss/SkeletalFishBoss.cpp':'Upstream J3DJoint getter/include spelling, same joint-index test.',
'src/Game/LiveActor/Binder.cpp':'Keep complete verified local bind/sweep/contact/reaction bodies and explicit HitInfo assignment; use upstream surrounding constructor/declarations/constants.',
'src/Game/LiveActor/ClippingActorInfo.cpp':'Equivalent bool test, swap-last erase and find locals.',
'src/Game/LiveActor/ClippingJudge.cpp':'Equivalent viewing-volume six-plane construction with upstream scalar and vector names.',
'src/Game/LiveActor/DisplayListMaker.cpp':'Adopt upstream complete display-list and material-difference methods, including previously missing flag methods.',
'src/Game/LiveActor/DynamicJointCtrl.cpp':'Adopt upstream complete node/controller methods; countdown decrement and branch are equivalent.',
'src/Game/LiveActor/EffectKeeper.cpp':'Upstream relocates floor-code and Binder helper definitions; retain one implementation rather than auto-merge duplicates; callback semantics unchanged.',
'src/Game/LiveActor/HitSensorInfo.cpp':'Equivalent original position/offset and optional host matrix handling with upstream names.',
'src/Game/LiveActor/LodCtrl.cpp':'Equivalent nested guard/LOD-distance decision tree; upstream constructor and methods remain complete.',
'src/Game/LiveActor/MaterialCtrl.cpp':'Adopt newly complete upstream view/projection/Mario-shadow material controllers and renamed mirror fields; addUpdatingTexMtxFromName is already owned by the upstream header, so no out-of-line duplicate.',
'src/Game/LiveActor/MirrorCamera.cpp':'Upstream retains complete S16/F32 resource parsing, reflected view/projection construction and lifecycle; reorganized helper placement.',
'src/Game/LiveActor/RailRider.cpp':'Upstream forwardGoal naming and original accessor; preserve new nearest-point and next-point methods.',
'src/Game/LiveActor/SensorHitChecker.cpp':'Upstream owns relocated SensorGroup once; retain locally verified checkAttack arithmetic grouping.',
'src/Game/LiveActor/ShadowController.cpp':'Use upstream complete controller and new setProjectionPtr; retain verified local direction/projection/length/gravity bodies, avoiding duplicated moved definitions.',
'src/Game/LiveActor/ShadowSurfaceCircle.cpp':'Equivalent original surface circle math and packed alpha 0x80.',
'src/Game/LiveActor/ShadowSurfaceDrawer.cpp':'Upstream Color8 spelling and inline destructor owner.',
'src/Game/LiveActor/ShadowVolumeCylinder.cpp':'Upstream named scale constant and new destructor, retain recovered matrix arithmetic.',
'src/Game/LiveActor/ShadowVolumeLine.cpp':'Retain verified local loadModelDrawMtx arithmetic; upstream remainder unchanged.',
'src/Game/LiveActor/ShadowVolumeOval.cpp':'Retain recovered matrix arithmetic and explicit destructor; upstream remainder and include ownership.',
'src/Game/LiveActor/ShadowVolumeOvalPole.cpp':'Retain recovered matrix arithmetic; preserve upstream named constant and matching helper.',
'src/Game/LiveActor/ShadowVolumeSphere.cpp':'Equivalent fully initialized scale matrix and projection position with upstream locals.',
'src/Game/LiveActor/ViewGroupCtrl.cpp':'Equivalent original group flags, last-match behavior, counts and LOD bindings with upstream variable names.',
'src/Game/Map/CollisionCategorizedKeeper.cpp':'Complete upstream keeper and zone methods preserve recovered encounter, filtering and bound checks; adapt to upstream fixed arrays and descriptive fields.',
'src/Game/Map/CollisionParts.cpp':'Retain verified local query/motion and equal-scale bodies within upstream surrounding lifecycle; adopt complete matching area return type and shared getScale template ownership.',
'src/Game/MapObj/ChipCounter.cpp':'Use complete upstream exe* methods/nerve macros, retain local empty destructor declaration implementation.',
'src/Game/MapObj/MapPartsRailRotator.cpp':'Equivalent wait/done/update behavior with upstream nerve macros, named message/axis constants and destructor owner.',
'src/Game/MapObj/MapPartsSeesaw1AxisRotator.cpp':'Equivalent complete torque/inertia/friction/hipdrop/limit behavior with upstream member names and inline helpers.',
'src/Game/MapObj/MapPartsSeesaw2AxisRotator.cpp':'Equivalent complete gravity/torque/restore and hipdrop behavior, upstream field names and ordinary exeStay.',
'src/Game/MapObj/PlantGroup.cpp':'Complete upstream group/member logic retains original placement, item, shake and clipping behavior; reconcile descriptive fields, constants and nerve macros.',
'src/Game/MapObj/StageEffectDataTable.cpp':'Complete upstream original camera/pad/sound tables and methods; use one table and consistent renamed fields.',
'src/Game/MapObj/WarpPod.cpp':'Retain complete local initPair/initDraw/drawCylinder (upstream path has uninitialized intermediate vectors), rename only member identifiers to upstream declarations.',
}
records=[]
for path,reason in REASONS.items():
    old=stage(path,2); up=stage(path,3); out=up
    if path.endswith('ShadowVolumeOval.hpp'):
        out=out.replace('    ShadowVolumeOval();','    ShadowVolumeOval();\n\n    virtual ~ShadowVolumeOval();')
    if path=='src/Game/AreaObj/CollisionArea.cpp':
        out=retain(out,old,'CollisionArea::hitCheck')
        out=append(out,old,'DynamicCollisionObj::~DynamicCollisionObj')
    if path=='src/Game/LiveActor/Binder.cpp':
        for name in ['bind','findBindedPos','moveAlongHittedPlanes','moveWithCollisionParts','storeCurrentHitInfo','obtainMomentFixReaction','storeContactPlane']:
            out=retain(out,old,'Binder::'+name)
        out=append(out,old,'HitInfo::operator=')
        out=out.replace('#include "Game/Map/CollisionDirector.hpp"','#include "Game/Map/CollisionDirector.hpp"\n#include "Game/Map/HitInfo.hpp"')
    if path=='src/Game/LiveActor/SensorHitChecker.cpp':out=retain(out,old,'SensorHitChecker::checkAttack')
    if path=='src/Game/LiveActor/ShadowController.cpp':
        for name in ['updateDirection','updateProjection','getProjectionLength','isCalcGravity']:
            out=retain(out,old,'ShadowController::'+name)
    if path in ['src/Game/LiveActor/ShadowVolumeCylinder.cpp','src/Game/LiveActor/ShadowVolumeOval.cpp','src/Game/LiveActor/ShadowVolumeOvalPole.cpp']:
        name=Path(path).stem
        out=retain(out,old,name+'::loadModelDrawMtx')
        out=out.replace('#include "Game/LiveActor/'+name+'.hpp"','#include "Game/LiveActor/'+name+'.hpp"\n#include "Game/LiveActor/ShadowController.hpp"\n#include "JSystem/JMath/JMath.hpp"')
        if name=='ShadowVolumeOval':out=append(out,old,name+'::~'+name)
    if path=='src/Game/LiveActor/ShadowVolumeLine.cpp':
        # Its recovered geometry lives in drawShape(), not a model loader.
        out=retain(out,old,'ShadowVolumeLine::drawShape')
        out=out.replace('#include "Game/LiveActor/ShadowVolumeLine.hpp"','#include "Game/LiveActor/ShadowVolumeLine.hpp"\n#include "JSystem/JMath/JMath.hpp"')
    if path=='src/Game/Map/CollisionParts.cpp':
        for name in ['makeEqualScale','checkStrikePoint','checkStrikeBall','checkStrikeBallCore','checkStrikeBallWithThickness','calcCollidePosition','projectToPlane','checkStrikeLine','createAreaPolygonList','createAreaPolygonListArray','calcForceMovePower']:
            out=retain(out,old,'CollisionParts::'+name)
    if path=='src/Game/MapObj/ChipCounter.cpp':out=append(out,old,'ChipCounter::~ChipCounter')
    if path=='src/Game/MapObj/WarpPod.cpp':
        mapping={'_8C':'mJMapIdInfo','_90':'mGroupId','_98':'mEventCameraName','_A0':'mDelay','mArg1':'mVisibilityState','mArg4':'mGrandstarReq','mArg5':'mCameraTime','mArg6':'mGlowColorIndex','_B4':'mPathFlagIndex','_CA':'mArg7','_CB':'mIsInactive'}
        renamed=old
        for a,b in mapping.items():renamed=re.sub(r'\b'+re.escape(a)+r'\b',b,renamed)
        for name in ['initPair','initDraw','drawCylinder']:out=retain(out,renamed,'WarpPod::'+name)
        out=out.replace('#include "Game/MapObj/WarpPod.hpp"','#include "Game/MapObj/WarpPod.hpp"\n#include "Game/Util.hpp"')
    out = '\n'.join(line.rstrip() for line in out.splitlines()) + '\n'
    if any(x in out for x in ['<<<<<<<','>>>>>>>']):raise ValueError(path)
    (DECOMP/path).write_text(out)
    records.append({'path':path,'decision':reason,'local_sha256':hashlib.sha256(old.encode()).hexdigest(),'upstream_sha256':hashlib.sha256(up.encode()).hexdigest(),'merged_sha256':hashlib.sha256(out.encode()).hexdigest()})
(ROOT/'notes/decomp-upstream-scene-reuse-20260919/merge-actor-collision.json').write_text(json.dumps(records,indent=2)+'\n')
