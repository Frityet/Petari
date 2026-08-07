# Data-driven ObjectNameTable runtime names

## Result

Stage placement now retains the English `name` value for factory, archive, and
model selection while passing the corresponding UTF-8 Japanese `jp_name` to
the actor constructor. The mapping is loaded once per `StageHostScene` from
`/StageData/ObjNameTable.arc`, and unknown names retain their English value.
There are no stage-name, route-name, actor-name, or effect-name aliases in the
implementation.

The original explicit-root path required one additional distinction. An
explicit root that matches a real placement row is now constructed with that
row's localized name and initialized from its placement iterator, but it still
receives the requested host-level `appear()` call. A non-empty explicit
`actor_name` override remains authoritative. Truly programmatic explicit roots
that do not match placement data remain English/override-named and use
`initWithoutIter()`. Ordinary placement roots continue to preserve the
appeared/dead state chosen by their own `init()`.

This fixes the concrete prologue case without special-casing it:

- `PrologueDirector` is constructed as `プロローグデモ`.
- It receives `PeachCastleGardenGalaxy` LayerA ObjInfo row 88 (`l_id=136`), so
  its original stage-switch setup is available.
- The same row is then data-identity-deduplicated.
- The explicit root still receives `appear()` after placement initialization.

See `artifacts/prologue-explicit-root-events.tsv` for the ordered trace proof.

## Resource behavior

`ObjectNameTable`:

- locates `ObjNameTable.tbl` by case-insensitive archive basename;
- requires hashed `en_name` and `jp_name` string fields;
- decodes Nintendo-authored CP932 `jp_name` values at the resource boundary;
- owns all keys and mapped values;
- preserves the first row for duplicate English keys;
- exposes stable pointer/null lookup and lookup-or-self fallback APIs.

The RMGK01 and RMGK02 archives used here are byte-identical:

```text
4d56a16857d44a86eb08748a191844a3e80109585cca8c3113163dca2f06183c  RMGK01 ObjNameTable.arc
4d56a16857d44a86eb08748a191844a3e80109585cca8c3113163dca2f06183c  RMGK02 ObjNameTable.arc
```

Both contain 1,691 rows. The focused real-data checks cover Gateway-relevant
mappings including:

- `DemoRabbit` -> `デモウサギ`
- `Rosetta` -> `ロゼッタ`
- `RunawayTico` -> `逃げチコ`
- `Tico` -> `チコ`
- `TicoBaby` -> `ベビチコ`
- `HeavensDoorAppearStepA` ->
  `ヘブンズドアミステリアス惑星階段（デモ中）`
- `HeavensDoorAppearStepAAfter` ->
  `ヘブンズドアミステリアス惑星階段（デモ後）`

The tests also prove that `LightDome`, `DomeHalo`, and `TicoDemoGetPower` are
absent and therefore use the English fallback. This guards against inventing
compatibility aliases for helper/model names that retail data does not define.

## Verification

Focused synthetic and extracted-data test:

```text
xmake run smg-pc-object-name-table-tests
[ok] synthetic lookup and first-row semantics
[ok] schema and archive validation
[ok] RMGK01 extracted ObjNameTable
[ok] RMGK02 extracted ObjNameTable
[ok] optional extracted tables
[skip] DVD service ObjNameTable load (set SMGPC_REAL_DISC)
[ok] optional DVD service load
4/4 tests passed
```

The target sets its run directory to the project root, so the ordinary
`xmake run` command above exercises both checked-out retail archives instead of
silently skipping them due to the binary directory.

Actual disc-image/DvdFileSystemService load:

```text
SMGPC_REAL_DISC=/workspaces/pcport/RMGK01.iso \
  xmake run smg-pc-object-name-table-tests
[ok] RMGK01 extracted ObjNameTable
[ok] RMGK02 extracted ObjNameTable
[ok] optional DVD service load
4/4 tests passed
```

Additional checks:

- Running the test binary directly from `/tmp` still reports 4/4 passed with
  the optional retail-data cases skipped, proving the core coverage is
  self-contained.
- `smg-pc-aurora-native-tests/aurora_native`: 27/27 passed, including the
  explicit placement-backed host-appear invariant.
- `xmake build smg-pc`: passed.
- `git diff --check`: passed.
- `clang-format --dry-run --Werror` on the new table code, StageHost changes,
  and focused test: passed.

## Route preservation

The current binary was driven through the normal title, file-select, and
picturebook route with the RMGK01 ISO. All three captures passed their layout,
trace, and non-black-image validators:

| Capture | Frame | Required layout | Render packets | Non-black ratio |
| --- | ---: | --- | ---: | ---: |
| title | 90 | `TitleLogo` | 22 | 1.0000 |
| file select | 1900 | `FileNumber` | 27 | 0.999831 |
| picturebook | 7600 | `PrologueDemo`, `IconAButton` | 390 | 1.0000 |

The picturebook PNG was also visually inspected and shows the expected Korean
story page and A-button prompt. The initial combined invocation passed title,
then encountered pre-existing exhausted Xvfb lock slots before starting the
second scenario. File select and picturebook were therefore rerun independently
on fixed free displays and both passed; their standalone aggregate manifests
report `status: passed`.

Route evidence is retained under:

- `route/title/`
- `route-file-select/file_select/`
- `route-picturebook/picturebook/`

## Files

- `src/scene/nameobj/ObjectNameTable.hpp`
- `src/scene/nameobj/ObjectNameTable.cpp`
- `src/scene/StageHostScene.hpp`
- `src/scene/StageHostScene.cpp`
- `tests/ObjectNameTableTests.cpp`
- `tests/AuroraNativeTests.cpp`
- `tests/xmake.lua`
- `scripts/aurora_route_smoke.lua`

Command output, resource/source hashes, decoded mapping excerpts, and the
explicit-root trace query are in `artifacts/`.
