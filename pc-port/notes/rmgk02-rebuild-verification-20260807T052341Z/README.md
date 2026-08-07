# RMGK02 decomp rebuild verification

The root decomp builds normally with the RMGK02 configuration after the
real-or-absent source-boundary changes.

Commands run from the repository root:

```text
ninja
sha1sum -c config/RMGK02/build.sha1
```

Result:

```text
ninja: success
build/RMGK02/main.dol: OK
```

The build reports 64.72% matched overall and 61.84% matched for Game. The only
compiler diagnostic in this incremental invocation was the pre-existing
Metrowerks implicit member-pointer conversion warning in `StarPiece.cpp`; it
did not affect the exact DOL checksum.

This verification deliberately uses RMGK02, as permitted for the PC-port work,
and does not alter the retail configuration or checksum manifest.
