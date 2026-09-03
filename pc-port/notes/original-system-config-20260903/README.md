# Shared Wii system configuration data — 2026-09-03

The original CameraContext obtains screen shape through
`MR::isScreen16Per9` -> `MR::isAspectRatioFlag16Per9` -> `SCGetAspectRatio`.
The existing native ScreenUtil instead infers console settings from a published
camera pose. Closing the original owner requires the actual shared system
configuration source rather than adding a camera-specific setting or return.

The complete nineteen existing root accessors in `src/RVL_SDK/sc/scapi.c`
already match the original DOL byte-for-byte, including all relocated calls:
1,156 bytes verified by `verify-original.py`. These include video, sound,
language, screen saver, controller settings and Bluetooth arrays. The source
was not changed. Their typed find/replace operations and product-info backing
must exist before native selection. No SC accessor, CameraContext owner, or
Game screen behavior has been activated by this checkpoint.

Aurora now has an owned `SysConf` resource representation for the complete Wii
wire format: both arrays, byte, halfword, word, doubleword, and boolean. It
validates header/footer, file size, every contiguous offset, name and payload
range, and final offset. It preserves unknown names, original type distinctions,
duplicate order and payload bytes, keeping scalar data big-endian. Native
integers are read explicitly. Encoding produces the original 16 KiB format;
compact valid inputs are accepted. Original SDK ID indexing will live outside
the binary storage, avoiding native pointers or host-endian offsets in bytes.

The resource API checks replacements before publishing them, preserving the
previous document on failure. This is a storage API guarantee; original SDK
replace failure/dirty-state and asynchronous persistence semantics still need
their own bridge. Empty resources are valid wire documents, not a substituted
Game settings table. The nineteen original getters retain their own original
missing/invalid-value defaults when ultimately selected.

`aurora/tests/sysconf_test.cpp` uses an independent fixed 64-byte fixture with
all seven wire kinds and known big-endian values, then checks owned input
lifetime, exact type matching, 256/257-byte array boundaries, unknown keys,
duplicate-name order, capacity rollback and eleven malformed documents. Normal
and ASan/UBSan runs pass; `aurora-os` rebuilds with LLVM 23. Both CMake and xmake
select the new resource implementation. No GPU or full showcase was run.

Reference contracts were checked in the root `scsystem.c` parser/lookup methods
and the local Dolphin `Core/SysConf.cpp` format reader/writer. Aurora's new
implementation is independent and does not include Dolphin source.

Reproduction from repository root:

```
python3 pc-port/notes/original-system-config-20260903/verify-original.py
clang++ -std=c++20 -Wall -Wextra -Werror -Ipc-port/aurora/include \
  pc-port/aurora/tests/sysconf_test.cpp pc-port/aurora/lib/sysconf.cpp \
  -o build/original-system-config-20260903/sysconf-test
build/original-system-config-20260903/sysconf-test
```

The sanitizer run adds `-fsanitize=address,undefined -fno-omit-frame-pointer`.
The next coherent step is the runtime-owned SDK lookup/replace and NAND/product
configuration lease, followed by original screen and CameraContext activation.
