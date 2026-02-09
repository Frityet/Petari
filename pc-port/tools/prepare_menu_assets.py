#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, List


@dataclass(frozen=True)
class ArchiveRequest:
    output_name: str
    archive_name: str
    kind: str  # "layout" or "language_layout"


def build_candidates(game_root: Path, version: str, language: str, archive_name: str, kind: str) -> List[Path]:
    file_name = f"{archive_name}.arc"

    def base_layout(root: Path, language_layout: bool) -> Path:
        if language_layout:
            return root / language / "LayoutData"
        return root / "LayoutData"

    orig_files = game_root / "orig" / version / "files"
    dump_files = game_root / "dump" / "DATA" / "files"
    direct_files = game_root / "files"

    language_layout = kind == "language_layout"

    candidates = [
        base_layout(orig_files, language_layout) / file_name,
        base_layout(dump_files, language_layout) / file_name,
        base_layout(direct_files, language_layout) / file_name,
        base_layout(game_root, language_layout) / file_name,
    ]

    if language_layout:
        candidates.extend(
            [
                base_layout(orig_files, False) / file_name,
                base_layout(dump_files, False) / file_name,
                base_layout(direct_files, False) / file_name,
                base_layout(game_root, False) / file_name,
            ]
        )

    deduped: List[Path] = []
    seen = set()
    for path in candidates:
        key = str(path)
        if key in seen:
            continue
        seen.add(key)
        deduped.append(path)
    return deduped


def resolve_archive(game_root: Path, version: str, language: str, req: ArchiveRequest) -> Path:
    for candidate in build_candidates(game_root, version, language, req.archive_name, req.kind):
        if candidate.exists():
            return candidate
    raise FileNotFoundError(f"Archive not found for {req.archive_name}: tried {build_candidates(game_root, version, language, req.archive_name, req.kind)}")


def run(cmd: Iterable[str]) -> None:
    cmd_list = list(cmd)
    print("[assets]", " ".join(cmd_list))
    subprocess.run(cmd_list, check=True)


def main() -> int:
    parser = argparse.ArgumentParser(description="Prepare visual menu assets for pc-port")
    parser.add_argument("--game-root", type=Path, required=True)
    parser.add_argument("--version", default="RMGK01")
    parser.add_argument("--language", default="KrKorean")
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--force", action="store_true")
    args = parser.parse_args()

    game_root = args.game_root.resolve()
    out_root = args.out_dir.resolve() / args.version / args.language

    script_root = Path(__file__).resolve().parent
    repo_root = script_root.parent.parent
    dtk = repo_root / "build" / "tools" / "dtk"
    if not dtk.exists():
        dtk = Path("dtk")

    requests = [
        ArchiveRequest("PressStart", "PressStart", "layout"),
        ArchiveRequest("FileSelect", "FileSelect", "layout"),
        ArchiveRequest("TitleLogo", "TitleLogo", "language_layout"),
        ArchiveRequest("Font", "Font", "language_layout"),
    ]

    resolved = {}
    out_root.mkdir(parents=True, exist_ok=True)

    for req in requests:
        archive_path = resolve_archive(game_root, args.version, args.language, req)
        resolved[req.output_name] = str(archive_path)
        dst = out_root / req.output_name

        if args.force and dst.exists():
            shutil.rmtree(dst)

        if dst.exists() and any(dst.iterdir()):
            print(f"[assets] Reusing existing extraction: {dst}")
            continue

        dst.mkdir(parents=True, exist_ok=True)
        run([str(dtk), "u8", "extract", "-o", str(dst), str(archive_path)])

    manifest_path = out_root / "manifest.json"
    manifest = {
        "version": args.version,
        "language": args.language,
        "game_root": str(game_root),
        "archives": resolved,
    }
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"[assets] Wrote manifest: {manifest_path}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.CalledProcessError as exc:
        print(f"[assets] command failed: {exc}", file=sys.stderr)
        raise
