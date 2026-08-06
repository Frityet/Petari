# Reproduction commands

The evidence used only read-only source/disc probes plus temporary extraction
under `/tmp/smg-gateway-demo-sheet-oracle*`.

```sh
# Identify and hash both local RMGK01 representations.
sha256sum RMGK01.iso 'Super Mario Wii - Galaxy Adventure (Korea).rvz'
pc-port/dolphin/build-nogui-libcxx/Binaries/dolphin-tool header -i RMGK01.iso
pc-port/dolphin/build-nogui-libcxx/Binaries/dolphin-tool header \
  -i 'Super Mario Wii - Galaxy Adventure (Korea).rvz'

# Extract only the relevant revision-0 files.
tool=pc-port/dolphin/build-nogui-libcxx/Binaries/dolphin-tool
out=/tmp/smg-gateway-demo-sheet-oracle
$tool extract -q -i RMGK01.iso -g -s ObjectData/DemoSheet.arc -o "$out"
$tool extract -q -i RMGK01.iso -g \
  -s StageData/HeavensDoorMysteriousZone.arc -o "$out"
$tool extract -q -i RMGK01.iso -g \
  -s StageData/HeavensDoorGalaxy/HeavensDoorGalaxyScenario.arc -o "$out"

# Build and run the existing, unchanged resource probes.
cd pc-port
xmake build smg-pc-rarc-probe
xmake build smg-pc-bcsv-probe
cd ..
rarc=pc-port/build/linux/x86_64/debug/smg-pc-rarc-probe
bcsv=pc-port/build/linux/x86_64/debug/smg-pc-bcsv-probe
sheet="$out/DATA/files/ObjectData/DemoSheet.arc"
$rarc "$sheet" | rg '^demospingetdemo|^demoticoguidedemo'
$bcsv "$sheet" demoticoguidedemo/demoticoguidedemotime.bcsv \
  | iconv -f CP932 -t UTF-8
$bcsv "$sheet" demoticoguidedemo/demoticoguidedemoaction.bcsv \
  | iconv -f CP932 -t UTF-8

# Decode the Gateway demo-group table.
zone="$out/DATA/files/StageData/HeavensDoorMysteriousZone.arc"
$bcsv "$zone" jmp/placement/layera/demoobjinfo | iconv -f CP932 -t UTF-8
$bcsv "$zone" jmp/placement/layera/objinfo | iconv -f CP932 -t UTF-8
$bcsv "$zone" jmp/childobj/layera/childobjinfo | iconv -f CP932 -t UTF-8

# Prove that the lock payload is a username, not a sheet alias.
$rarc "$sheet" demospingetdemolockfile.txt /tmp/spin-lock.txt
$rarc "$sheet" demoticoguidedemolockfile.txt /tmp/tico-lock.txt
xxd -g 1 /tmp/spin-lock.txt /tmp/tico-lock.txt

# Show the actual original call and absence of a SpinGetDemo caller.
rg -n 'startTimeKeepDemoMarioPuppetable.*チコガイドデモ.*スピンゲット' \
  src/Game/NPC/RosettaDemoHeavensDoor.cpp
rg -n '"スピンゲットデモ"|SpinGetDemo' src include

# Read revision-1 FST sizes. The RMGK02 data bodies are not present locally.
python3 -c '<parse big-endian FST records and print selected file lengths>'
```

Source oracles followed:

- `src/Game/Demo/DemoFunction.cpp`: archive loading and
  `Demo<sheet><suffix>.bcsv` lookup.
- `src/Game/Demo/DemoTimeKeeper.cpp`: ordered part/step semantics.
- `src/Game/Demo/DemoExecutor.cpp`: update and keeper-dispatch order.
- `src/Game/Demo/DemoActionKeeper.cpp`: cast matching and action types 0-13.
- `src/Game/Demo/DemoPlayerKeeper.cpp` and `DemoCameraKeeper.cpp`.
- `build/RMGK02/asm/Game/Demo/DemoWipeKeeper.s`: wipe schema and type mapping.
- `src/Game/NPC/RosettaDemoHeavensDoor.cpp` and
  `src/Game/NPC/TicoDemoGetPower.cpp`: actual spin start and actor-owned events.
