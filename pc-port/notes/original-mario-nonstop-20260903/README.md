# Original MarioModule animation attributes — 2026-09-03

Recovered root-first in `src/Game/Player/MarioModule.cpp`:

| Method | Retail range | Exact fully relocated instructions |
| --- | --- | ---: |
| changeAnimationNonStop(const char*) | 0x802E8F84 / 0x6C | 27 |
| changeAnimationWithAttr(const char*, u32) | 0x802E8FF0 / 0x68 | 26 |

Both retain the actor `_B90` animation-change guard. A non-null name goes through
the existing original changeAnimation(name, nullptr). NonStop then changes the
actual lower XanimePlayer frame controller's attribute from 0 to 1 only;
WithAttr writes the caller's attribute through the original u8 setter. Neither
constructs or substitutes an animator or frame controller, and a null animation
name does not bypass the attribute operation.

`verify.py` compiles complete root MarioModule with configured GC/3.0a3 flags,
compares normalized instructions and relocations to the retail object, resolves
the relocation targets and compares all 53 actual instruction words to the
supplied DOL. All match. The recovered NonStop method also had a separate 100%
objdiff check before adding its neighbor. DOL SHA1 is
`25c5959534b3c21246c6c7e42021b916b41fb578`.

The literal native extraction is
`build/original-mario-nonstop-20260903/staged/OriginalMarioModuleAnimationFlags.cpp`,
also copied beside this note for durability. It contains both complete root
bodies and compiles with current native override/Aurora/fallback header order.
It has one external algorithm dependency: existing
MarioModule::changeAnimation(const char*, const char*). The attribute operations
are original inline frame-controller methods. Parent can copy the two bodies
into its full native MarioModule owner or select this extraction until that
owner is activated; do not select both providers.

The same original compiler run compiled complete root MarioWait successfully
after the parent corrected root/native `Mario::checkSpecialWaitAnimation` to
its existing void definition. The source/header hashes and commands are in
`evidence.json`. This is compilation evidence for MarioWait, not a retail
equivalence or live wait-state claim.

## Original sound initialization prerequisites

`Mario::initSound` has no dependency on a constructed animator, sound object,
movement states, or RuntimeContext. It performs table/hash initialization only:

1. initSoundTable(soundlist, 0) resets every entry's `_10` and copies the
   authored sound ID into `_14`; zero selects the base column and does not read
   swap columns.
2. It allocates a real HashSortTable, inserts all names with their real table
   indices and `isValidSkip=false`, then sorts it.
3. It initializes `_970` (current BAS name) to null.

The original compiler object contains 206 SoundList rows at 24 Wii bytes per
row (4944 bytes), including the empty sentinel: 205 actual sound entries. That
fits the existing original 1024-entry temporary sorting arrays. HashSortTable
allocates two 205-element u32 arrays and two 256-element u16 arrays. Its actual
native provider is HashSortTableCompat.cpp; add/search call the existing
MR::getHashCode, and sort calls the original selection-sort body in that same
provider. These references already resolved in the explicit initSound/playSoundJ
link proof of `original-mario-sound-20260903`.

The caller must provide the same retained Game allocation domain used for the
Mario object, so its table and primitive arrays live for the Mario lifetime.
The original constructor invokes initSound before setting `_97C` and constructing
the 37 state owners. Restoring this original initialization statement does not
itself require those states to be constructed first. It can therefore close
the real sound-table ownership prerequisite while the later constructor/state
group is being integrated. It is not a substitute for completing that group.

Current native constructor suppression of initSound leaves `_96C` null and
cannot be combined safely with the real playSoundJ body. Preserve original
initialization and lookup behavior; no null-return guard or fabricated table
was added by this task. No native Game file or shared build was modified here.
