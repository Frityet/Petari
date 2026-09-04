# Native original draw-category integration

Applied the corrected fourteen-file proposal, imported the complete original NameObjHolder header for its member-function typedefs, and selected the providers through the existing source globs. The first regular build caught this missing header, previously supplied by the isolated compiler overlay; production has no root-header fallback.

Added smg-pc-original-predraw-scheduler-tests using the verified CPU fixture. LLVM 23 ARM64 debug build and execution pass all callback order, empty category, heap routing, retirement, replacement, clear and mixed layout boundary assertions. The existing smg-pc-scene-scheduler-heap-tests also builds and passes.

Rebuilt and ran smg-pc-runtime-context-construction-tests with the real disc and actual Aurora Metal renderer. Actual constructor allocation failure, logger failure, both capture registrations, SC settings, original CameraContext, catalog ownership and teardown pass in two reconstruction cycles. Teardown reports the pre-existing aborted pending buffer mapping/device destruction warnings and exits zero. This is startup/ownership evidence, not a particle image or playable Mario claim.

The legacy EffectService draw call remains until the complete original EffectSystem owner and API can replace it atomically. The original draw-category callback path is now active for all categories; no actor or stage is singled out.

Focused logs: production-test.log, scheduler-heap-test.log, runtime-context-test.log. Original isolated ASan/UBSan proof remains in probe-runtime.log and native-verification.json.
