# Movement and camera focus

The user redirected the active compatibility goal to movement and camera. NPC,
bunny, Rosalina, and demo-progression expansion are paused. All new behavior
must remain system-wide and source/data-backed; no Gateway-specific movement
or camera recipes.

## Implementation

- Ordinary Gateway previously resolved its authored `s:004e` XZ parallel
  camera once and kept that pose while Mario walked. The spin showcase used
  a custom orbit (550 behind, 180 up, watch offset 80). Both now publish the
  player target to one persistent original camera controller. The custom
  orbit helper is removed.
- `OriginalGameCamera` owns actual `CameraParallel`, `CameraHeightArrange`,
  `CameraMan`, and pose objects. The seven imported Camera source/header
  pairs are byte-identical to root. Actual `reset`, `calc`, round input,
  height arrangement, safe-pose helpers, and Director view conversion execute.
  Native code provides resource/target ownership and host scheduling.
- Resource fields follow CamTranslatorParallel and CameraManGame units and
  ordering. The complete CameraHolder is not constructed: that requires the
  full controller collection. The adapter invokes the original controller's
  setter and original shared helper functions.
- The target snapshot uses MarioActor's original camera translation, up,
  front, last movement, ground, gravity, and jump/rise/drop getters through
  `compat/MarioCameraTarget.hpp`. Camera calculations sample current WPAD
  input before player movement, preserving the original camera/player order.
- WASD maps to the Nunchuk stick; arrows map to the Wii D-pad; C maps to
  Nunchuk C. Movement no longer also presses camera/menu D-pad buttons, and A
  no longer also presses Wii A. Enter/Space/mouse retain action input.
- XZ event cameras also retain an original controller between calculations.
  Director pause freezes their calculation, including vertical/round state.
  Game-camera reactivation requests original reset; start-position toggles
  preserve the controller, and explicit manager reset uses the source seed.
- Original `Mario::getTargetWalkSpeed` and `Mario::decideInertia` replace two
  pre-existing PC substitutions. Missing source-backed state accessors are
  provided outside Game while their large owning translation units remain
  excluded. Live-actor tests exercise parameter priorities and timer behavior.
- Shared math dependencies were recovered against the verified RMGK01 DOL
  in root first, then mirrored. See the separate camera trigonometry note.

## Verification

Source mirror tests pass for the seven original camera source/header pairs.
Player source mirror checks pass for 96 retail source branches and 63 headers.
The Apple Silicon debug build of the showcase and camera/walk targets passed.
The modified Gateway demo, spin-checkpoint, and talk fixtures also compile
with the renamed player bridge; this checkpoint does not claim new runtime
coverage of their longer progression flows.
The test uses actual SDL W/arrow/C events and captures frame 157 after the
display copy with `SMGPC_TEST_SCREENSHOT_PATH`.

Verified on 2026-09-03 with the supplied RMGK01 RVZ:

- Shared math and original camera target-scope/input tests passed.
- Stage-start camera: 13 cases passed, including original vertical state,
  safe pose, deferred reset, target retirement, original start countdown,
  priority/resume, and real-disc XZ event pause/resume.
- Actor-event camera: six cases passed, including explicit player camera
  capability/lifetime and same-XZ re-request preserving original round/height
  state against an uninterrupted controller.
- Shared zone-matrix registry checks passed.
- Real Gateway stand/walk/release passed: 14,521 collision triangles,
  stand prism 4642 (NoSlip/Lawn), 325.684 units of walking, Wait -> Run -> Wait,
  12 Mario draw packets during Run, 60 stable idle frames, actor recreation,
  and stage-lease retirement. Camera watch moved 333.381 units while its
  authored eye offset and FOV remained unchanged. Arrow input left Mario's
  Nunchuk stick neutral.
- `original-camera-walk.png` was captured and visually inspected: Mario is
  visible on the planet in the authored view. The screenshot and raw logs
  remain local, outside version control.

The real start chunk is `s:004e`, `CAM_TYPE_XZ_PARA`, with `num1=0` and
`v_pan_use=0`; that authored chunk disables manual rounding and vertical pan.
Enabled rounding and vertical-state paths are exercised with explicit test
parameters, rather than changing the game's authored data.

Reproduce from `pc-port` with `PATH=/opt/homebrew/opt/llvm/bin:$PATH` for xmake.
Build `smg-pc-mario-gateway-walk-tests`, `smg-pc-stage-start-camera-tests`,
`smg-pc-actor-event-camera-tests`, and `smg-pc-original-camera-runtime-tests`.
Run their binaries under `build/macosx/arm64/debug/`, setting `SMGPC_REAL_DISC`
to the supplied RVZ for real-resource cases. Set `SMGPC_TEST_SCREENSHOT_PATH`
to an absolute PNG path for the walking capture.

## Current limits

The existing player slice retains other pre-existing TARGET_PC replacements,
an inward ground bias and idle latch. Its original jump/action state closure
is incomplete. Restoring two walking functions does not establish full retail
movement parity. CameraParallel now executes the original height/round
algorithm, but normal-play CubeCamera/ground CameraID selection and the full
CameraDirector/CameraManGame lifecycle are not integrated. Non-XZ event camera
types retain their pre-existing native implementations. The Gateway/Rosalina
demo objective is still incomplete.

The player target callback covers normal unbound Mario. The original retained
CameraTargetPlayer lifecycle (bound/Bee modes and demo movement timer) is the
next target closure. `notes/next-camera-selection-20260903/README.md` records
the shared collision-owner provenance and original selection dependencies.
