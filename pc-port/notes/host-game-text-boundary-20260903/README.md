# Host and original Game text boundary

## Activated change

`resource::encode_cp932` explicitly converts native UTF-8 text into CP932 for
Game APIs. Invalid UTF-8 and unrepresentable characters throw. Windows uses
strict UTF-8 decoding, disables best-fit CP932 substitutions, and checks the
default-character flag. POSIX requests CP932 specifically and rejects iconv's
nonreversible conversion count as well as errors. It does not substitute the
different Shift-JIS codec. Converter ownership covers allocation/error paths.
Embedded NULs are preserved through the length-based API.

The existing decode helper remains available for UTF-8 presentation. Its header
now states that original resource bytes remain the Game identity. Encoding
host text does not imply that decoding and re-encoding an original resource
will preserve its bytes: CP932 has duplicate encodings. Retain original bytes
for identity and use the new encoder only at explicit host-to-Game boundaries.
There is no encoding auto-detection and no change to Game files or JMap data.

This checkpoint adds the conversion primitive and its contract. It does not
activate the separately staged original-Game execution-charset compiler view
or change callers while their current literal encoding is still UTF-8.

## Validation

The regular `smg-pc-text-encoding-tests` target and a standalone compile of both
the test and codec with AddressSanitizer/UndefinedBehaviorSanitizer passed on
macOS ARM64 with Homebrew LLVM 23. Explicit expected CP932 byte strings cover
original common/material effect prefixes, a wipe name, half-width kana,
CP932's circled-number extension, and Japanese names containing backslashes.
The test also covers every ASCII byte, embedded NULs, 8,192 concatenated names,
malformed UTF-8, surrogate/out-of-range scalar sequences, unrepresentable
Unicode, and 256 error/success cycles. Windows is implemented but was not run
on this host. This is a codec test, not a full-game activation claim.

```
cd pc-port
PATH=/opt/homebrew/opt/llvm/bin:$PATH xmake build -j8 smg-pc-text-encoding-tests
./build/macosx/arm64/debug/smg-pc-text-encoding-tests
```

From the repository root, the isolated sanitizer command is:

```
/opt/homebrew/opt/llvm/bin/clang++ -std=c++23 -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -I pc-port/src pc-port/tests/TextEncodingTests.cpp pc-port/src/resource/TextEncoding.cpp -liconv -o build/text-encoding-sanitized
ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ./build/text-encoding-sanitized
```

## Audited boundaries for the pending compiler view

The original sjiswrap compiler emits ordinary narrow literals in CP932.
Clang currently emits UTF-8, while BcsvTable/JMapInfo preserve original disc
bytes. Changing only Mario's byte-prefix predicates would conceal the same
problem elsewhere; the planned source-literal compiler view preserves those
original methods. Before enabling it globally, these native boundaries need
coherent changes and validation:

| Boundary | Current evidence and required contract |
| --- | --- |
| NameObj registry | `register_name_obj_runtime_state` and `update_name_obj_runtime_name` retain the supplied bytes. Preserve that behavior for Game names. |
| Native layout adapter | `LayoutDrawAdaptor` passes native `LayoutRuntime::getName()` to NameObj. That host-to-Game bridge needs explicit encoding when Game names become CP932. |
| Scheduler trace/snapshots | `entry_name`, `sensor_host_name`, connection trace, and BCK/BRK/BTK debug names feed UTF-8 presentation/JSON. Decode Game strings at those sinks; native Layout strings already use UTF-8. Do not decode raw identity storage. |
| ObjectNameTable | `lookup` returns eagerly decoded Japanese names; `StageHostScene::resolve_actor_name` passes the result into actor construction. Retain CP932 for that API and decode only where displayed. |
| Shadow CSV/registry | CSV retains `name_raw` but both registry lookup overloads compare decoded `name`. The controller constructor also copies the input into both fields. Pass raw CP932 into creation, decode the display copy, and compare raw identity. CSV joint lookup and line endpoint linking already use their raw fields. |
| LightData | Both area-light and zone-light tables decode names before assigning original Game fields. A separate staged patch restores raw names and explicit test presentation decoding. |
| DemoSheet/GeneralPos | DemoSheet decodes all string columns; DemoSceneRuntime compares them with Game demo/part/cast requests and passes animation names into original MR APIs. GeneralPos names are also decoded. Their identity and native default-name literals must migrate together, with presentation decoding at trace/error boundaries. |
| EventCamera | Catalog decodes authored `e:` IDs, while `CameraUtilCompat` passes original Game event names to native camera-system lookup. Preserve matching CP932 identity across both ends until the full original camera owner replaces this facade. |
| PlanetMapCatalog | Eagerly decodes planet/scenario names; complete original factory activation will replace this authority. Do not assume all possible authored keys are ASCII just because current examples are. |

The existing native SceneScheduler also sends names into legacy EffectService
host bindings. The original EffectSystem activation is being prepared
separately; retire its legacy provider atomically rather than adding a second
effect-name authority. This audit is not an assertion that the full shadow,
camera, demo, or factory owners are already original implementations.
