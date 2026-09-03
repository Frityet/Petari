# Original Mario state lifecycle

This checkpoint supplies the real base `MarioState` virtual table and the
original Mario status-stack operations. It does not install partial Wall,
Hang, Swim, or Wait objects, or activate the recovered full `Mario::update`.

## Source and binary evidence

The six missing root base methods were recovered from the verified RMGK01
revision-0 DOL (`25c5959534b3c21246c6c7e42021b916b41fb578`) and added to
`src/Game/Player/MarioState.cpp` before the existing nerve declarations:

| Method | Retail address | Exact behavior |
| --- | --- | --- |
| `init` | `0x802CEF24` | return without work |
| `notice` | `0x802CEF1C` | return false |
| `keep` | `0x802CEF14` | return true |
| `hitPoly` | `0x802CEF10` | return without work |
| `getBlurOffset` | `0x802C43B0` | return positive zero |
| `draw3D` | `0x802AEC5C` | return without work |

These empty bodies and constant returns are actual retail implementations.
The five other base defaults already existed in root `MarioDamage.cpp`
(`start`, `close`, `update`: true) and `MarioActorDefensiveMsg.cpp`
(`hitWall`: no work; `passRing`: false). All eleven defaults compile to the
retail instruction sequence; the blur load's SDA2 relocation is checked
against the actual positive-zero word at `0x806BF7EC`.

The original compiler also produces a **100% objdiff match** for all ten
existing lifecycle/query methods: constructor, `proc`, `sendStateMsg`,
`updatePosture`, state `postureCtrl`, `changeStatus`, `closeStatus`,
`getNoticedStatus`, `getCurrentStatus`, and `isStatusActive`.

The original State table at `0x805CB588` and Hang table at `0x805CBB98` each
contain sixteen method slots after two zero words. Every original-compiler
relocation agrees with its retail slot. Hang overrides exactly `start`,
`close`, `update`, `notice`, and `postureCtrl`. Its root and PC header now
declare those existing methods, plus the three already defined nonvirtual
methods `recordWallPolygon`, `recordHangNorm`, and `tryClimb`. No fields were
added and no method algorithms changed.

The full root Hang translation unit still has an unrelated pre-existing
`Mario::fixHangDir` return-type declaration mismatch. The header proof
therefore compiles the unchanged Hang constructor and `start` bodies in a
standalone evidence unit. It does not claim that full Hang currently builds.

Run the original-only verifier from the repository root:

```sh
python3 pc-port/notes/original-mario-state-lifecycle-20260903/verify-source.py
```

`source-evidence.json` records the source body hashes, default instruction
bytes, complete vtable maps, and match percentages. Generated PPC objects,
compiler commands/logs, and full objdiff output are under
`build/original-mario-state-lifecycle-20260903/`. The existing verified retail
split is reused from `build/mario-update-restoration-20260903/retail/`.

## Native ownership and scope

`compat/MarioStateCompat.cpp` contains nineteen unchanged root method bodies.
It includes no actor-name conditions, substitute state implementations, or
native transition rules. `getCurrentStatus` and `isStatusActive` remain in
the existing `MarioStateAccessCompat.cpp`: ordinary Game archive consumers
can use these field-only queries without pulling the optional native player
and `MarioModule` implementation. Their exact root correspondence is also
verified. The previous PC-only `MarioState::draw3D` definition in `Mario.cpp`
was retired to avoid duplicate ownership.

Original `notice=false` removes an old base state before the next starts.
When a state handles Update successfully, later retained states get Keep.
A rejected Update closes that state and gives the saved successor a chance
to handle Update. Close unlinks first, clears its link, and uses the original
`_10` flag to prevent recursive invocation of the close callback. These
details are preserved, including the original caller preconditions: this
checkpoint does not add new exception handling or make `closeStatus` safe
for a pointer that is not in the active stack.

The original posture dispatch bodies call the currently linked
`Mario::postureCtrl`; this checkpoint does not change that existing PC
posture provider. It also leaves construction of the actual gameplay states
and the full animation/collision/gravity dependencies for a later coherent
activation.

## Runtime regression

`tests/OriginalMarioStateTests.cpp` is invoked by the existing real-disc
`smg-pc-mario-gateway-walk-tests` fixture after actual Mario initialization.
Both owner fields (`_97C`, `_980`) are restored through RAII, including on a
test failure. It does not cast incomplete storage into a Mario object.

Coverage includes all eleven base defaults through virtual dispatch,
constructor ownership/linkage, base Notice rejection, new-state identity
during Notice, retained stack order, duplicate requests, Update/Keep
sequencing, rejected Update and Start, recursive Close suppression,
non-head splicing, rejected Keep, and close-all. Recording subclasses exist
only in the test to observe callbacks; they never supply production
Wall/Hang/Swim behavior. The gameplay fixture continues afterward to check
that the original owner fields were restored and normal movement remains
usable.

Native build and runtime results are recorded in the adjacent log files.

The first native build passed. Its run stopped before the new state helper,
at the existing camera accessor test's exact-one normalized-up assertion.
The parallel source-correct `MR::normalize` import now calls the SDK
`PSVECNormalize`, whereas the previous host implementation used division.
The original SDK graph (`src/RVL_SDK/mtx/vec.c`, retail `0x804B9050`) produces
`0x3F7FFFFF` for both tested axis inputs, `(0,2,0)` and `(0,0,1)`:
`frsqrte(4) = 0.499908447265625`, the single-precision Newton step gives
`0.4999999701976776`, and multiplying by two gives `0.9999999403953552`.
The test now checks this independent exact float witness against a direct
SDK evaluation, then compares the actual camera output to it. All raw
front/side/up getter assertions retain their exact expectations. No camera
or math production behavior was changed to satisfy the test.

The corrected build also passed. The real-disc rerun passed the camera
accessors, original camera-target tests, and all four blocks of the new
state lifecycle helper, then continued into the existing movement checks.
It subsequently failed the release-ground check at frame 203: speed/band
were zero, no support planes remained, and the diagnostic found contacts at
radius multiplier 1.3 but not 1.25. Full output is in
`native-runtime-normalization-expectation.log`. The concurrent collision
tranche owns that remaining regression; this note does **not** claim the
whole walk executable passed. No movement thresholds or state algorithms
were adjusted to hide the failure.
