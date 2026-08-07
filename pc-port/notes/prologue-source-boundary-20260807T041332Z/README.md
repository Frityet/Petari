# Prologue source-boundary restoration

The Princess Peach letter and five-page prologue picturebook sources and
headers are now byte-identical to the regular decomp tree. The host's minimal
`J3DFrameCtrl` compatibility declaration was expanded with the retail-shaped
frame/rate accessors used by the exact picturebook source; the picturebook no
longer needs direct host-only member access or renamed fields.

## Exactness evidence

```text
364f8698b727b4aab8afcfb246acf531dc0fbbccc395f15402f2be8edeb2507e  src/Game/Screen/PrologueLetter.cpp
364f8698b727b4aab8afcfb246acf531dc0fbbccc395f15402f2be8edeb2507e  pc-port/src/Game/Screen/PrologueLetter.cpp
863d7137d822f15f9af8f4720a990224989079acbefb4bfcbf6c35927a4b6f52  include/Game/Screen/PrologueLetter.hpp
863d7137d822f15f9af8f4720a990224989079acbefb4bfcbf6c35927a4b6f52  pc-port/src/Game/Screen/PrologueLetter.hpp
6a8be74adc64e0c0118efddb60703380be26dfad0be67472b158ca4b3b1c34cc  src/Game/Screen/ProloguePictureBook.cpp
6a8be74adc64e0c0118efddb60703380be26dfad0be67472b158ca4b3b1c34cc  pc-port/src/Game/Screen/ProloguePictureBook.cpp
b75c89992e1327f53e112301e45e186e16391b3ccdcb37dd8b99c32c4e27cabf  include/Game/Screen/ProloguePictureBook.hpp
b75c89992e1327f53e112301e45e186e16391b3ccdcb37dd8b99c32c4e27cabf  pc-port/src/Game/Screen/ProloguePictureBook.hpp
```

All four `cmp -s` checks returned zero.

## Build evidence

```text
$ xmake build smg-pc-game
build ok, spent 1.546s
```

The exact retail `StorySequenceExecutor` path that selects this prologue was
separately verified by the story-sequence, Aurora-native, and stage-start
camera suites in `notes/story-sequence-real-or-absent-20260807T040912Z/`.
