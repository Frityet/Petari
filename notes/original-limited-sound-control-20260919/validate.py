#!/usr/bin/env python3
"""Build and run the retained original JAI ownership regression modes."""

import hashlib
import json
import os
from pathlib import Path
import subprocess
import time


ROOT = Path(__file__).resolve().parents[2]
NOTES = Path(__file__).resolve().parent
BINARY = ROOT / "build/macosx/arm64/debug/smg-pc-original-jai-sound-ownership-tests"
FIXTURE = ROOT / "notes/original-audio-category-volume-20260907/fixture"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def point_light_objects():
    return {
        str(path.relative_to(ROOT)): digest(path)
        for path in (ROOT / "build/.objs").glob("**/PointLightRuntimeTests.cpp.o")
    }


def main():
    assert (FIXTURE / "KrKorean/AudioRes/SMR.szs").is_file()
    assert (FIXTURE / "AudioRes/Stream/SMG_title_strm.ast").is_file()
    environment = os.environ.copy()
    environment["SMGPC_RETAIL_FILES_ROOT"] = str(FIXTURE)
    result = {
        "fixture": str(FIXTURE.relative_to(ROOT)),
        "point_light_objects_before": point_light_objects(),
        "commands": [],
    }
    commands = [
        ("build", ["xmake", "build", "smg-pc-original-jai-sound-ownership-tests"]),
        ("scene-only", [str(BINARY), "--scene-only"]),
        ("backend-only", [str(BINARY), "--backend-only"]),
        ("full-retail", [str(BINARY)]),
    ]
    try:
        for name, command in commands:
            start = time.monotonic()
            print(f"Running {name}", flush=True)
            with (NOTES / f"{name}.log").open("w") as log:
                completed = subprocess.run(
                    command, cwd=ROOT, env=environment,
                    stdout=log, stderr=subprocess.STDOUT,
                )
            record = {
                "name": name,
                "command": command,
                "returncode": completed.returncode,
                "elapsed_seconds": round(time.monotonic() - start, 3),
            }
            result["commands"].append(record)
            output = (NOTES / f"{name}.log").read_text()
            print(json.dumps(record), flush=True)
            if name != "build":
                print(output, end="", flush=True)
                record["fixture_skipped"] = "[skip]" in output
            if completed.returncode:
                raise RuntimeError(f"{name} exited {completed.returncode}: see {name}.log")
            if record.get("fixture_skipped"):
                raise RuntimeError(f"{name} skipped required retail coverage")
            if name == "full-retail":
                assert "[pass] original limited-sound slots" in output
        result["binary_sha256"] = digest(BINARY)
    finally:
        result["point_light_objects_after"] = point_light_objects()
        result["point_light_objects_unchanged"] = (
            result["point_light_objects_before"] == result["point_light_objects_after"]
        )
        (NOTES / "validation.json").write_text(json.dumps(result, indent=2) + "\n")
    assert result["point_light_objects_unchanged"]


if __name__ == "__main__":
    main()
