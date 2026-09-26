# Persistent ringing: compressed sequence bytes interpreted as notes

The first real listening feedback identified a continuous high pitched tone. DSP capture isolated a square-wave voice at pitch 0x2ca3 (about 2792 Hz), enabled from the start of the opening sequence until shutdown. LLDB broke in original JASBank::noteOnOsc with pitch 89 (ASCII Y), velocity 122 (ASCII z), and program 0xf0. The track reader buffer began `59 61 7a 30 00 00 20 c0`: an unexpanded Yaz0 member, not sequence commands.

The port JKRArchive::readResource replacement copied compressed member bytes directly. Restored the donor JKRMemArchive buffer-fetch overload, its compression-flag dispatch to the original JKRDecomp worker, the original expanded-size lookup and pointer lookup. ID/path reads now dispatch through that implementation. Pointer resource lookups publish their cache identity consistently with indexed reads. Game sound code and authored sound data are unchanged by this fix.

Validation:

- Original archive tests cover Yaz0 and Yay0 members through ID and path reads, partial reads, zero capacity, output guards, encoded versus expanded sizes, and existing metadata/cache lifetime tests. All ten test groups pass.
- The real-disc 600-frame Good Egg probe accepts all three original UI SE requests, advances audio during a 100ms render pause, and exits status 0.
- Comparing one-second PCM windows: before the fix the 2785-2799 Hz band accounts for 81.06% of spectral energy at seconds 7 and 9; afterward it accounts for 0.000308% and 0.00343%. The sustained square-wave artifact has disappeared. This is captured DSP evidence, not a claim that every sound has been assessed by listening.
- Raw PCM and channel captures remain local in /tmp/petari-ring-{before,after}.{pcm,vpb}; game audio assets are not committed.

The longer opening/gameplay probe is still checking native shutdown. That is separate from the now-isolated compression defect.
