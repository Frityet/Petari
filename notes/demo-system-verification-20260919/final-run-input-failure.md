# Corrected-source replay with an invalid test input

This run is **not** bounded completion or a completed demo. It reached trace frame 26970, then exited 1 because the supervised input author joined A-button spans with a comma instead of the native semicolon. The last fatal diagnostic is `Debug WPAD button script contains an unknown or empty button`. The native parser correctly rejected it. This does not establish a game collision/physics crash. The process was reaped, and its binary hash stayed unchanged.

The first catch/dialogue completed at 10130/10500; a normal supervised A jump entered the pipe at 23380, followed by its catch/dialogue at 23770/24140. The long delays included unattended driver stalls. A screen capture after the pipe jump shows the second caught rabbit. The later hole approach visibly stopped against stone geometry (final-hole-approach-ui.png); the attempted jump over those stones was never accepted due to the malformed span.

The sampled trace has 2698 snapshots and 113148 actor samples, with no nonfinite checked vectors. Original Binder contacts include ground, wall and roof plus feature codes 1, 2, 3, 4 and 5. These observations do not prove every contact or retail parity. Rosalina did not appear; the tower gravity transition was not exercised.

The external driver was restarted at frame 14280 to use corrected full-speed chase input. Both operator logs and exact accepted native input receipts are preserved; the source hash snapshot and supervised events distinguish this run from a single-version deterministic replay. Only ordinary controller input was written. No game state was forced.
