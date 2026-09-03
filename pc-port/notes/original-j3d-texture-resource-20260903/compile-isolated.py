#!/usr/bin/env python3
from pathlib import Path
import json, shlex, subprocess
root = Path(__file__).resolve().parents[3]
pc = root / "pc-port"
out = root / "build/original-j3d-texture-resource-20260903"
out.mkdir(parents=True, exist_ok=True)
entries = json.loads((pc/"compile_commands.json").read_text())
e = next(e for e in entries if e["file"].endswith("/J3DModelDataCompat.cpp"))
base = e.get("arguments") or shlex.split(e["command"])
for rel in ["src/resource/Mem1ResourceHeap.cpp", "src/resource/J3dTextureData.cpp", "tests/OriginalJ3DTextureResourceTests.cpp"]:
    name = Path(rel).stem
    args = list(base)
    args[args.index("-o")+1] = str(out/(name+".o"))
    args[-1] = str(pc/rel)
    result = subprocess.run(args, cwd=e["directory"], capture_output=True, text=True)
    (out/(name+".log")).write_text(result.stdout+result.stderr)
    print(name, "exit", result.returncode)
    if result.returncode:
        print(result.stderr)
        raise SystemExit(result.returncode)
