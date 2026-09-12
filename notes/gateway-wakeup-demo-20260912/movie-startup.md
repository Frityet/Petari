# Original movie owner activation

Imported MoviePlayerSimple, MoviePlayingSequence, THPSimplePlayerWrapper, MovieSubtitles/DataTable, DemoPadRumbler and the original THP GX YUV drawing helper. The game target now links Aurora's existing THP decoder. Native changes read packed THP file/frame fields through Aurora's general endian helpers and size pointer-bearing memory clears using native sizeof. Stereo sample clearing uses sample count times two s16 channels rather than sizeof a pointer.

Movie audio follows the existing explicit disabled audio policy: no JAS output callback is registered and the wrapper decodes video without waiting for audio-buffer consumption. Video frame/component order and original Nerve sequencing remain intact. Native THP decoder declarations now match Aurora's const input contract.

Movie and subtitle sources compiled and their symbols resolved in the shared fifth startup link. This establishes source closure only; no movie playback or wakeup sequence has run yet.
