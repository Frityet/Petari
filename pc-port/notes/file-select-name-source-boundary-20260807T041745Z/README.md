# File-select names: exact Game source, real data, or absence

`FileSelectFunc.cpp/.hpp`, `FileSelectIconID.cpp`, and `MemoryUtil.hpp` are now
byte-identical to the regular decomp. The previous Game-side defensive name
clearing and host `wchar_t` conversion were removed.

The generalized compatibility boundary now supplies:

- the Wii `MEMAllocator` declaration in Aurora's `revolution/mem.h` surface;
- a checked byte-copy implementation for `MR::copyMemory`;
- direct access to the message service's actual UTF-16 BMG storage;
- a source wrapper that gives this exact retail translation unit the Wii's
  16-bit `wchar_t` width without changing Game code or globally changing the
  host C++ ABI.

No missing fellow name becomes English text or an empty manufactured name: a
missing BMG message is explicitly unavailable. No missing official RFL record
is copied or replaced.

## Exactness evidence

```text
fa0238247be06167f9e663766938364b61491a8da3562f4c222635172ccd564b  src/Game/Map/FileSelectFunc.cpp
fa0238247be06167f9e663766938364b61491a8da3562f4c222635172ccd564b  pc-port/src/Game/Map/FileSelectFunc.cpp
8d28b62718ee044c10bf5b82505cedcda243c44632e4decf8016840be86bf83a  include/Game/Map/FileSelectFunc.hpp
8d28b62718ee044c10bf5b82505cedcda243c44632e4decf8016840be86bf83a  pc-port/src/Game/Map/FileSelectFunc.hpp
1a57aa8c28ee24ae48597ffff093ef2f6902a64c71277f3e7b4d809714a66ba1  include/Game/Util/MemoryUtil.hpp
1a57aa8c28ee24ae48597ffff093ef2f6902a64c71277f3e7b4d809714a66ba1  pc-port/src/Game/Util/MemoryUtil.hpp
36bb4ca57bb192e7a08d7d513835924bc2a99102569bac0ac41e33a6537bd3a3  src/Game/Map/FileSelectIconID.cpp
36bb4ca57bb192e7a08d7d513835924bc2a99102569bac0ac41e33a6537bd3a3  pc-port/src/Game/Map/FileSelectIconID.cpp
```

All four corresponding `cmp -s` checks returned zero.

## Verification

```text
$ xmake build smg-pc-game
build ok
$ xmake build smg-pc-file-select-name-real-or-absent-tests
build ok, spent 5.299s
$ xmake run smg-pc-file-select-name-real-or-absent-tests
File-select name real-or-absent tests passed: 3/3
```
