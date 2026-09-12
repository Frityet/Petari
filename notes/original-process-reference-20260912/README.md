# Original process and utility reference recoveries — 2026-09-12

Reference-first recovery now supplies the retail Wii overrides for JKRAram construction and DMA dispatch, four previously missing CSV readers, and three missing sound utility functions. FileLoader's invalid integer-null expression uses zero; FileRipper's stack alignment uses the existing SDK ROUND_UP_PTR macro. These retain original source conventions and are mirrored to native owners as their underlying SDK services become available.

Fresh original-compiler builds of all five touched reference translation units pass. The two recovered ARAM overrides match at 100%; CSV readers are 99.66–100%; recovered sound methods are 99.875–100%. The full FileRipper/FileLoader text matches are 99.978%/99.319%. Selected function and section results are recorded separately so partial section coverage is not confused with an exact whole-file match.

Retail ARAM dispatch includes direction-independent numeric guards and acknowledgements on those early exits. They are preserved in the reference; the native SDK memory-copy boundary validates real transfer spans independently. Sound dispatch uses the real sound ID table and selects the actor or system owner, while the fade query inspects its actual transition and stopping state.

This checkpoint publishes reference recovery and compiler evidence. Native ARAM workers, CSV ownership and sound ownership have separate bounded runtime receipts; complete original GameSystem startup and Gateway bunny/Rosalina progression remain unfinished.
