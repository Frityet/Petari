"""Resolve the reviewed Player/Screen/System files without changing the index.

Reads stages 2 and 3, so this is only replayable while the original merge index
is present. The explicit function choices are recorded in the JSON report.
"""
import hashlib
import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DECOMP = ROOT / "decomp"
REPORT = Path(__file__).with_suffix(".json")
decisions = []


def stage(path, side):
    return subprocess.check_output(["git", "-C", str(DECOMP), "show", f":{side}:{path}"]).decode()


def mask(source):
    return re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                  lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), source, flags=re.S)


def methods(source):
    clean = mask(source)
    result = {}
    pattern = r'^(?=[^\s])([^\n{};]*?\b(?P<name>[A-Za-z_]\w*(?:::[~\w]+)+)\s*\([^;{}]*?\)(?:\s+const)?(?:\s*:[^{}]*?)?\s*)\{'
    for match in re.finditer(pattern, clean, re.M):
        depth = 1
        end = match.end()
        while depth:
            if clean[end] == '{': depth += 1
            elif clean[end] == '}': depth -= 1
            end += 1
        result.setdefault(match['name'], []).append((match.start(), end))
    return result


def chunks(source, name):
    found = methods(source).get(name, [])
    if len(found) != 1:
        raise ValueError((name, len(found)))
    a, b = found[0]
    return source[a:b]


def replace_method(source, name, replacement):
    found = methods(source).get(name, [])
    if len(found) != 1:
        raise ValueError((name, len(found)))
    a, b = found[0]
    return source[:a] + replacement + source[b:]


def publish(path, text, reason, kept=()):
    assert not re.search(r'^(<<<<<<<|=======|>>>>>>>)', text, re.M), path
    text = '\n'.join(line.rstrip() for line in text.splitlines()) + '\n'
    (DECOMP / path).write_text(text)
    decisions.append({"path": path, "decision": reason, "local_methods_retained": list(kept),
                      "stage2_sha256": hashlib.sha256(stage(path, 2).encode()).hexdigest(),
                      "stage3_sha256": hashlib.sha256(stage(path, 3).encode()).hexdigest(),
                      "merged_sha256": hashlib.sha256(text.encode()).hexdigest()})


def upstream(path, reason, keep=()):
    text = stage(path, 3)
    local = stage(path, 2)
    for name in keep:
        text = replace_method(text, name, chunks(local, name))
    publish(path, text, reason, keep)


# Player: retain independently recovered, runtime-exercised matrix/input and
# bind/warp cores. Keep upstream's surrounding improvements and declarations.
for filename, keep in {
    "Mario.cpp": ["Mario::checkForceGrounding", "Mario::fixHeadFrontVecByGravity",
                  "Mario::createDirectionMtx", "Mario::createCorrectionMtx", "Mario::inputStick",
                  "Mario::postureCtrl", "Mario::createAngleMtx", "Mario::getGravityVec"],
    "MarioActor.cpp": ["MarioActor::calcAndSetBaseMtx"],
    "MarioActorOffensiveMsg.cpp": ["MarioActor::attackOrPushSensor", "MarioActor::checkAndTryTrampleAttack",
                                  "MarioActor::tryGetItem", "MarioActor::cylinderPushCheck"],
    "MarioActorRush.cpp": ["MarioActor::beginRush", "MarioActor::endRush"],
    "MarioActorSpecialDraw.cpp": ["MarioActor::calcSpinEffect", "MarioActor::drawSpinEffect"],
    "MarioAnimator.cpp": ["MarioAnimator::update"],
    "MarioDamage.cpp": ["MarioFireRun::update"],
    "MarioWarp.cpp": ["MarioWarp::updateJump", "MarioWarp::start", "MarioWarp::update", "MarioWarp::close"],
}.items():
    upstream("src/Game/Player/" + filename,
             "Upstream unit plus complete local recovered methods; avoids mixing renamed locals or reordered arithmetic inside a method.", keep)

