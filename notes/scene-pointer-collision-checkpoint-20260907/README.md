# Scene, pointer, collision compatibility checkpoint

This checkpoint restores the original StarPointerTarget object and LiveActor initializer, scene initialization phase ownership, retained KCL area traversal with original zone mutation ordering, and signed fixed-point conversion. Shared raw Wii word access now lives in Aurora. Game changes restore original source or supply missing architecture/declaration boundaries; the native StarPointer service consumes the actual target object.

Validation: original StarPointer ctor/calcPosition Wii100%; native pointer, scene initialization, scene object holder, factory, original effect ownership, Talk, authored placement, KCL area, collision registration, and math targets passed their bounded runs. Talk/authored LensFlare tests use the real Korean RVZ; the asset-independent authored subset also passes without a disc. Math adds signed fixed-point boundaries and round trips. The Aurora byte-order proof passes ASan/UBSan for65,536 values. Individual notes retain commands and limitations.

Fur reference recovery is underway in parallel. Its new Drawer/Shader sources remain excluded from the shared archive until complete owners and native activation are checked; this checkpoint does not claim a linked full showcase or Gateway gameplay. Once these current integration gaps are resolved, Mario movement and camera are the next priority. Existing walking-slice substitutions are inventoried under notes/movement-camera-priority-20260907 and will be replaced through original ownership/compatibility expansion.

User-staged DISCREPENCY_REPORT.md and MACOS.md deletions are excluded from this commit. New author/committer identity is codex.
