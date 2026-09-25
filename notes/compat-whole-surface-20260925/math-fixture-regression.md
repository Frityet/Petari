# Math owner fixture and RNG regression

Both test files were clean before this task. Updated only `tests/GameMathRotationTests.cpp` and `tests/GravityMathFoundationTests.cpp`; removed their unnecessary MetrowerksStdCompat includes.

GameMathRotation now initializes the original arccos table at suite entry, before its early quaternion/spherical cases. Previously `initAcosTable` was called only by a later table-specific case; the old lazy compat table hid this ordering issue.

A local fixture creates the real GameSystem singleton and publishes actual trivially-copyable GameSystemObjHolder and GameSystemSceneController values for their random/current-stage data. `std::bit_cast` from zeroed representation avoids booting unrelated DVD/NAND/archive constructor services; static assertions enforce the trivial-copy requirement. This is confined to the test, requires no production test hooks, and introduces no alternate RNG owner or fallback. The fixture unpublishes borrowed fields and releases the singleton before its values retire. Existing antiparallel quaternion perturbation cases now have the original random owner.

Added one ownership/reseed regression to that existing target:

1. Seed the holder, draw via MR::getRandom, and verify the original JMath generator output and actual seed advancement.
2. Interleave a direct holder draw and verify the next MR draw continues the same stream.
3. Call setRandomSeedFromStageName using the actual scene controller stage and verify hash/sequence (`HeavensDoorGalaxy`: `0x667cac16`).
4. Consume four samples, reseed the same stage, and require exact sequence replay.
5. Change the actual stage to EggStarGalaxy (`0x162a21c7`), reseed and verify a different observed sequence from the shared holder.

GravityMathFoundation now explicitly initializes the math table at suite entry and its stale assertion text no longer says MathUtil is excluded. It invokes no random path and needs no GameSystem fixture.

Static declaration-name scan found no restored MathUtil definition missing a declaration in MathUtil.hpp. Diff whitespace check passed. No build or test was run here while root compilation was active. Existing target names for root: `smg-pc-game-math-rotation-tests` and `smg-pc-gravity-math-foundation-tests`.

The root compile exposed the inline `MR::abs` overloads' dependency on forced Metrowerks intrinsic declarations. The canonical header now includes its native `<cmath>`/`<bit>` dependencies directly: float abs uses `std::fabs`, and signed integer abs computes the magnitude in unsigned arithmetic before bit-casting back. The original Metrowerks intrinsic branches remain. This preserves negative-zero and NaN-sign behavior and the minimum signed integer's wrapped result without signed-overflow undefined behavior. Added regression cases to the existing GameMathRotation target for negative zero, a negative NaN payload, minimum integer, and ordinary integer inputs. No concurrent build was run.