for filename, reason in {
    "MarioActorRushMsg.cpp": "Both sides implement the same target selection/rebind gates; upstream uses coherent helper and parameter names.",
    "MarioBlown.cpp": "Preserve full close behavior at upstream position; remove duplicate MarioActor INIT_NERVE definitions from this unrelated unit.",
    "MarioDamageParalyze.cpp": "Preserve full close behavior at upstream position; remove duplicate MarioActor INIT_NERVE definitions from this unrelated unit.",
    "MarioEffect.cpp": "Upstream retains smoke table and all recovered methods, names effect bitfields, and relies on merged MovingFollowMtx and Color8 initialization.",
    "MarioSwim.cpp": "Retain upstream implementation and water-area naming; local getBlurOffset is now the same inline body in merged MarioSwim.hpp.",
    "MarioTeresa.cpp": "Both sides have complete behavior; upstream retains bodies and merged header owns the local constant keep/notice virtual methods inline.",
    "MarioWait.cpp": "Equivalent timing/weight behavior with upstream named actor predicate and constructor stores.",
    "MatrixControl.cpp": "Equivalent hash, packed-nibble and bit queries with upstream index types and coherent constructor names.",
}.items():
    upstream("src/Game/Player/" + filename, reason)

path = "src/Game/Player/MarioRabbit.cpp"
publish(path, stage(path, 2), "Preserve all local recovered Rabbit bodies and notice; auto-merged header retains their descriptive fields, whereas upstream CPP still uses anonymous offsets. No upstream-only behavior methods exist.")

# Screen: use one recovered message-tag class, and retain the explicit local
# animation binding/locale methods alongside upstream's real LayoutManager.
for path, reason in {
    "include/Game/Screen/CustomTagProcessor.hpp": "Adopt upstream descriptive alpha/tag fields, inherited context type, and matching function table declaration.",
    "src/Game/Screen/CustomTagProcessor.cpp": "Adopt upstream complete tag/text writer implementation, matching its descriptive fields and existing recovered behavior coverage.",
    "include/Game/Screen/ReplaceTagProcessor.hpp": "Adopt namespace-private dispatch implementation and upstream bounded variadic wrapper declaration.",
    "src/Game/Screen/ReplaceTagProcessor.cpp": "Adopt upstream same tag replacement behavior and added ReplaceTagFunction wrapper; dispatch structs remain implementation-private.",
    "src/Game/Screen/LayoutCoreUtil.cpp": "Use upstream mIsInfo spelling matching merged CustomTagProcessor.",
    "include/Game/Screen/LayoutGroupCtrl.hpp": "Use upstream mManager spelling and retain getPane declaration.",
    "src/Game/Screen/PauseMenu.cpp": "Equivalent original pause gates with upstream helper locals, explicit default SE arguments and GET_NERVE spelling.",
}.items():
    upstream(path, reason)

path = "include/Game/Screen/MessageTagSkipTagProcessor.hpp"
publish(path, stage(path, 2), "Retain dedicated recovered MessageEditorMessageTag header and NO_INLINE skipTag; avoid a duplicate class from upstream.")

path = "src/Game/Screen/LayoutGroupCtrl.cpp"
text = stage(path, 3) + "\n" + chunks(stage(path, 2), "LayoutGroupCtrl::getPane") + "\n"
publish(path, text, "Upstream constructor/member naming plus local original getPane iteration omitted upstream.", ["LayoutGroupCtrl::getPane"])

path = "include/Game/Screen/LayoutManager.hpp"
text = stage(path, 3)
text = text.replace("u32 _8;", "void* mGroupCtrlList;").replace("u32 _C;", "MtxPtr mMtxRef;").replace("u32 mChildCount;", "u32 mSubtreeSize;")
text = text.replace("LayoutPaneCtrl* createAndAddGroupCtrl", "LayoutGroupCtrl* createAndAddGroupCtrl")
publish(path, text, "Combine upstream manager owner fields/inlines with local typed pane metadata and precise subtree-count meaning.")

path = "src/Game/Screen/LayoutManager.cpp"
local, text = stage(path, 2), stage(path, 3)
text = text.replace(".mChildCount", ".mSubtreeSize").replace("]._8 = 0;", "].mGroupCtrlList = nullptr;").replace("]._C = 0;", "].mMtxRef = nullptr;")
names = ["LayoutManager::getAnimTransform", "LayoutManager::bindPaneCtrlAnim", "LayoutManager::bindPaneCtrlAnimSub",
         "LayoutManager::unbindPaneCtrlAnim", "LayoutManager::unbindPaneCtrlAnimSub"]
