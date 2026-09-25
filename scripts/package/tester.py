#!/usr/bin/env python3
"""Package an existing native tester executable without disc assets or saves."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import plistlib
import re
import shutil
import subprocess
import tarfile
import zipfile


def output(*args):
    return subprocess.check_output(args, text=True)


def sha256(path):
    return hashlib.file_digest(path.open("rb"), "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--platform", choices=("macos-arm64", "linux-x86_64"), required=True)
    parser.add_argument("--output", type=Path, default=Path("dist"))
    parser.add_argument("--licenses", type=Path, required=True)
    parser.add_argument("--revision", required=True)
    parser.add_argument("--aurora-revision", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    binary = args.binary.resolve()
    macos = args.platform == "macos-arm64"
    description = output("file", str(binary)).strip()
    expected = "Mach-O 64-bit executable arm64" if macos else "ELF 64-bit LSB pie executable, x86-64"
    if expected not in description and not (not macos and "ELF 64-bit LSB executable, x86-64" in description):
        raise SystemExit(f"Unexpected executable architecture: {description}")
    name = f"petari-{args.platform}-{args.revision[:8]}"
    package = args.output.resolve() / name
    package.mkdir(parents=True, exist_ok=False)
    manifest = {"source_revision": args.revision, "aurora_revision": args.aurora_revision,
                "platform": args.platform, "build_mode": "optimized debug",
                "disc_assets_included": False, "gameplay_validation": "See README.txt",
                "source_binary_sha256": sha256(binary), "file": description}
    if macos:
        commands = output("otool", "-l", str(binary))
        minimum = re.search(r"cmd LC_BUILD_VERSION.*?\n\s*minos (\S+)", commands, re.S).group(1)
        dependencies = output("otool", "-L", str(binary))
        libraries = re.findall(r"^\s*(\S+) \(compatibility", dependencies, re.M)
        external = [lib for lib in libraries if not lib.startswith(("/System/Library/", "/usr/lib/"))]
        if external:
            raise SystemExit(f"Non-system dynamic dependencies: {external}")
        manifest.update(minimum_macos=minimum, dynamic_libraries=libraries, notarized=False)
        app = package / "Petari.app"
        bindir = app / "Contents/MacOS"
        bindir.mkdir(parents=True)
        info = {"CFBundleExecutable": "launch-game", "CFBundleIdentifier": "org.petari.smg-pc.tester",
                "CFBundleName": "Petari", "CFBundlePackageType": "APPL", "CFBundleVersion": "1",
                "CFBundleShortVersionString": "0.1.0", "LSMinimumSystemVersion": minimum,
                "NSPrincipalClass": "NSApplication", "NSHighResolutionCapable": True}
        (app / "Contents/Info.plist").write_bytes(plistlib.dumps(info))
        launcher = bindir / "launch-game"
        package_dir = '$(CDPATH= cd -- "$tester_bin_dir/../../.." && pwd)'
        requirements = f"Apple Silicon Mac, macOS {minimum} or newer. No Homebrew installation needed."
        launch_help = "Open Petari.app and choose your disc image.\nTerminal: Petari.app/Contents/MacOS/launch-game /path/to/game.rvz"
    else:
        dependencies = output("readelf", "-d", str(binary))
        libraries = re.findall(r"\(NEEDED\).*?\[(.*?)\]", dependencies)
        allowed = {"libc.so.6", "libm.so.6", "libdl.so.2", "libpthread.so.0", "librt.so.1",
                   "libgcc_s.so.1", "ld-linux-x86-64.so.2"}
        external = sorted(set(libraries) - allowed)
        if external:
            raise SystemExit(f"Non-system dynamic dependencies: {external}")
        versions = output("readelf", "--version-info", str(binary))
        glibc = max(set(re.findall(r"GLIBC_([0-9.]+)", versions)), key=lambda v: tuple(map(int, v.split("."))))
        manifest.update(minimum_glibc=glibc, dynamic_libraries=libraries)
        bindir = package
        launcher = package / "play.sh"
        package_dir = '"$tester_bin_dir"'
        requirements = f"x86_64 Linux with glibc {glibc} or newer, an X11 session (or XWayland), and a Vulkan-capable GPU driver."
        launch_help = "Run: ./play.sh /path/to/game.rvz\nOr place one disc image beside play.sh and run ./play.sh."
    copied = bindir / "smg-pc"
    shutil.copy2(binary, copied)
    copied.chmod(0o755)
    template = (root / "scripts/package/play.sh.in").read_text()
    launcher.write_text(template.replace("@PACKAGE_DIR@", package_dir).replace("@BINARY@", "smg-pc"))
    launcher.chmod(0o755)
    subprocess.run(["sh", "-n", str(launcher)], check=True)
    shutil.copytree(args.licenses, package / "licenses")
    (package / "README.txt").write_text(f"""Petari — Super Mario Galaxy PC tester build

{requirements}

{launch_help}
Supply your own Super Mario Galaxy RVZ, ISO or WBFS. No disc data is included.
Normal game startup is the default. For Gateway: append --stage HeavensDoorGalaxy --scenario 1.

Controls: WASD move, Space/Enter jump or confirm, X spin, Z crouch,
C reset camera, arrow keys camera/D-pad, B/Backspace cancel, +/- pause/menu.
Mouse moves the star pointer; left click confirms, right click uses B.

Logs: macOS ~/Library/Logs/Petari/; Linux $XDG_STATE_HOME/petari
(default ~/.local/state/petari). Saves persist in the game's per-user SDL
preference directory. SMGPC_SAVE_DIR can select a separate save directory.
Send the game log, OS/GPU details, galaxy/scenario and reproduction steps with bugs.

Application libraries and the C++ runtime are static. OS libraries and graphics
drivers remain system-provided. This optimized debug build retains crash checks.
The Mac app is ad-hoc signed, not notarized; macOS may require Open Anyway in
System Settings > Privacy & Security after the first launch attempt.

Current gameplay evidence: the Mac build before the latest FIFO fix reached
Gateway's Grand Star, then crashed during scene teardown. The included fix passed
a focused FIFO regression; the post-star transition and full-game completion
remain unverified. Audio output is not implemented. Packaging checks establish
architecture, linkage and launchability, not complete gameplay compatibility.

Source: {args.revision}
Aurora: {args.aurora_revision}
""")
    if macos:
        subprocess.run(["codesign", "--force", "--sign", "-", str(app)], check=True)
        subprocess.run(["codesign", "--verify", "--deep", "--strict", str(app)], check=True)
    manifest["packaged_binary_sha256"] = sha256(copied)
    (package / "build.json").write_text(json.dumps(manifest, indent=2) + "\n")
    checksums = [f"{sha256(p)}  {p.relative_to(package)}" for p in sorted(package.rglob("*")) if p.is_file()]
    (package / "SHA256SUMS").write_text("\n".join(checksums) + "\n")
    if macos:
        archive = package.with_suffix(".zip")
        with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as z:
            for p in sorted(package.rglob("*")):
                z.write(p, p.relative_to(package.parent))
    else:
        archive = package.with_suffix(".tar.gz")
        with tarfile.open(archive, "w:gz", compresslevel=6) as tar:
            tar.add(package, arcname=package.name)
    archive.with_name(archive.name + ".sha256").write_text(f"{sha256(archive)}  {archive.name}\n")
    print(json.dumps({"archive": str(archive), "sha256": sha256(archive), "bytes": archive.stat().st_size, **manifest}, indent=2))


if __name__ == "__main__":
    main()
