# Native GX array extents and immutable upload reuse — 2026-09-10

The first Gateway draw exhausted Aurora's fixed storage staging area because unsized J3D bindings repeatedly uploaded growing prefixes and A→B→A bindings repeated the same bytes. The generalized fix carries authoritative native array bounds through unchanged GX/GD consumers and reuses existing immutable frame snapshots when bytes agree. No Game source, shader addressing, staging capacity, or rendering high-water behavior changed.

## API and decoding

`aurora/gx_array.hpp` provides movable `aurora::gx::ArrayRegistration(data, size, littleEndian)` ownership. A registration describes exact readable array bytes; interior pointers resolve only their remaining extent. Null/empty/overflow ranges and conflicting overlaps are rejected. Identical registrations share lifetime. Destruction/reset unregisters the last owner, and the registry state survives static teardown while an owner remains.

The FIFO decoder resolves registrations only for unsized `GX_AURORA_LOAD_ARRAYBASE` commands. Lookup occurs after reading the final pointer at replay time, so a `GDPatchArrayPtr` replacement gets the replacement's actual extent/endian metadata. Explicit-sized commands retain their stated cap and byte order. Unknown arrays still use bounds proven from referenced indices.

The root's separate J3D loader integration registers each fully materialized native array using its actual offset/size and unregisters before its bytes are destroyed. It does not register an entire allocation or infer an array length from a neighboring native pointer. The complete root test and runtime evidence belong to the parent.

The initialization agent's accompanying frame cache stores an immutable CPU byte snapshot per source pointer. Rebinding reuses the existing GPU range only when the requested prefix fits and its bytes still agree. Changed bytes, a wider unknown extent, or a new frame trigger a fresh upload. FIFO shutdown clears retained snapshots after stopping the worker.

## Why reservation was not used

Current `capture_frame_op` records a monotonic storage high-water mark. `copy_staging_to_high_water` copies each prefix only once, while the worker can consume sealed passes concurrently. Reserving holes and filling them later below an already captured/copied mark would require explicit dirty-range replay and staging synchronization. Tail-only extension is safe under narrower conditions but does not solve interleaved position/normal/texture array growth. Authoritative extents avoid both issues while retaining the existing immutable staging contract.

## Evidence and publication

- Independent CMake build: exit 0. Combined GX/GD/array tests: **269/269**, zero skips. Logs: `../gx-array-snapshot-cache-20260910/combined-{build,tests}.log`.
- Eight new registry tests cover exact/interior/exclusive bounds, endian metadata, moves/shared lifetime, rejected overlapping/overflow extents, original GX and base-only consumers, late-registered GD pointer replacements, explicit-sized cap preservation, and unknown fallback.
- Sixteen increasing position indices require **one upload** with a registered extent versus **sixteen uploads** for an unknown source. This is a CPU decoder/staging-call proof; real GPU presentation remains parent-owned.
- The initialization agent's mutation/rebinding/frame tests run in the same combined suite.
- Aurora commit `220b873e6478de033682ed79bd2b4a2a4ef2b1ed` includes exactly the eleven frozen combined paths, authored/committed as codex, pushed and verified against `origin/codex/macos-compat`. `source-manifest.json` records every path/hash; `result.json` is the compact result.
