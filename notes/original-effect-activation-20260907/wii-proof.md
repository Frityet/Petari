# Current original-owner source verification

Per-function objdiff percentages use current PPC output and retail objects.

| Source | Retail functions | Supplied | 100% | >=90% | Minimum % |
| --- | ---: | ---: | ---: | ---: | ---: |
| EffectSystem | 9 | 9 | 9 | 9 | 100.0 |
| ParticleEmitter | 5 | 5 | 5 | 5 | 100.0 |
| ParticleEmitterHolder | 9 | 9 | 9 | 9 | 100.0 |
| ParticleDrawExecutor | 20 | 20 | 17 | 20 | 99.82143 |
| ParticleCalcExecutor | 10 | 10 | 9 | 10 | 99.88095 |
| SingleEmitter | 8 | 8 | 8 | 8 | 100.0 |
| MultiEmitter | 44 | 43 | 41 | 42 | 52.68421 |
| MultiEmitterAccess | 22 | 22 | 20 | 22 | 97.37705 |
| MultiEmitterCallBack | 34 | 34 | 31 | 33 | 12.272727 |
| MultiEmitterParticleCallBack | 4 | 4 | 4 | 4 | 100.0 |
| SyncBckEffectChecker | 8 | 8 | 6 | 8 | 97.77778 |
| SyncBckEffectInfo | 7 | 7 | 7 | 7 | 100.0 |
| EffectKeeper | 38 | 38 | 31 | 38 | 97.60204 |
| NameObjAdaptor | 9 | 9 | 9 | 9 | 100.0 |

The unmatched TVec3<short> constructor in MultiEmitter is inlined into setGlobalRotationDegree. Retail calls an out-of-line constructor. Both perform float-to-integer truncation followed by the same three short stores; this accounts for the 52.68% instruction score without a missing rotation operation.

MultiEmitterCallBack::effectLight is independently audited in notes/original-effect-light-20260907 (agent report). Retail performs the same near-zero test as the candidate. Retail uses a prologue/epilogue while the candidate tail-calls the predicate. Its low score is not a missing lighting algorithm.

The audit found and repaired two actual omitted operations: createEmitterWithCallBack and forceDelete(EffectSystem*) now apply the same callback/delete operation to each child MultiEmitter. Both methods now score 100%.

The decomp legacy bind2nd factory now stores the converted second_argument_type by value, as retail and native Aurora do. The previous reference field changed callback ABI and could borrow temporary argument storage. setDrawOrder now scores 100%.

setGlobalSRTMatrix declarations now use const Mtx (const float elements), rather than const MtxPtr (const pointer with mutable elements); both original exported symbols now match retail.
