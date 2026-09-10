# Independent TimeKeeper end-predicate review

The current reference/native `DemoTimeKeeper::isDemoEnd` matches the retail branches. No source correction is warranted.

* Paused: return false (`0x800BFEA8`–`0x800BFEB8`).
* Suspended part: compare signed total against current at `0x800BFED4`; `bgt` skips the true return. Thus `total <= current` ends the suspended segment.
* Ordinary end: compare signed total against current at `0x800BFEEC`; `blt` goes to false. Only `total >= current` continues to the equality test `numParts == partIndex`.

This is coherent with the original `update`: on the last part's boundary current becomes exactly total, the part index increments to numParts, and the final part pointer is retained. `isDemoEnd` returns true at that boundary. A caller that continues updating afterward can miss the ordinary equality boundary; the original DemoExecutor consumes it during the same update. This review does not rewrite the condition into a conventional catch-up predicate.

All 108 bytes of this function's saved retail assembly were independently compared to the actual main.dol; see `isDemoEnd-dol-proof.json`. Prior whole-TU proof already reported100% for this function. Only review/evidence files were written.
