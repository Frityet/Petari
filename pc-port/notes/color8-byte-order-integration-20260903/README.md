# Original numeric RGBA on native byte order

The native Color8 integer constructor and integer conversion now explicitly
unpack/pack the original `0xRRGGBBAA` number. Wii's big-endian union representation
previously reversed component bytes on little-endian hosts. Component fields,
GXColor conversion, size, copies and the original root header remain unchanged.
This is a two-expression architecture correction in the native Game header.

The normal `smg-pc-color8-byte-order-tests` target and standalone ASan/UBSan
compile pass 1,024 values with each component varied independently, including
numeric round trips, individual-channel construction, GXColor conversions,
copies, setters and default white. The earlier actual-disc effect cohort also
validated 170 authored packed color fields with this exact staged correction;
see `../original-effect-system-native-20260903/README.md`. That cohort remains
separate from this active primitive and is not a full particle-rendering claim.

The only direct Color8 `.mColor` writes found in the selected/native Game source
are MarioEffect's two all-white `0xffffffff` defaults, which are endian-invariant.
Other `.mColor` search hits are distinct light-info GXColor fields. No caller
workaround or new color interpretation was added.

```
cd pc-port
PATH=/opt/homebrew/opt/llvm/bin:$PATH xmake build -j8 smg-pc-color8-byte-order-tests
./build/macosx/arm64/debug/smg-pc-color8-byte-order-tests
```
