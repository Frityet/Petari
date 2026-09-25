# Actual linker closure

Evidence: integrated `build-retry2.log` reached linking and reported the exact missing symbols assigned to this lane. Root owns the resulting rebuild; no lane compilation or tests were run.

- Enable the complete existing `Game/RhythmLib/AudBgmTempoAdjuster.cpp`. Restore its exact donor header's field names; the stale header exposed only offset names. All tempo/stream synchronization arithmetic stays original. Its JAIStream/JASAramStream native fields already match the accesses. Preserve `-ffp-contract=off`.
- `AudChordInfo.cpp` retains the full donor in the non-PC branch. The PC branch preserves original invalidation/initParams field clearing and explicitly rejects chord resource loading. The donor CITS loader relocates a big-endian 32-bit table in place into pointer arrays; compiling it unchanged on a 64-bit host would corrupt the resource. There is no instantiated chord owner under disabled output.
- `AudRhythmMeSystem.cpp` retains its full donor in the non-PC branch. PC setSeq declines with false; parser selection and sequence retirement explicitly reject an unavailable initialized rhythm graph. No parser, rhythm holder or ME manager is fabricated.
- `AudMePlayer.cpp` retains its full donor in the non-PC branch. PC AudMeMgr::startMe declines before creating an AudMe or changing handle attachment. This supersedes the earlier wiring note to keep this file excluded: its PC branch now avoids the preexisting native inline/donor duplicate definitions.
- Canonical JAISeMgr.cpp/JAISeqMgr.cpp retain complete existing donor bodies in non-PC branches; their PC startSound methods decline with false and leave caller handles untouched. The real JAS voice/sequence pipeline remains absent.
- Canonical JASTrack.cpp preserves the exact original writePort algorithm. Complete JASTrackPort.cpp is imported unchanged apart from formatting. JASSeqCtrl.cpp supplies the original interrupt-mask operation in PC and the full donor otherwise. Muting rejects the unavailable JAS channel engine: the original mute performs noteOffAll/channel release, so merely changing mIsMute would be an incorrect success surrogate.

No JASDsp::setFXLine error appeared in the actual link list; this lane did not add it. Source snapshots, full manifest and scoped patch include all ten additional inspected paths. No catch-all provider or compatibility service was added.
