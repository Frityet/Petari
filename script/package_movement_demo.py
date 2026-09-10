#!/usr/bin/env python3
"""Package a selected, already-validated local showcase binary without rebuilding.

--dry-run inspects inputs and prints provenance; it creates no app or output files.
The expected hash must come from the binary selected for runtime validation.
"""

import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import plistlib
import re
import shlex
import shutil
import stat
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE_SCOPES = ("src", "tests", "script", "xmake.lua", ".gitmodules", "aurora", "decomp")
APP_NAME = "Super Mario Galaxy Movement Demo"


def run(*command, cwd=ROOT):
    return subprocess.check_output(command, cwd=cwd, stderr=subprocess.PIPE)


def sha256(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def dirty_records(repository, scopes):
    entries = run("git", "status", "--porcelain=v1", "-z", "--untracked-files=all",
                  "--ignore-submodules=none", "--", *scopes, cwd=repository).split(b"\0")
    result = []
    index = 0
    while index < len(entries) and entries[index]:
        entry = entries[index]
        index += 1
        status_code = entry[:2].decode("ascii")
        name = os.fsdecode(entry[3:])
        record = {"status": status_code, "path": name}
        if "R" in status_code or "C" in status_code:
            record["original_path"] = os.fsdecode(entries[index])
            index += 1
        path = repository / name
        if path.is_symlink():
            record["symlink_target"] = os.readlink(path)
        elif path.is_file():
            record.update(sha256=sha256(path), size=path.stat().st_size)
        elif path.is_dir():
            record["kind"] = "directory_or_submodule"
        else:
            record["kind"] = "deleted_or_missing"
        result.append(record)
    return result


def repository_snapshot(repository, scopes=(".",), visited=None):
    visited = set() if visited is None else visited
    repository = repository.resolve()
    if repository in visited:
        raise ValueError(f"recursive submodule path: {repository}")
    visited.add(repository)
    snapshot = {
        "repository": str(repository),
        "head": run("git", "rev-parse", "HEAD", cwd=repository).decode().strip(),
        "scopes": list(scopes),
        "dirty_paths": dirty_records(repository, scopes),
        "submodules": [],
    }
    # Index records retain the commit expected by each parent, including dirty
    # or uninitialized submodules. Recursion hashes only their reported changes.
    for entry in run("git", "ls-files", "--stage", "-z", "--", *scopes,
                     cwd=repository).split(b"\0"):
        if not entry:
            continue
        metadata, raw_path = entry.split(b"\t", 1)
        mode, object_id, stage = metadata.split()
        if mode != b"160000":
            continue
        name = os.fsdecode(raw_path)
        child = repository / name
        record = {"path": name, "index_commit": object_id.decode(), "index_stage": stage.decode()}
        if (child / ".git").exists():
            record["checkout"] = repository_snapshot(child, visited=visited)
        else:
            record["initialized"] = False
        snapshot["submodules"].append(record)
    return snapshot


def binary_requirements(binary):
    with binary.open("rb") as stream:
        header = stream.read(16)
    if len(header) != 16 or struct.unpack("<4I", header)[:2] != (0xFEEDFACF, 0x0100000C):
        raise ValueError("binary must be a native arm64 Mach-O executable")
    if struct.unpack("<4I", header)[3] != 2:
        raise ValueError("selected Mach-O is not an executable")
    load_commands = run("/usr/bin/otool", "-l", str(binary)).decode()
    version = re.search(r"cmd LC_BUILD_VERSION\b.*?\n\s*minos (\S+)", load_commands, re.S)
    if version is None:
        version = re.search(r"cmd LC_VERSION_MIN_MACOSX\b.*?\n\s*version (\S+)", load_commands, re.S)
    if version is None:
        raise ValueError("could not determine the executable's minimum macOS version")
    libraries = []
    for line in run("/usr/bin/otool", "-L", str(binary)).decode().splitlines()[1:]:
        library = line.strip().split(" (compatibility version", 1)[0]
        if library:
            libraries.append(library)
    external = [item for item in libraries
                if not item.startswith(("/System/Library/", "/usr/lib/"))]
    if external:
        raise ValueError("binary has dependencies this local packager cannot bundle: " + ", ".join(external))
    run("/usr/bin/codesign", "--verify", str(binary))
    return {"architecture": "arm64", "minimum_macos": version[1],
            "linked_libraries": libraries, "binary_signature_verified": True,
            "bundle_signing": "local bundle; no distribution signing or notarization performed"}


def launcher_text(disc):
    return """#!/bin/sh
set -eu
demo_bin_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
demo_log_dir="$HOME/Library/Logs/PetariDemo"
mkdir -p "$demo_log_dir"
demo_log="$demo_log_dir/movement-$(date +%Y%m%d-%H%M%S)-$$.log"
exec >"$demo_log" 2>&1
demo_disc=${SMGPC_DEMO_DISC:-""" + shlex.quote(str(disc)) + """}
if [ ! -r "$demo_disc" ]; then
    printf 'Disc image is unavailable: %s\\n' "$demo_disc"
    /usr/bin/osascript -e 'display alert "Game disc unavailable" message "Restore the game disc to its original location, then reopen the demo." as critical' || true
    exit 1
fi
# Relative runtime outputs go beside the log, never into the app or game disc.
cd "$demo_log_dir"
exec "$demo_bin_dir/smg-pc-showcase" gateway --disc "$demo_disc" --width 1280 --height 720 "$@"
"""


def readme_text(disc, minimum_macos, validation_note):
    checked = validation_note.strip() if validation_note else (
        "No gameplay validation report was supplied with this package. Packaging checks alone do not establish working movement, jumping or camera behavior.")
    return f"""{APP_NAME}

Open {APP_NAME}.app and click the game window.
Requires an Apple Silicon Mac running macOS {minimum_macos} or later.
The game reads your existing disc at:
{disc}
Keep that file in place. No disc image or extracted game data is included.

Keyboard and mouse
- W, A, S, D: movement stick.
- Space, Enter or left mouse button: A / jump / confirm.
- Arrow keys: game camera requests.
- C: reset the game camera when the game permits it.
- Z: crouch input.
- X: swing / spin input when available in the game.
- Mouse: pointer.
- Close the game window to quit.

F9 toggles the development free camera. Press it again to return to the game
camera. While free camera is active, WASD moves the camera, the mouse looks
around, Space rises and Left Shift descends; Mario's input is suspended.
The controls above describe bindings; actions can depend on the game state.

What was checked
{checked}

If the app closes or gets stuck, the logs are in:
~/Library/Logs/PetariDemo/
Use Finder > Go > Go to Folder to open that location.

This is a local development demo. The app's Contents/Resources/provenance.json
records the exact executable hash, current source checkout and scoped changes.
That checkout snapshot is taken at packaging time; it does not prove which
source files were used by the compiler. The binary is copied unchanged.
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=ROOT / "build/macosx/arm64/debug/smg-pc-showcase")
    parser.add_argument("--disc", required=True, type=Path)
    parser.add_argument("--expected-sha256", required=True, help="SHA256 of the exact runtime-tested executable")
    parser.add_argument("--output", type=Path, default=ROOT / "build/playable-demo" / f"{APP_NAME}.app")
    parser.add_argument("--validation-note", type=Path, help="plain-text description of the runtime checks and known limitations")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    try:
        binary = args.binary.expanduser().resolve(strict=True)
        disc = args.disc.expanduser().resolve(strict=True)
        output = args.output.expanduser().absolute()
        expected_hash = args.expected_sha256.lower()
        if not re.fullmatch(r"[0-9a-f]{64}", expected_hash):
            raise ValueError("--expected-sha256 must contain exactly 64 hexadecimal digits")
        if not binary.is_file() or not os.access(binary, os.X_OK):
            raise ValueError("binary must be an existing executable file")
        if not disc.is_file() or not os.access(disc, os.R_OK):
            raise ValueError("--disc must name a readable existing local disc image")
        if output.exists() or output.is_symlink():
            raise ValueError(f"output already exists; choose a new path: {output}")
        if output.suffix != ".app":
            raise ValueError("--output must end in .app")
        if sha256(binary) != expected_hash:
            raise ValueError("binary differs from --expected-sha256; select or validate the new build first")
        requirements = binary_requirements(binary)
        validation = None
        validation_note = None
        if args.validation_note:
            note_path = args.validation_note.expanduser().resolve(strict=True)
            note_bytes = note_path.read_bytes()
            validation_note = note_bytes.decode("utf-8")
            validation = {"path": str(note_path), "sha256": hashlib.sha256(note_bytes).hexdigest(),
                          "description": "caller-supplied report; not independently executed by the packager"}
        provenance = {
            "schema_version": 1,
            "packaged_at_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
            "source_binary": str(binary), "binary_sha256": expected_hash,
            "binary_size": binary.stat().st_size, "binary_mtime_ns": binary.stat().st_mtime_ns,
            "output_app": str(output), "external_disc": str(disc), "bundled_game_assets": False,
            "launch_arguments": ["gateway", "--disc", str(disc), "--width", "1280", "--height", "720"],
            "frame_limit": None, "runtime_validation_report": validation,
            "platform": requirements, "source_checkout_at_packaging": repository_snapshot(ROOT, SOURCE_SCOPES),
            "provenance_limit": "Packaging-time checkout snapshot, not a build attestation. Binary identity is pinned to the caller's expected SHA256. No runtime validation is performed by this script.",
            "packaging_script_sha256": sha256(Path(__file__)),
        }
        if args.dry_run:
            print(json.dumps(provenance, indent=2, ensure_ascii=False))
            return
        output.parent.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix=".movement-demo-", dir=output.parent) as temporary:
            staged = Path(temporary) / output.name
            macos = staged / "Contents/MacOS"
            resources = staged / "Contents/Resources"
            macos.mkdir(parents=True)
            resources.mkdir(parents=True)
            copied_binary = macos / "smg-pc-showcase"
            shutil.copyfile(binary, copied_binary)
            copied_binary.chmod(stat.S_IMODE(binary.stat().st_mode) | 0o111)
            launcher = macos / "launch-movement-demo"
            launcher.write_text(launcher_text(disc), encoding="utf-8")
            launcher.chmod(0o755)
            run("/bin/sh", "-n", str(launcher))
            readme = readme_text(disc, requirements["minimum_macos"], validation_note)
            (resources / "README.txt").write_text(readme, encoding="utf-8")
            if validation_note is not None:
                (resources / "runtime-validation.txt").write_text(validation_note, encoding="utf-8")
            (resources / "provenance.json").write_text(json.dumps(provenance, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
            info = {
                "CFBundleInfoDictionaryVersion": "6.0", "CFBundleExecutable": launcher.name,
                "CFBundleIdentifier": "org.petari.smg-pc.movement-demo", "CFBundleName": APP_NAME,
                "CFBundleDisplayName": APP_NAME, "CFBundlePackageType": "APPL",
                "CFBundleShortVersionString": "0.1.0", "CFBundleVersion": "1",
                "LSMinimumSystemVersion": requirements["minimum_macos"],
                "NSPrincipalClass": "NSApplication", "NSHighResolutionCapable": True,
                "SMGPCBinarySHA256": expected_hash,
            }
            with (staged / "Contents/Info.plist").open("wb") as stream:
                plistlib.dump(info, stream, sort_keys=False)
            if sha256(copied_binary) != expected_hash or sha256(binary) != expected_hash:
                raise ValueError("executable changed during packaging; validate and package again")
            run("/usr/bin/codesign", "--verify", str(copied_binary))
            # Publication is a single rename after every package check succeeds.
            if output.exists() or output.is_symlink():
                raise ValueError(f"output appeared during packaging: {output}")
            staged.rename(output)
        print(output)
        print(f"README: {output / 'Contents/Resources/README.txt'}")
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"Packaging failed: {error}\n")


if __name__ == "__main__":
    main()
