#!/usr/bin/env python3
"""Build only the independent real-KCollisionServer resource fixture."""
import hashlib
import json
import os
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BUILD = ROOT / "build/original-collision-owner-20260903"
SOURCES = [
    "pc-port/tests/OriginalKCollisionResourceTests.cpp",
    "pc-port/src/resource/KCollisionResource.cpp",
    "pc-port/src/resource/JMapResource.cpp",
    "pc-port/src/compat/OriginalKCollisionCompat.cpp",
    "pc-port/src/resource/BcsvTable.cpp",
    "pc-port/src/Game/Util/JMapInfo.cpp",
]
BUILD.mkdir(parents=True, exist_ok=True)
results = {}
for name, extra in [("normal", []), ("address-undefined", ["-fsanitize=address,undefined"])]:
    binary = BUILD / f"kcollision-resource-tests-{name}"
    command = ["clang++", "-std=c++20", "-O1", "-g", *extra, "-DTARGET_PC",
               "-Ipc-port/src", "-Ipc-port/aurora/include", *SOURCES, "-o", str(binary)]
    (BUILD / f"native-{name}.command.json").write_text(json.dumps(command, indent=2) + "\n")
    with (HERE / f"build-{name}.log").open("w") as output:
        subprocess.run(command, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT, check=True)
    environment = dict(os.environ, ASAN_OPTIONS="halt_on_error=1", UBSAN_OPTIONS="halt_on_error=1")
    run = subprocess.run([str(binary)], cwd=ROOT, text=True, stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT, env=environment)
    (HERE / f"native-{name}.log").write_text(run.stdout)
    run.check_returncode()
    cases = [line.removeprefix("PASS ") for line in run.stdout.splitlines() if line.startswith("PASS ")]
    assert len(cases) == 10 and "FAIL " not in run.stdout
    assert "runtime error:" not in run.stdout and "ERROR: AddressSanitizer" not in run.stdout
    results[name] = {"case_count": len(cases), "cases": cases,
                     "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(), "returncode": run.returncode,
                     "output_sha256": hashlib.sha256(run.stdout.encode()).hexdigest()}
    print(name, f"{len(cases)}/10 passed")
result = {"tests": results, "source_sha256": {source: hashlib.sha256((ROOT / source).read_bytes()).hexdigest() for source in SOURCES},
          "scope": "Independent actual KCollisionServer resource lifecycle; no CollisionParts or scene query activation, no shared xmake/GPU build."}
(HERE / "native-evidence.json").write_text(json.dumps(result, indent=2) + "\n")
