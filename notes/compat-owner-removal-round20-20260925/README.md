# Canonical SDK, text encoding and allocation owners

Nine compatibility files removed (37 to 28 remaining). All scene sidecars remain deleted.

- JKernel owns all original/native new/delete overloads and allocation routing; root/MEM2 provenance comes from authoritative heap/arena owners.
- Existing TextEncoding owns the compile-time CP932 literal encoder and frozen mapping. Runtime codecs and literal storage semantics are unchanged.
- Aurora's MSL runtime owns string formatting and its client stdio boundary. Provider numeric formatting still uses native stdio.
- The forced Metrowerks prefix is deleted. Canonical SDK headers own intrinsics/macros; real callers include their dependencies. Game's mixed-long clamp stays beside its original overload. Two scratch emitters are local static.
- Nine dead or fixture-only actor-registry APIs and an obsolete mirrored-state fixture target are removed.

The integrated build passed after eleven compile attempts: removing the forced header exposed missing type/SDK includes and the NWC24 outer C-linkage defect, which were corrected at their owners. Final incremental completion was 6.864 seconds; this is not the total rebuild time. Logs preserve the earlier diagnostics. No fixture suites ran.

One fresh-save Gateway opening run completed 120 frames and exited 0 in 3.126 seconds with no remaining process. Binary SHA-256: 8704e17a8b6a7325df64b9bbf0a822dec2879d6c120c44d2a507323e1bd4aa66. This validates bounded opening/shutdown only, not the full wakeup-to-Rosalina demo or audio playback.

Next: 24 audio compat files, then the two actor-registry files and two heap-domain files. Concrete owner plans are included. Unrelated staged route notes, editor settings and historical notes are preserved.
