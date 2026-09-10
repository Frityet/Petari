# Direct original LodCtrl and raw Game identity

The first Tico LOD model reached `ModelObj::ModelObj` with a raw CP932 actor name followed by UTF-8 fullwidth parentheses. The concrete mixed encoding originated in `src/compat/LodCtrlRuntimeCompat.cpp::createSubModelObjName`, rather than the LOD actor itself. The exception stack is retained in `../cp932-startup.log`.

Removed `src/compat/LodCtrlSource.cpp`, which included the entire Game implementation through a `ModelObj` substitution macro and a forwarding-only subclass. The complete native `src/Game/LiveActor/LodCtrl.cpp` is already included by the existing Game source glob; its genuine ModelObj constructor is public and the needed utility declarations already exist. No Game source/header or build-list change was required. Direct Game compilation now has the standard original literal provenance.

The native name helper retains raw Game actor/submodel bytes and formats the original fullwidth parentheses with explicit CP932 bytes `81 69` / `81 6a`. Its output remains allocated with the original current Game allocation domain; no new owner or text heuristic was added. The original helper in `decomp/src/Game/Util/LiveActorUtil.cpp` uses the same length-plus-parentheses formatting behavior.

The existing native LodCtrl::update body remains unchanged. Its dead/inactive gate, no-submodel branch, hidden/forced-model precedence, distance comparisons, and final transform copy were compared with retail `0x80166F08–0x801670A8`. The reference TU still lacks this body; reference recovery remains a separately recorded gap rather than a new native alteration in this checkpoint.

Production was frozen for the parent build immediately after these two edits. No new tests, isolated compilation, or runtime replay was performed by this subtask; the parent owns the single integrated build and smoke.
