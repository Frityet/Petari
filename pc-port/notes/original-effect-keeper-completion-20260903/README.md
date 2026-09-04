# EffectKeeper completion and MSL binder storage

Recovered the four missing original EffectKeeper methods in root source:
placement-time attribute-floor selection, onDraw, offDraw and updateFloorCode.
The floor query retains all original guards, matrix Y-axis negation and
normalization, the 100-unit start offset, the original collision line query,
and the nonnegative floor-code condition. Bound actors retain the original
ground-triangle query even when their current contact is wall/roof. No movement
rule or stage-specific contact fallback is introduced.

`verify.py` compiles the actual Game source with its configured Metrowerks
compiler and checks the SHA1-verified RMGK01 DOL. Three methods reproduce all
380 instruction bytes after independently verified relocations; the two
draw callbacks also verify complete 12-byte member-function pointers. The
392-byte placement method is 97.60% objdiff: its only seven differing words
are paired-single temporary-register allocation and one independent
store/multiply reorder. The verifier checks those exact words, and every
remaining instruction, branch, called method and data reference agrees.
This is source/binary verification, not live floor-collision gameplay proof.

The actual draw methods instantiate binder2nd with a stored s32 value. Root
MSL and the native Aurora adapter previously always retained a reference to
the supplied argument, which can dangle for returned callbacks or temporaries.
Both now store the member's declared parameter type: value parameters are
copied/converted, while reference parameters retain the original referent.
This also preserves ILP32 s32 conversion when the native host passes a long
literal. `verify-baseline.py` checks the previous EffectKeeper methods and
existing reference-taking clients (ElectricBall, GalaxyMap and AreaObj) against
the frozen old MSL header.

The regular `smg-pc-msl-functional-tests` executable and a separately compiled
ASan/UBSan version pass scalar-copy, returned-local/temporary lifetime,
conversion, noncopyable reference identity, const and noexcept member cases.
Exact sanitizer command, run from pc-port:

```
/opt/homebrew/opt/llvm/bin/clang++ -std=c++23 -fsanitize=address,undefined -fno-omit-frame-pointer -Iaurora/include tests/MslFunctionalTests.cpp -o ../build/original-effect-keeper-completion-20260903/functional-asan
../build/original-effect-keeper-completion-20260903/functional-asan
```

EffectKeeper itself remains root-first until the actual MultiEmitter,
EffectSystem and scheduler owners are ready for native activation. These
changes do not activate jumping or claim full original camera gameplay.
