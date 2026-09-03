#!/usr/bin/env python3
"""Reproduce the bounded Aurora emulated-CPU API checks and retain evidence."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
BUILD = ROOT / "build/aurora-upstream-merge-tests"
LOCAL = ROOT / "build/aurora-os-execution-20260903"
NORMAL = BUILD / "tests/os_execution_tests"
SANITIZED = LOCAL / "os_execution_tests_tsan"
parser = argparse.ArgumentParser()
parser.add_argument("--run", action="store_true")
args = parser.parse_args()


def run(command, log):
    with (NOTES / log).open("w") as output:
        subprocess.run([str(part) for part in command], cwd=ROOT, stdout=output,
                       stderr=subprocess.STDOUT, check=True)


if args.run:
    LOCAL.mkdir(parents=True, exist_ok=True)
    run(["cmake", "--build", BUILD, "--parallel", "2", "--target", "os_execution_tests"], "build.log")
    run([NORMAL, f"--gtest_output=xml:{NOTES / 'os-execution-tests.xml'}"], "os-execution-tests.log")
    run([
        "clang++", "-std=c++20", "-O1", "-g", "-fsanitize=thread", "-DTARGET_PC", "-DAURORA",
        f"-I{ROOT / 'pc-port/aurora/include'}",
        f"-I{BUILD / '_deps/googletest-src/googletest/include'}",
        ROOT / "pc-port/aurora/lib/dolphin/os/OSExecution.cpp",
        ROOT / "pc-port/aurora/tests/os_execution_test.cpp",
        BUILD / "lib/libgtest_main.a", BUILD / "lib/libgtest.a", "-pthread", "-o", SANITIZED,
    ], "build-tsan.log")
    run([SANITIZED, f"--gtest_output=xml:{NOTES / 'os-execution-tsan.xml'}"], "os-execution-tsan.log")

dol = (ROOT / "build/compat-math-oracle/main.dol").read_bytes()
assert hashlib.sha1(dol).hexdigest() == "25c5959534b3c21246c6c7e42021b916b41fb578"


def at(address, size):
    offsets = struct.unpack_from(">18I", dol, 0)
    addresses = struct.unpack_from(">18I", dol, 0x48)
    lengths = struct.unpack_from(">18I", dol, 0x90)
    for offset, start, length in zip(offsets, addresses, lengths):
        if start <= address and address + size <= start + length:
            return dol[offset + address - start:offset + address - start + size]
    raise ValueError(f"Unmapped DOL address {address:#x}")


checks = {
    0x804A9778: 0x7C6000A6,  # Read old MSR.
    0x804A977C: 0x5464045E,  # Clear EE.
    0x804A9784: 0x54638FFE,  # Return old EE bit.
    0x804A9790: 0x60648000,  # Enable sets EE.
    0x804A9798: 0x54638FFE,
    0x804A97A0: 0x2C030000,  # Restore tests zero, not just TRUE == 1.
    0x804A97A8: 0x4182000C,
    0x804A97AC: 0x60858000,
    0x804A97B4: 0x5485045E,
    0x804A97BC: 0x54838FFE,
    0x804AC590: 0x4BFFD1E9,  # Scheduler disable calls interrupt disable.
    0x804AC594: 0x83EDE0C8,  # Both load Reschedule from the same SDA slot.
    0x804AC598: 0x381F0001,
    0x804AC59C: 0x900DE0C8,
    0x804AC5A4: 0x7FE3FB78,  # Return value before the increment.
    0x804AC5D0: 0x83EDE0C8,
    0x804AC5D4: 0x381FFFFF,
    0x804AC5D8: 0x900DE0C8,
    0x804AC5E0: 0x7FE3FB78,
    0x804AC8B8: 0x800DE0C8,  # SelectThread uses the same global count.
    0x804AC8BC: 0x2C000000,  # Signed compare.
    0x804AC8C0: 0x4081000C,  # Continue only for <= 0.
    0x804ACAF0: 0x4BFFCC89,  # Yield disables interrupts.
    0x804ACAF8: 0x38600001,  # True argument to SelectThread.
    0x804ACAFC: 0x4BFFFDA5,
    0x804ACB04: 0x4BFFCC9D,  # Restore saved interrupt state.
}
for address, expected in checks.items():
    assert struct.unpack(">I", at(address, 4))[0] == expected, f"Retail word changed: {address:#x}"

functions = {
    "OSDisableInterrupts": (0x804A9778, 0x14),
    "OSEnableInterrupts": (0x804A978C, 0x14),
    "OSRestoreInterrupts": (0x804A97A0, 0x24),
    "OSDisableScheduler": (0x804AC580, 0x3C),
    "OSEnableScheduler": (0x804AC5BC, 0x3C),
    "OSYieldThread": (0x804ACAE0, 0x3C),
}
sources = [
    "src/RVL_SDK/os/OSInterrupt.c", "src/RVL_SDK/os/OSThread.c",
    "pc-port/aurora/lib/dolphin/os/OSExecution.cpp",
    "pc-port/aurora/include/dolphin/os.h",
    "pc-port/aurora/include/dolphin/os/OSThread.h",
    "pc-port/aurora/cmake/aurora_os.cmake", "pc-port/aurora/xmake.lua",
    "pc-port/aurora/tests/CMakeLists.txt", "pc-port/aurora/tests/os_execution_test.cpp",
]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


test_results = {}
for label, stem, binary in [
    ("normal", "os-execution-tests", NORMAL),
    ("thread_sanitizer", "os-execution-tsan", SANITIZED),
]:
    result = ET.parse(NOTES / f"{stem}.xml").getroot()
    assert result.get("tests") == "12"
    assert result.get("failures") == "0" and result.get("errors") == "0"
    log = (NOTES / f"{stem}.log").read_text()
    assert "WARNING: ThreadSanitizer" not in log and "SUMMARY: ThreadSanitizer" not in log
    test_results[label] = {
        "tests": int(result.get("tests")), "failures": 0, "errors": 0,
        "seconds": result.get("time"), "binary_sha256": digest(binary),
        "log_sha256": digest(NOTES / f"{stem}.log"),
    }

output = {
    "scope": "Cooperative emulated SDK API callers; no host priority or interrupt dispatch emulation",
    "dol_sha1": hashlib.sha1(dol).hexdigest(),
    "retail_instruction_checks": len(checks),
    "retail_functions": {
        name: {"address": f"0x{address:08X}", "size": size,
               "sha256": hashlib.sha256(at(address, size)).hexdigest()}
        for name, (address, size) in functions.items()
    },
    "source_sha256": {source: digest(ROOT / source) for source in sources},
    "tests": test_results,
}
(NOTES / "source-evidence.json").write_text(json.dumps(output, indent=2) + "\n")
print(f"Verified {len(checks)} retail words; normal 12/12 and ThreadSanitizer 12/12 passed.")
