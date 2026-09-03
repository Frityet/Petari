#!/usr/bin/env python3
"""Build/run the isolated CPU GD/FIFO suite, or record its current evidence."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
AURORA = ROOT / "pc-port/aurora"
BUILD = ROOT / "build/aurora-upstream-merge-tests"
FILES = (
    "include/dolphin/gd/GDGeometry.h", "include/dolphin/gx/GXGeometry.h",
    "lib/dolphin/gd/GDBase.cpp", "lib/dolphin/gd/GDGeometry.cpp", "lib/dolphin/gx/GXGeometry.cpp",
    "tests/CMakeLists.txt", "tests/gd_array_test.cpp",
)


def run(command, log):
    result = subprocess.run(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=120)
    (NOTES / log).write_text(result.stdout)
    result.check_returncode()


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--run", action="store_true")
    options = parser.parse_args()
    build_command = ["cmake", "--build", str(BUILD), "--parallel", "2", "--target", "gx_fifo_tests"]
    test_command = [str(BUILD / "tests/gx_fifo_tests"), "--gtest_output=xml:" + str(NOTES / "gx-fifo-tests.xml")]
    if options.run:
        run(build_command, "build.log")
        run(test_command, "gx-fifo-tests.log")
    xml = ET.parse(NOTES / "gx-fifo-tests.xml").getroot()
    assert int(xml.attrib["tests"]) == 244
    assert int(xml.attrib["failures"]) == int(xml.attrib["errors"]) == 0
    gd = next(s for s in xml if s.attrib.get("name") == "GDArrayTest")
    assert int(gd.attrib["tests"]) == 11 and int(gd.attrib["failures"]) == 0
    report = {
        "scope": "CPU display-list emission and real FIFO replay; no GPU/full-game validation",
        "build_command": build_command, "test_command": test_command,
        "total_tests": 244, "new_gd_tests": 11, "failures": 0,
        "test_cases": [c.attrib["name"] for c in gd if c.tag == "testcase"],
        "source_sha256": {p: sha(AURORA / p) for p in FILES},
        "original_contract_sha256": sha(ROOT / "src/RVL_SDK/gd/GDGeometry.c"),
        "physical_address_provider_sha256": sha(AURORA / "lib/dolphin/os/OSAddress.cpp"),
        "test_binary_sha256": sha(BUILD / "tests/gx_fifo_tests"),
        "raw_array_command_bytes": 16, "base_and_stride_command_bytes": 22,
        "twelve_array_vcd_vat_bytes": 303, "twelve_array_vcd_vat_padded_bytes": 320,
    }
    (NOTES / "source-evidence.json").write_text(json.dumps(report, indent=2) + "\n")
    print("244/244 CPU FIFO/display-list tests pass, including 11 GD/array-base cases")


if __name__ == "__main__":
    main()
