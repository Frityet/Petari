# Verification transcript

## Focused compile

Command:

```text
ninja -j12 build/RMGK02/src/Game/Player/MarioDamage.o
```

Result: pass. The only output was the two existing `MarioActor.hpp` nontrivial-union warnings at lines 684 and 905.

## Objdiff

Command:

```text
build/tools/objdiff-cli diff -p . -u main/Game/Player/MarioDamage \
  -o pc-port/notes/mario-damage-rmgk02-20260809T034609Z/objdiff-final.json \
  --format json-pretty
```

Final section summary:

```text
.text    12756    98.96457
.ctors       4   100.00000
.data      984    52.427185
.sdata2     56   100.00000
```

Function summary:

```text
functions                     56
mean match             99.16357%
match >= 95%                  54
match >= 99%                  47
match == 100%                 28
```

The only functions below 95% are `Mario::isDamaging() const` (94.520546%) and `Mario::doFireObjHitWithInitialDamage()` (83.77778%); both are complete and semantically reconstructed. The unit-level text score remains 98.96457%.

## Full DOL

Commands:

```text
ninja -j12 build/RMGK02/main.dol
sha256sum build/RMGK02/main.dol orig/RMGK02/sys/main.dol
cmp -s build/RMGK02/main.dol orig/RMGK02/sys/main.dol
```

Result:

```text
8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf  build/RMGK02/main.dol
8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf  orig/RMGK02/sys/main.dol
cmp: identical
```

## Diff check

Tracked owned paths were checked with `git diff --check`; the new source was checked with `git diff --no-index --check /dev/null src/Game/Player/MarioDamage.cpp`. Result: clean.
