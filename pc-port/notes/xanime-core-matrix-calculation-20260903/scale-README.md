# XanimeCore scale matrix recovery

Recovered three missing methods in root `src/Game/Animation/XanimeCore.cpp`, immediately before the preexisting `calcBlend` body. This change adds original behavior; it does not add a PC path or change playback frames. Root `XjointTransform::_68` is now `MtxPtr`: the retail routines pass that field directly to `PSMTXConcat`, and Maya also passes the parent's field to `PSMTXInverse`. This preserves the Wii layout and makes its pointer width correct when imported to a native ABI.

| Method | RMGK01 address | Size | Raw objdiff | Instructions after verified relocation |
| --- | --- | --- | --- | --- |
| `calcScaleBlendMaya` | `0x80019F74` | `0x548` | 99.881660% | All 338 equal |
| `calcScaleBlendSI` | `0x8001A688` | `0x4D0` | 99.870130% | All 308 equal |
| `calcScaleBlendSpecial` | `0x8001AC5C` | `0x98` | 100.000000% | All 38 equal |

The non-100 raw percentages are generated float constant labels, not arithmetic or register differences. `scale-verify.py` verifies all 684 instructions against the supplied DOL after applying 96 actual relocations. It resolves calls and global HA/LO references using `config/RMGK01/symbols.txt`. For float SDA loads it computes the effective retail address from the instruction and real SDA base, reads those DOL bytes, and compares them with the compiled constant. It does not accept constants merely because their values appear somewhere in the binary. This establishes exact original-compiler instruction correspondence for these methods, including branches and load/store ordering; it does not claim native arithmetic is bit-exact.

The supplied DOL SHA-1 is `25c5959534b3c21246c6c7e42021b916b41fb578`. Original compilation uses GC/3.0a3, sjiswrap and `configure.py`'s `cflags_game` for RMGK01. The existing anonymous union containing `J3DTransformInfo` produces the recorded compiler warning; compilation succeeds. No full game binary is committed.

## Recovered behavior

Both extended modes first postmultiply the prepared joint rotation matrix by the optional current `_64` and `_68` matrices, in that order. They compute local scale as `animationScale * (mScale * _14)` with the retail multiplication grouping. The three animation output rows are compensated for the parent's custom `mScale` when a parent index exists. Separately, when the joint's scale-compensate byte is exactly 1, the rows are compensated for `J3DSys::mParentS`. Each component equal to 1 skips `JMath::fastReciprocal`; other values use the existing retail `fres` helper, not ordinary division.

Maya additionally premultiplies the inverse of the parent's `_68` before writing the supplied translation plus `_2C`. It concatenates the local result into `mCurrentMtx`. Optional `_6C` then premultiplies its orientation after saving/zeroing translation; the saved translation is restored afterward, so `_6C` translation does not propagate. `_38` is added to the inherited current matrix. The current matrix is copied to the joint output, then `_20/_24/_28` are added only to the output translation. Finally `mParentS` receives the input animation scale, without the custom scale factors. `mCurrentS` is untouched.

Softimage does not invert the parent's `_68` and does not use `_6C` or `_38`. It writes translation multiplied by the previous `mCurrentS`, then adds `_2C`, and concatenates into `mCurrentMtx`. It updates `mCurrentS` by input animation scale, applies that accumulated scale to the output, restores output translation from the inherited matrix, and adds `_20/_24/_28` only to the output. Its final scale flag depends on the updated accumulated scale and overwrites the preliminary local-scale flag. It leaves `mParentS` untouched.

Special reads the cached **current** pose at `XjointInfo::_28`, builds its quaternion rotation with `PSMTXQuat`, and calls the Maya method. It does not resample animation data.

These three methods need the real current joint, matrix buffer, joint scale flags, and J3DSys matrix/scale globals. They do not access `j3dSys.getModel()`. The separate `fixT` method uses the model only in `_6 == 3`; default `_6` is zero.

## Behavioral checks and limits

`scale-behavior.py` executes the actual retail instructions in a deliberately small, rejecting interpreter. It has eight numeric cases with independent expected matrices/global state:

- Plain Maya scale with translated parent.
- Both distinct scale compensation sources and all three offset stages.
- Noncommuting `_64`, `_68`, inverse-parent and `_6C` rotations, with `_6C` translation discarded.
- Scale-compensate byte 2 disabled and unit scale skipping reciprocal.
- Softimage translation using prior accumulated scale and output using updated scale.
- Softimage final unit-scale flag overriding a nonunit preliminary flag, and ignored `_38`.
- Signed parent compensation with unchanged `mParentS`.
- Special selecting current rather than frozen pose, converting quaternion and using Maya.

The interpreter models external SDK helpers at their arithmetic interface. Fixtures use exact simple matrices and power-of-two reciprocal inputs; the latter explicitly expect the retail estimate (`fres(1) = 0.9998779296875`). This confirms these recovered routines' control flow, helper argument order and state effects. It does not independently validate general SDK inverse/concat/quaternion rounding, exceptional floating inputs, or native rendering. No native builds or runtime activation were performed for this root-only recovery.

Reproduce from the repository root with the supplied verified DOL and pinned original tools already present:

```sh
python3 pc-port/notes/xanime-core-matrix-calculation-20260903/scale-verify.py
```

The script reuses the adjacent entrypoint verifier's original-compiler/DTK readers, writes full object/diff artifacts under ignored `build/xanime-core-matrix-calculation-20260903/scale/`, and checks the numerical cases. Checked-in `scale-evidence.json` retains hashes, direct calls, actual relocation targets, full normalized event records and numeric results. The source-body hashes isolate these three methods from later additions elsewhere in the shared source file.
