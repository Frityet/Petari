# Original special-mode animation update and chest data

This root-only recovery closes two gaps identified by the complete MarioAnimator staging audit. The actual method is **MarioActor::updateSpecialModeAnimation**, called by MarioAnimator; it is not a method on MarioAnimator itself. No native animation branch is activated and no PC Game, shared build configuration, or runtime/test code changes.

Changed root files:

- `src/Game/Player/MarioActorParts.cpp`: restores the complete missing method and includes MarioAnimator.hpp so its real player/member interface is available.
- `src/Game/Player/MarioAnimator.cpp`: replaces the declaration-only `jname_chest` with the authentic definition `const char* jname_chest = "Spine1"`.

## Current retail and original compiler proof

`verify-original.py` compiles both complete actual root TUs with GC 3.0a3 and configure.py's Game flags for RMGK01 / VERSION=0, using real root headers. Existing nontrivial-union warnings are retained in the logs; there are no compilation errors or generated replacement headers.

`updateSpecialModeAnimation__10MarioActorFv` is retail `0x802BCD08`, size `0x1E0` / 120 instructions. Objdiff reports **99.6%**. The only eight differing operands are the offsets used to address the six authored strings through the shared data-pool base. The current root omits unrelated earlier strings from this TU, so these strings begin 0x1B bytes earlier in its compiled pool.

The verifier checks every compiled and retail string byte (including original Shift-JIS encoding and terminator), resolves 16 ELF references to real functions/data, and normalizes only those eight proven string-pool offsets. Every one of the 120 instruction words then equals the verified retail DOL. This includes all conditional and unconditional branches, loads, stores, counter arithmetic, register use, argument order, and helper calls. It does not label the unnormalized object a byte-for-byte match or add artificial string padding for percentage matching.

The DOL SHA1 is `25c5959534b3c21246c6c7e42021b916b41fb578`. `source-evidence.json` retains commands, source hashes, retail range hash, references, each string byte/offset comparison, and original-compiler field-layout checks for actor/player members used by this method. Build artifacts and diagnostics are under `build/original-mario-special-animation-20260903/`.

## Recovered behavior

The method preserves the original ordering of three independent pieces of animation state:

1. When player movement flag `_A` is clear and `Mario::getCurrentStatus()` returns zero, the grounded/default-animation path checks player `_960 == 0x20`. It replaces tracks 0/1 with authored `泥低速歩行` and `泥高速歩行` and sets actor `_B96=2`. If flag `_A` is set or status is nonzero, it instead clears `_B96`. Failing the nested grounded/code/animation condition leaves the prior counter intact.
2. Mode 6 delegates to the real `updateTeresaAnimation`. Mode 4, when Bee wall walking and not jumping and the default animation is active, selects `ハチ匍匐前進` on tracks 0/1/2 and `ハチ匍匐ウエイト` on track3. Other modes clear player `_418`; the Bee and Teresa branches do not perform that default clear.
3. A nonzero `_B96` is decremented in this same call. On reaching zero, if still grounded and on the default animation, tracks0/1 restore `鈍行` and `歩行`. Thus the initial value2 is immediately reduced to1. This behavior also runs for ordinary Mario and must not be replaced by a no-op based on the function's special-mode name.

The movement-state tests are the original bitfields (`_A` and `_1`, Wii masks `0x00200000` and `0x40000000`), and `_960`, `_418`, `_B96` retain their existing typed fields. No invented state or new field/layout is introduced. All animation changes use the existing original `XanimePlayer::changeTrackAnimation(u8,const char*)` interface, preserving authored lookup, resource swapping, and track weights.

## Authentic chest-joint data

The configured retail split and actual extracted `Game/Player/MarioAnimator.o` own `jname_chest` in `.sdata`, address `0x806B2288`, size4. Its complete retail word is `0x805C3FDA`; the pointed bytes are `Spine1\0`. The newly compiled original object emits the same named writable pointer-to-const-char with one `R_PPC_ADDR32` relocation to the exact string. Resolving that relocation to the verified retail string reproduces the full original pointer word.

This is a restored source data definition, not a pointer-width cast or a renamed joint. The earlier animator staging snapshot still lists the symbol as unresolved; that snapshot predates this root restoration. Native import and allocation/model ownership remain separate work. The lower/upper players must still be actual constructed objects with their retained model/resources before this method can run.

Reproduce:

```sh
python3 pc-port/notes/original-mario-special-animation-20260903/verify-original.py
```

No native/shared build, GPU test, or runtime validation is claimed for this root-only recovery.