for name in names:
    assert name not in methods(text)
    text += "\n" + chunks(local, name).replace("mPaneInfos", "mPaneInfoList") + "\n"
text = replace_method(text, "LayoutManager::removeUnnecessaryPanes", chunks(local, "LayoutManager::removeUnnecessaryPanes"))
block = local[local.index("namespace {"):local.index("nw4r::lyt::AnimTransform* LayoutManager::getAnimTransform")]
text = text[:text.index("namespace {")] + block + "\n" + text[text.index("namespace {"):]
if '#include "Game/System/Language.hpp"' not in text:
    text = text.replace('#include "Game/System/LayoutHolder.hpp"', '#include "Game/System/Language.hpp"\n#include "Game/System/LayoutHolder.hpp"')
if '#include "Game/Util/HashUtil.hpp"' not in text:
    text = text.replace('#include "Game/Util/FileUtil.hpp"', '#include "Game/Util/FileUtil.hpp"\n#include "Game/Util/HashUtil.hpp"')
publish(path, text, "Union upstream constructor/archive/draw/pane/group/indirect texture owners with local recovered animation binding and nonempty locale pruning.", names + ["LayoutManager::removeUnnecessaryPanes"])

# System: keep ARAM overrides; other stage conflicts are complete equivalent
# functions, type/name migrations or correctly moved inline accessors.
for path, reason in {
    "src/Game/System/AudSystemWrapper.cpp": "Use coherent upstream stopThreads argument and renamed wrapper members; full local function coverage retained.",
    "src/Game/System/GameSequenceProgress.cpp": "Retain full galaxy-move sequence using merged descriptive fields and upstream isCometStar helper.",
    "include/Game/System/LayoutHolder.hpp": "Use actual JKRArcFinder return contract and upstream inline definitions for all local getResOther accessors.",
    "src/Game/System/LayoutHolder.cpp": "Use JKRArcFinder contract and inline-header accessor ownership; preserve original font/resource lookup and recursive archive walking.",
    "src/Game/System/SaveDataFileAccessor.cpp": "Equivalent offset/size/kind extraction with upstream signed loop and early continue; no bounds policy added.",
    "include/Game/System/WPadHVSwing.hpp": "Adopt descriptive channel, threshold, latch, hold and cooldown names.",
}.items():
    upstream(path, reason)

path = "src/Game/System/WPadHVSwing.cpp"
text = stage(path, 3)
text = replace_method(text, "WPadHVSwing::WPadHVSwing", """WPadHVSwing::WPadHVSwing(const WPad* pPad, u32 channel)
    : pPad(pPad), mChannel(channel), mDistanceSwingThreshold(1.0f), mIsSwing(false), mIsSwingLatched(false),
      mSwingLatchedFrames(0), mSwingBelowThresholdFrames(0), mIsTriggerSwing(false), mSwingThreshold(0.4f),
      mSwingDetected(false), mSwingTriggered(false), mSwingHoldFrames(0), mSwingCooldownFrames(0) {
}""")
publish(path, text, "Keep original local complete member initialization while adopting upstream descriptive names; reject upstream pPad=pPad self-assignment bug.", ["WPadHVSwing::WPadHVSwing"])

path = "src/Game/System/Overwrite.cpp"
publish(path, stage(path, 2), "Keep local recovered JKRAram constructor and JKRAramPiece DMA override absent upstream; common audio-thread/exception-pad methods are equivalent.")

path = "src/Game/System/ResourceHolderManager.cpp"
upstream(path, "Retain explicit recovered member-function functors for main-thread dispatch, with upstream surrounding owner code.",
         ["ResourceHolderManager::startCreateResourceHolderOnMainThread", "ResourceHolderManager::startCreateLayoutHolderOnMainThread"])

REPORT.write_text(json.dumps(decisions, indent=2) + "\n")
print("Resolved", len(decisions), "reviewed paths; index untouched.")
