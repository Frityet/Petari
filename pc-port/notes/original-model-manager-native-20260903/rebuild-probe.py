#!/usr/bin/env python3
"""Rebuild the reviewed isolated owner/probe; never run xmake or publish sources.

Requires the preceding verified ModelX preparation and the staged owner files
listed in source-evidence.json. Build/GPU slots remain coordinated by the parent.
"""
from pathlib import Path
import json
import re
import shutil
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
PC = ROOT / "pc-port"
BUILD = ROOT / "build/original-model-manager-native-20260903"
MODELX = ROOT / "build/original-modelx-native-20260903"
DRAW = ROOT / "build/original-model-manager-render-20260903"


def run(command, log):
    result = subprocess.run(command, cwd=PC, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    log.write_text(result.stdout)
    result.check_returncode()


def main():
    # Capture the current Game target's actual compiler/ABI flags. This does not
    # ask xmake to regenerate or mutate any shared object or archive.
    dependency = PC / "build/.deps/smg-pc-game/macosx/arm64/debug/src/Game/System/ResourceHolder.cpp.o.d"
    values = re.search(r"values = \{(.*?)\n    \}", dependency.read_text(), re.S)[1]
    flags = [json.loads('"' + value + '"') for value in re.findall(r'"((?:\\.|[^"\\])*)"', values)]
    prefix = [flags[0], "-I" + str(BUILD), "-I" + str(MODELX / "staged/include"),
              "-I" + str(MODELX / "staged/native-sdk"), *flags[1:]]
    assert (PC / "src/runtime/RuntimeContext.hpp").read_bytes() == (MODELX / "staged/include/runtime/RuntimeContext.hpp").read_bytes(), "Refresh the staged RuntimeContext header before linking current libraries"
    commands = []
    sources = [(BUILD / (name + ".cpp"), BUILD / (name + ".o"), []) for name in (
        "ModelManager", "MaterialCtrl", "DisplayListMaker", "ShadowPublish", "Helpers", "ModelManagerOwner",
        "ActorRuntimeRegistry", "LodCtrlRuntimeCompat", "LiveActor")]
    sources += [(MODELX / "staged" / (name + ".cpp"), MODELX / (name + ".o"), []) for name in (
        "J3DModelX", "J3DModelXDrawCompat", "NewJ3DModel", "ModelXFog")]
    draw_flags = ["-I" + str(DRAW / "staged"), "-include", str(DRAW / "staged/MetrowerksDrawAlgorithm.hpp")]
    sources += [(DRAW / "staged/Game/System" / (name + ".cpp"), DRAW / (name + ".o"), draw_flags) for name in (
        "DrawBuffer", "DrawBufferExecuter", "DrawBufferGroup", "DrawBufferHolder")]
    sources += [(BUILD / "draw-live.cpp", BUILD / "draw-live.o", ["-I" + str(DRAW / "staged")])]
    for source, output, extra in sources:
        command = [prefix[0], *extra, *prefix[1:], "-c", str(source), "-o", str(output)]
        commands.append(command)
        run(command, output.with_suffix(".refresh.log"))
        print("compiled", source.name, flush=True)

    # Replace conflicting providers only inside this private archive copy.
    archive = BUILD / "libsmg-pc-game.a"
    shutil.copy2(PC / "build/macosx/arm64/debug/libsmg-pc-game.a", archive)
    subprocess.run(["/opt/homebrew/opt/llvm/bin/llvm-ar", "d", str(archive), "MaterialCtrlCompat.cpp.o",
                    "ActorRuntimeRegistry.cpp.o", "LodCtrlRuntimeCompat.cpp.o", "LiveActor.cpp.o"], check=True)
    command = [value.replace("${ROOT}", str(ROOT)) for value in json.loads((HERE / "draw-link-template.json").read_text())]
    assert "-Wl,-dead_strip" in command
    commands.append(command)
    run(command, BUILD / "draw-live.refresh.link.log")
    (BUILD / "refresh-commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    print("linked", BUILD / "draw-live", flush=True)


if __name__ == "__main__":
    main()
