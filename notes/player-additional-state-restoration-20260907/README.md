# Additional retained player state sources — 2026-09-07

Restored MarioRabbit, MarioSpin, MarioSkate and MarioBlown source files lost
from the decomp reference during flatten, from preflatten commit
e1985ac3a9736a9c432c1d7af6ce8bf96e3abed4. All four source files are byte-identical
to their existing native counterparts. Restored the matching Rabbit and Skate
headers. No new player behavior was invented.

Mario::doSkate and Mario::blown return bool in the retained bodies; their
previous void declarations were compile errors. Both declarations are now bool
here and in the port.

The exact original Metrowerks flags, source hashes and retail comparison scores
are recorded in next-player-wii-proof.json. Complete .text fuzzy matches:
MarioRabbit96.607254%, MarioSpin99.65156%, MarioSkate99.19598%, and
MarioBlown99.65417%. These are Wii source comparisons, not native gameplay tests.
Native source activation and link closure remain in the parent port notes.
