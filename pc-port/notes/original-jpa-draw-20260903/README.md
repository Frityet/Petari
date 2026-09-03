# Original JPA draw and full-manager CPU closure — 2026-09-03

The original JPA manager now links and runs in an isolated native CPU fixture with the staged host-order JPC loader. This checkpoint does not activate production effect ownership or rendering. The earlier `original-jpa-resource-loader-20260903` package is unchanged.

## Root checkpoint

Only these three root files belong to this checkpoint:

- `src/Game/System/Overwrite.cpp`: complete original draw/helper closure, seven missing draw callbacks, `JPAEmitterManager::calcYBBCam`, and corrected `JPADrawLine`.
- `src/JSystem/JParticle/JPABaseShape.cpp`: remove the misplaced, incorrect `JPADrawLine` definition. Its old uninitialized-local write into particle position is not retail behavior. Retail first copies particle position, rejects near-zero velocity at 0.001, scales/subtracts velocity, and emits two line vertices without modifying position.
- `libs/RVL_SDK/include/revolution/mtx.h`: declare the existing original `PSMTXMultVecArraySR` provider. This provider already exists in root `mtxvec.c` and Aurora. No implementation replacement is needed.

`root.patch` is against the field checkpoint (185bea40b) and includes all three files. `provider-inventory.json` records literal definitions before/after. The actual original-compiler SDK object has no `JPADrawLine` definition; `Overwrite` owns its sole definition. Other SDK callbacks remain untouched.

The original compiler builds the full changed root Overwrite and SDK BaseShape files. `root-evidence.json`, `sdk-provider-evidence.json`, and `dol-evidence.json` contain commands/results. The retail object used for fuzzy comparison is independently matched to local DOL SHA1 `25c5959534b3c21246c6c7e42021b916b41fb578`, masking only relocations. Both display-list byte arrays are byte-exact DOL data, and all five dispatch tables compare 100%.

| Restored method | Retail address | Original-compiler match |
| --- | --- | --- |
| JPADrawDirection | 803A514C | 99.90654% |
| JPADrawRotDirection | 803A52F8 | 99.78102% |
| JPADrawDBillboard | 803A551C | 99.79570% |
| JPADrawLine | 803A5690 | 99.578316% |
| JPADrawStripe | 803A57DC | 98.18725% |
| JPADrawStripeX | 803A5BC8 | 93.61659% |
| JPAEmitterManager::calcYBBCam | 803A65D4 | 99.649124% |
| JPADrawYBillboard | 803A66B8 | 99.74324% |
| JPADrawRotYBillboard | 803A67E0 | 99.82758% |

The source preserves original traversal direction, both cross-stripe passes, particle-axis updates, fallback directions, 0.001/0.01 predicates, authored pivots, texture coordinates, GX calls, projection selection, rotation/plane callbacks and display-list identities. The substantial StripeX difference is register allocation/local lifetime; the instruction/control-flow and arithmetic sequence was recovered from the attached DOL assembly. No compiler pragmas or header changes force matching. Helpers compare 95.78261–100%; the original `noLoadPrj` really is a four-byte `blr`.

## Native staging

`build/original-jpa-draw-20260903/staged/` contains the complete isolated fixture source closure; `native-manifest.json` maps files to eventual native destinations. The staged JPA resource/loader manager code is byte-identical to the earlier frozen package except BaseShape's removed incorrect line renderer. The current production TVec header is used, not the previous package's staged geometry header.

For review/application after the prior loader package:

1. Apply `native-delta-after-loader.patch`: remove the SDK line renderer, add literal `compat/OriginalJPADraw.cpp`, and add literal `compat/OriginalJPAFields.cpp` from committed root field methods.
2. Ensure the earlier `compat/OriginalJPAEmitterInit.cpp` extraction is included once; this checkpoint's staged copy is unchanged from `original-effect-construction-20260903/native/OriginalJPAEmitterInit.cpp`.
3. Build the original JPA SDK sources with these three extraction TUs. Do not additionally select native full `Overwrite.cpp`, which would duplicate the same bodies.
4. Keep the resource registration/owner prerequisite from the original loader notes: register immutable `particles.jpc` bytes plus their actual archive owner before constructing the actual original resource manager. No emitter/object substitute is introduced here.

Parent owns native application and build selection. No production files, xmake, shared build outputs, GPU state, or assets are written by these scripts. Archive extraction remains in ignored build only.

## CPU evidence and remaining arithmetic boundary

Normal full fixture result:

- All 3,327 actual authored resources and 225 textures load through original managers/blocks.
- All 45,386 original function-list entries exist.
- Emitters are constructed by every authored ID, retaining exact resource identity and authored rate.
- 52,571 actual original CPU calculation frames, 352,896 particle-frame observations.
- Actual emitter and particle pools are reused; authored pool exhaustion returns null through the real original manager algorithm.
- Recovered Y-billboard camera calculation passes numeric checks.
- Domain retirement releases all native resource backing; explicit weak ownership expires.

ASan/UBSan full simulation completes, with no AddressSanitizer findings but four UndefinedBehaviorSanitizer diagnostics in previously present SDK arithmetic. **This is not a clean sanitized simulation result.** Exact diagnostics are retained in `full-probe-asan-runtime.log`:

- `JPAParticle.cpp:100`: negative float to unsigned 16-bit conversion.
- `JPAParticle.cpp:101`: float 32768 to signed 16-bit conversion.
- `JPADynamicsBlock.cpp:57`: integer division by zero.
- `JPADynamicsBlock.cpp:58`: float 32768 to signed 16-bit conversion.

`--sanitize --ownership-only` explicitly excludes all simulation calculation (zero frames/particle observations) but constructs all resource IDs, checks all function lists, checks the recovered camera helper and pool exhaustion/reuse, and tears down heaps. This narrower run is clean under both sanitizers. The next independent checkpoint must establish exact PPC conversion/division behavior and implement a general native SDK arithmetic boundary before claiming clean simulation or activating runtime drawing.

Reproduce without a shared build:

```
python3 pc-port/notes/original-jpa-draw-20260903/verify-root.py
python3 pc-port/notes/original-jpa-draw-20260903/verify-native.py
python3 pc-port/notes/original-jpa-draw-20260903/verify-native.py --sanitize
python3 pc-port/notes/original-jpa-draw-20260903/verify-native.py --sanitize --ownership-only
```

The native fixture links existing read-only Aurora/project libraries; its newly compiled object files/executable live only under the isolated build directory. Draw callbacks are linked but never executed, and no GPU or presentation result is claimed.
