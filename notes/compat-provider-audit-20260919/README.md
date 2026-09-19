# Compatibility provider audit

The source inventory includes src/compat and Aurora include/lib. Directory categories are not approvals of compatibility behavior. Actual configured Xmake source/object mappings and LLVM strong archive/object symbols supplement the existing Game source comparison.

## Final result

`xmake source-closeness-audit --output=... --check-providers` passes on the rebuilt program. The final report covers all configured project archive dependencies of smg-pc and its directly compiled objects; external package libraries are explicitly excluded.

- 894 compatibility source/header files inventoried.
- 23,351 strong symbol rows across the configured program source graph; zero duplicate strong providers.
- Zero ambiguous/missing provider mappings, zero directly stale source/object pairs, and no missing reviewed exported identities.
- 22 anchored original function signatures/bodies match donor tokens. Exported approval uses complete, explicit ABI identities; a new overload cannot inherit another overload's approval. Six mutation/incomplete-evidence tests pass.

There are still 8,668 unreviewed provider rows and 824 excluded original translation units. The manifest intentionally does not invent complete provenance for them. Existing Game file classifications are 1,163 byte-exact, 244 compile-only, 276 requiring review/migration, and 7 without an implementation counterpart. These counts are not API completeness or semantic equivalence claims.

## Earlier checkpoints and corrections

The first expanded archive scan covered Game/Common/Aurora: 898 inventoried files, 23,281 strong symbol rows, and 14 duplicate symbols. `before-final-owner-cleanup/` preserves this input; the ownership gate rejected it. `duplicate-removal.md` describes the retained original providers. `final/` is the earlier **archive-only** cleanup result (23,265 rows, zero duplicates), not the final full program scope.

Independent review then found that prefix attribution could incorrectly approve a newly added overload. The tool now requires exact ABI identity and verifies that each reviewed export actually exists in its declared provider. The scope was also expanded to app/render and binary objects. An intermediate scan (`complete-linked-surface/`) included an unrelated compile-only test archive and rejected its missing artifact; selection was corrected to the actual application's dependency graph. That scan also exposed five real duplicate C/runtime shim symbols: Aurora's shim source was compiled in both its base library and executable targets. The build now compiles it only in aurora-base; all 176 affected concrete targets retain that dependency. Final output is in `final-program/`.

The removal of unused direct shim inputs produced a byte-identical main executable and signed bundle after explicit relinking. This does not weaken the ownership check: the configured graph, not leftover object files on disk, determines candidate providers.

## Evidence limits

`provider-audit.json` records exact archive/object/executable hashes and scope. Archive membership plus executable symbol presence cannot identify which duplicate member a linker selected; duplicates are reported rather than accepted. Timestamp checks do not attest compiler flags or transitive headers. Source token comparisons do not prove semantics, static-data equivalence, or preprocessing outcomes. Weak/static symbols are outside the strong-provider gate.

`curated-artifacts.json` lists retained evidence and SHA256 digests. Large TSV/JSON evidence is deterministic gzip with the uncompressed digest retained. Full local outputs remain beside it. The mutation tests cover changed relocated bodies, overload ambiguity, literal/comment token boundaries, missing files, duplicate providers, unreviewed overloads, and reviewed bodies whose exported owner disappears.
