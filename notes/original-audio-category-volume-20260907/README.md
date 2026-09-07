# Original JAudio category volume ownership

The native playback service now retains the original AudSystemVolumeController and sixteen actual JAISoundParamsMove records. Its original preset stack, all eight AudParams::scCtgVolume rows, trigger/recover order, 60-step level ramp and two-frame refresh timeout run unchanged. The Game header/source are copied unchanged; one native method boundary routes setSeVolumeSetInner onto the retained category records. No AudSystem or JAISeMgr instance is fabricated. Unbound controllers fail explicitly.

Fresh Wii compile/object comparison: all ten original controller methods are 100%; the existing original SDK moveVolume/movePitch/moveFxMix methods and existing transition set helper are all 100%. The complete existing reference SDK source in decomp/src/JSystem/JAudio2, including movePan/moveDolby, is imported unchanged into Aurora. No reference change remains; the three methods present in this retail object and transition helper have exact fresh comparison evidence. Retail category selection is JAUStdSoundInfo::getCategory at 0x804A2220, which reads JAISoundID group byte.

Aurora PCM voices now have a separate multiplicative bus gain. Category changes never overwrite a voice's own gain ramp or cancel stop/release. Existing playing one-shots, new voices and refreshed level voices all use current category gain; stage music remains independent. Native sound recipe/voice cache allocation boundaries use HostAllocationScope, keeping them outside Game scene arenas.

Validation:
- Seven production/test TUs compiled directly with LLVM23 (native-proof.json).
- Fresh exact-provider isolated link/run 0: all presets/categories, stack recovery, timeout, independent controller ownership, resource-free playback state, real PCM sample gains and stop-fade completion (isolated-result.json).
- Eleven affected production/test/extractor TUs in combined direct audit compiled0. Retail service test used freshly rebuilt archive/stream decoders and PCM/service providers against frozen native archives, without mutating root Xmake output.
- Extracted only nine files requested by the existing audio fixture from the user's RMGK01 RVZ (fixture-manifest.json). SMR.szs, decompressed BAA and B64kawa_0.aw matched the existing RMGK02-labelled SHA256 oracles exactly; no oracle was relaxed. Label is historical fixture naming, not a claim that the entire disc is RMGK02.
- Retail service probe LINK/RUN0: actual decoded title/prologue streams, finite SE, two level sounds, handles, pauses, retirement and new category routing assertions. SDL's explicitly enabled dummy test sink had 67 callbacks/68608 mixed frames/46556 nonzero samples. This proves concrete PCM rendering, not audible output or gameplay.

The full RuntimeContext regression is pending a coherent coordinated root rebuild, because the service/PCM descriptor sizes changed. Secondary BGM ownership is a separate next checkpoint. The isolated sample/state test does not claim full AudSystem mixing, multi-BGM synchronization, or GameScene death/wipe completion.
