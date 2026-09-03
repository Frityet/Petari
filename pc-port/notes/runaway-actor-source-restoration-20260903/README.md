# Runaway actor source restoration

Date: 2026-09-03 UTC.

The upstream merge left `RunawayRabbitCollect.cpp` with eight forward
declarations where this branch had previously reconstructed function bodies.
It also replaced `RunawayRabbit.hpp` member names while retaining the older
`RunawayRabbit.cpp` implementation. This work restores the collector and makes
the rabbit source refer to the current upstream declarations. It does not
enable either actor in the PC placement factory or copy them into the PC
`Game` source glob.

The decompilation instructions used are `AGENT_DECOMP_GUIDE.md` and
`pc-port/AGENTS.md`; no root `AGENTS.md` is present in this checkout. Source
changes are confined to the two root NPC sources and their root headers.

## Provenance and scope

Historical source revision:
`96e5ef0decce22e5bfd7d0ee876fb15ac80a725b`.

| Historical source | SHA-256 |
| --- | --- |
| `src/Game/NPC/RunawayRabbitCollect.cpp` | `525309e25a63ec76a9d24a3a140c56b431fab4d7b05512796180ac29ed31e236` |
| `src/Game/NPC/RunawayRabbit.cpp` | `904c4cfbe200236db80ec9e63b357b95f2b7cdf575f72954c7b7a485b37bb868` |

The rabbit source before this change had exactly the second hash. Its changes
are identifier substitutions and a utility-header include correction. The field
renames are supported by matching offsets in the
historical and current headers. Both headers gain the destructor declaration
needed by their existing/restored out-of-line definition; current member
types, accessibility, ordering, documentation, and array sizes remain intact.

The collector retains its current constructor, nerve declarations, and all
eight BGM state/timing constants. Restored calls use the already-named timing
constants with the same 60/30/90/120 frame values. Its broad utility include is
replaced by the concrete headers used by the restored functions. Its restored
function bodies otherwise match the historical implementation after the
correspondence below, including the final `isValidSwitchA` query and original
three-element comment-selection loop. The current four-byte `_B0` declaration
is preserved.

The earlier note at
`../runaway-rabbit-collect-decomp-20260806T190520Z/README.md` records a historical
99.64343% RMGK02 objdiff result. That result was **not rerun here**. Current
verification establishes source correspondence and host compilation only;
there is no new target-object matching or runtime claim.

## Source correspondence

Offsets below are the documented original 32-bit layout, not host offsets.

| Collector offset | Historical name | Current name |
| --- | --- | --- |
| 0x90 | `mRabbits` | `mRabbit` |
| 0x94 | `mRabbitCount` | `mRabbitNum` |
| 0x98 | `mTicos` | `mTico` |
| 0x9C | `mTicoCount` | `mTicoNum` |
| 0xA0 | `mAppearedTicoCount` | `_A0` |
| 0xA4 | `mCaughtRabbitCount` | `_A4` |
| 0xB0 | `mHasAppearedTico` | `_B0` |

| Rabbit offset | Historical name | Current name |
| --- | --- | --- |
| 0x8C | `mRunawayState` | `mStateRunaway` |
| 0x90 | `mBlowDamageState` | `mStateBlowDamage` |
| 0x94 | `mCollector` | `mCollect` |
| 0x9C | `mSpotLight` | `mSpotMarkLight` |
| 0xA4 | `mQuat` | `_A4` |
| 0xB4 | `mFrontVec` | `_B4` |
| 0xC0 | `mBindQuat` | `_C0` |
| 0xD0 | `mBindFrontVec` | `_D0` |
| 0xE0 | `mGroupId` | `mObjArg0` |
| 0xE8 | `mMessageId` | `mObjArg1` |
| 0xF0 | `_F0` | `mNotCaughtableTimer` |
| 0xF4 | `mIsCaughtable` | `_F4` |
| 0xF5 | `mHasAppearedTico` | `_F5` |
| 0xF8 | `mRunawayDistance` | `mObjArg3` |

## Verification

`verify_source_correspondence.py` checks the recorded Git object hashes, all
collector bodies from `init` through the destructor, the complete rabbit
source, and the exact permitted header changes. The collector comparison
ignores formatting whitespace outside quoted literals; the rabbit comparison
is byte-for-byte after renaming and the documented include correction. All checks pass.

`RunawayRabbitCollect-syntax-command.json` records a syntax-only invocation
derived from the actual `AreaObj.cpp` compile database entry. With LLVM 23 and
the current PC headers first, plus root game/JSystem headers as fallback, the
restored collector compiles successfully. Its log contains only three existing
missing-override warnings from `LiveActor.hpp`.

The similarly isolated rabbit compile reaches 11 errors in source-facing
headers or host compatibility support, with no remaining member-name errors.
The exact initial frontier is recorded in `RunawayRabbit-syntax.log`; it is an
audit snapshot, and parallel compatibility work may make it stale. No Xmake
configuration or production target was changed by this subtask.

## Immediate dependency audit

The 2026-08-06 integration note is stale in two useful respects:
`RunawayTico.cpp` now supplies its guide/demo/comment bodies, including all six
methods called directly by the collector, and `TrickRabbitUtil.cpp` now defines
`createRabbitFootPrint`. `Tico.cpp` and `NPCActor.cpp` also contain their actor
implementations. No additional missing collector dependency body was restored
in this subtask.

The obsolete `TrickRabbit.hpp` include was corrected to `TrickRabbitUtil.hpp`,
which owns the utility namespace. The remaining host probe also
needs declarations/support for `BaseMatrixFollowValidater`,
`addBaseMatrixFollowTarget`, actor-state helpers, star-pointer-at-joint setup,
`TPos3f::setQuat`, the two-argument quaternion `slerp`, talk branch navigation,
and `tryTalkForceWithoutDemoMarioPuppetableAtEnd`. General compatibility
implementations and explicit dependency headers should resolve these; actor
state-machine or physics substitutions would change the reconstructed source.

Completing these source-facing APIs, linking the dependency stack, and testing
real child placements/catch/demo progression are still separate integration
gates. The successful collector syntax check does not establish runnable bunny
chase behavior.

## Utility include correction

Replaced the obsolete `TrickRabbit.hpp` include with `TrickRabbitUtil.hpp`,
which owns the utility namespace called by this source. The source
correspondence check explicitly accounts for this compile-only correction.
No class from `TrickRabbit.hpp` is used by `RunawayRabbit.cpp`.
