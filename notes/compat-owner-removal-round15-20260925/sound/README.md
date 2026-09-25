# Restore complete SoundUtil

Enabled the complete latest decomp SoundUtil.cpp in its actual Game location and deleted the parallel OriginalSoundUtil.cpp. Preserved only the eight existing disabled-output dispatches in the corresponding Game utility functions, plus explicit native overload arguments. Audio output remains optional; this does not claim full AudSystem restoration. The original utility bodies now follow the current donor rather than an older separately compiled copy.

No new decompilation or fixtures. Root owns Game/xmake.lua wiring and the integrated build/opening smoke.
