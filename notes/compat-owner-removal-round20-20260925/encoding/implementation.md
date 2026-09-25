# Compile-time CP932 belongs to TextEncoding

Integrated the entire literal encoder directly into existing resource/TextEncoding.hpp alongside its runtime conversion declarations. The implementation now belongs to smgpc::resource::cp932; no compat namespace/path alias is retained. The CP932 macro remains the same public expression API. Frozen data is colocated under resource/detail.

The literal implementation body is unchanged except namespace; the table is unchanged except namespace, and the TSV bytes retain SHA-256 46778ae55afa614d3ece7e4f7160d9f1205626f09a461e6d7a933b57b582d600. Therefore the exact array-lvalue/static-storage representation, strict invalid-input diagnostics, embedded NULs and frozen Windows CP932 canonicalization remain intact. Runtime TextEncoding.cpp is untouched.

Consumer edits are mechanical include/path updates only: 719 Game files, 10 test/tool consumers, one Lua source-normalization helper and README. Normalization continues allowing only the exact leading encoding include and literal-only CP932 expression; its rejection policy is unchanged. There is no replacement wrapper header, build-time source generation or xmake wiring change.

Every owned path is listed in owned-manifest.json with before existence/status/hash. Before copies and the eventual exact patch preserve the change boundary. No build, test, staging or commit is performed by this lane; root handles integrated validation.
