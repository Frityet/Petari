# Animation block navigation and retained file bounds

Gateway's first run through the original ResourceHolder stopped on
`HeavensDoorMysteriousPlanet.arc/heavensdoormysteriousplanet.btk`.
LLDB stopped at the native decoder range check: retained/file size 576,
block offset 32, declared TTK1 block size 548. That next-block address is
580, four bytes beyond the retained file. This is actual supplied disc data.
The original Game constructor and SDK dispatch had reached the correct file.

The original key/full loaders in `src/JSystem/J3DGraphLoader/J3DAnmLoader.cpp`
read block-relative table offsets, then use `JUTDataBlockHeader::getNext()`
to advance by mSize. The pointer after the final block is never dereferenced.
There is no per-block memory ownership; all offsets refer into the same file.
The previous native decoder incorrectly used this navigation size as both a
required allocation extent and the bound for every referenced table.

Both generic animation and transform decoders now preserve the declared
navigation step for required following headers, while validating table reads
against the remaining retained file. No source bytes or metadata are changed.
The existing complete-file bounds and individual header/table/name/key/sample
checks remain active. Actual truncation still fails before SDK sampling.
This has no archive name, stage check, four-byte tolerance, invented padding,
or special animation-family branch.

The new block-navigation regression exercises all twelve supported families
with an undersized final navigation extent and a final next pointer far beyond
the file. It samples actual transform data after loading and separately rejects
a required out-of-file next block and a genuinely missing final visibility
sample. Existing fixtures cover original block order and last-block wins.

Runtime results are retained in the neighboring `*.bounds-*` logs; final shared
and sanitizer results belong in this migration's README and evidence files.
