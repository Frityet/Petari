# Verification

Focused compile:

```text
ninja -j12 build/RMGK02/src/Game/Player/Mario.o
PASS
```

The only diagnostics were the two pre-existing non-trivial-union warnings in `MarioActor.hpp` at lines 684 and 905.

Focused comparison:

```text
build/tools/objdiff-cli diff -p . -u main/Game/Player/Mario \
  -o pc-port/notes/mario-walking-core-rmgk02-20260809T043114Z/objdiff-final.json \
  --format json-pretty
PASS
```

Full link and retail comparison:

```text
ninja -j12 build/RMGK02/main.dol
PASS

sha256sum build/RMGK02/main.dol orig/RMGK02/sys/main.dol
8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf  build/RMGK02/main.dol
8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf  orig/RMGK02/sys/main.dol

cmp -s build/RMGK02/main.dol orig/RMGK02/sys/main.dol
PASS (status 0)
```

Whitespace/error check:

```text
git diff --check -- include/Game/Player/Mario.hpp src/Game/Player/Mario.cpp
PASS
```
