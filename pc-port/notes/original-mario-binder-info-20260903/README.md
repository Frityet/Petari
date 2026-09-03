# Original Mario Binder contact handling

`Mario::updateBinderInfo` is restored root-first and mirrored exactly into the PC
source. This corrects the historical reconstruction; it does not activate the
original player update loop or claim that jumping is working yet.

The original GC3.0a3 compiler produces **99.63259% objdiff agreement**, up from
**65.60863%**, and the retail size of **2504 bytes**. A stricter comparison checks
**all 626 instructions**: after documented register allocation and relocation
normalization, every opcode, field offset, stack slot, flag mask, constant,
float operation, call, and branch destination agrees. No instruction or operand
is dropped from that comparison. The remaining byte differences are saved
integer register allocation and compiler data-label placement; this remains a
nonmatching decompilation rather than a claimed byte match.

The [audit](audit.md) lists the recovered differences and their implications.
The significant corrections are:

- The first ground push requires upward speed greater than 30, preserves its
  allowance when a contact needs no push, and uses the original distinct
  contact-direction and actor-offset vectors.
- Edge push scales, the `_14` correction flag, radian thresholds, and the
  stalled-falling vector corrections use their actual retail values and branch
  scopes. Multiplications and vector additions retain the original order.
- Hip-drop steering uses `_1A8`; the existing front-vector fallback is preserved.
- The original ceiling-contact host-name exception is recovered from the actual
  `lis`/`addi` address and sensor/host/name loads. It is an existing game rule.
- The routine ends after normalizing `_3A4`. The old reconstruction's extra
  Binder ground/wall/roof summary assignments are absent from retail and removed.
  The original floor pipeline remains responsible for its own ground state.

Reproduce the original compilation, raw-DOL reference checks, complete
instruction comparison, and native syntax check with:

```sh
python3 pc-port/notes/original-mario-binder-info-20260903/verify.py
```

The script requires the configured original compiler/toolchain and the supplied
RMGK01 DOL at `build/compat-math-oracle/main.dol`. Its SHA1 is checked before use:
`25c5959534b3c21246c6c7e42021b916b41fb578`. It verifies every instruction in the
retail split object against that DOL, checks direct call destinations, resolves
actual SDA2 loads, and decodes the actual HA/LO string address. Build products,
full objdiff, and the complete canonical instruction listing remain under
`build/original-mario-binder-info-20260903/`; only source and reproducible evidence
are retained in these notes.

The isolated native syntax check passes for the actual PC translation unit.
No native runtime test with a fabricated player owner was added: the complete
original instruction correspondence covers this bounded restoration, while the
real state/animation/model owners are still required before original jumping
can be enabled. The earlier compile-foundation report remains its historical
checkpoint; this report supersedes its 65.60863% Binder result.
