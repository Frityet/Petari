#!/usr/bin/env python3

import argparse
import json
import sys
from pathlib import Path


TRACE_SCHEMA = "smgpc-trace-ndjson-v1"


def fail(path: Path, line_no: int | None, message: str) -> None:
    prefix = str(path)
    if line_no is not None:
        prefix = f"{prefix}:{line_no}"
    raise ValueError(f"{prefix}: {message}")


def require(condition: bool, path: Path, line_no: int | None, message: str) -> None:
    if not condition:
        fail(path, line_no, message)


def validate_trace(path: Path, args: argparse.Namespace) -> dict[str, int | str]:
    counts: dict[str, int] = {}
    frame_index: int | None = None
    emulator: str | None = None
    top_level_keys: set[str] = set()
    line_count = 0

    with path.open("r", encoding="utf-8") as file:
        for line_no, line in enumerate(file, start=1):
            text = line.strip()
            if not text:
                continue

            line_count += 1
            try:
                record = json.loads(text)
            except json.JSONDecodeError as exc:
                fail(path, line_no, f"invalid JSON: {exc.msg}")

            require(record.get("schema") == TRACE_SCHEMA, path, line_no, "unexpected trace schema")

            record_type = record.get("record_type")
            require(isinstance(record_type, str) and record_type, path, line_no, "missing record_type")
            counts[record_type] = counts.get(record_type, 0) + 1

            record_emulator = record.get("emulator")
            require(isinstance(record_emulator, str) and record_emulator, path, line_no, "missing emulator")
            if emulator is None:
                emulator = record_emulator
            require(record_emulator == emulator, path, line_no, "emulator changed within trace")

            record_frame = record.get("frame_index")
            require(isinstance(record_frame, int), path, line_no, "missing integer frame_index")
            if frame_index is None:
                frame_index = record_frame
            require(record_frame == frame_index, path, line_no, "frame_index changed within trace")

            payload = record.get("payload")
            require("payload" in record, path, line_no, "missing payload")

            if record_type == "trace_meta":
                require(isinstance(payload, dict), path, line_no, "trace_meta payload must be an object")
                require(payload.get("source_schema") == "smgpc-runtime-parity-trace-v1",
                        path, line_no, "unexpected source_schema")
                require(payload.get("requested_frame") == frame_index,
                        path, line_no, "trace_meta requested_frame mismatch")
            elif record_type == "frame":
                require(isinstance(payload, dict), path, line_no, "frame payload must be an object")
                require(payload.get("index") == frame_index, path, line_no, "frame payload index mismatch")
                require(isinstance(payload.get("framebuffer"), dict), path, line_no, "frame payload missing framebuffer")
            elif record_type == "top_level":
                key = record.get("key")
                require(isinstance(key, str) and key, path, line_no, "top_level record missing key")
                top_level_keys.add(key)
            elif record_type == "render_packet":
                require(isinstance(payload, dict), path, line_no, "render_packet payload must be an object")
                if "frame_index" in payload:
                    require(isinstance(payload.get("frame_index"), int),
                            path, line_no, "render_packet payload frame_index must be an integer")
                require(isinstance(payload.get("render_pass"), str), path, line_no, "render_packet missing render_pass")
            elif record_type == "semantic_event":
                require(isinstance(payload, dict), path, line_no, "semantic_event payload must be an object")
                require(isinstance(payload.get("category"), str), path, line_no, "semantic_event missing category")
                require(isinstance(payload.get("name"), str), path, line_no, "semantic_event missing name")

    require(line_count > 0, path, None, "empty trace")
    require(frame_index is not None, path, None, "missing trace frame")
    require(emulator is not None, path, None, "missing trace emulator")

    if args.require_emulator is not None:
        require(emulator == args.require_emulator, path, None, "trace emulator does not match requirement")
    if args.require_frame is not None:
        require(frame_index == args.require_frame, path, None, "trace frame does not match requirement")
    for record_type in args.require_record_type:
        require(counts.get(record_type, 0) > 0, path, None, f"missing required record_type {record_type}")
    for key in args.require_top_level:
        require(key in top_level_keys, path, None, f"missing required top_level key {key}")
    require(counts.get("render_packet", 0) >= args.min_render_packets,
            path, None, "render_packet count below minimum")
    if args.require_semantic_events:
        require(counts.get("semantic_event", 0) > 0, path, None, "missing semantic_event records")

    return {
        "path": str(path),
        "status": "passed",
        "frame_index": frame_index,
        "emulator": emulator,
        "lines": line_count,
        "render_packet": counts.get("render_packet", 0),
        "semantic_event": counts.get("semantic_event", 0),
        "top_level": counts.get("top_level", 0),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate SMG PC runtime parity NDJSON traces.")
    parser.add_argument("traces", nargs="+", type=Path)
    parser.add_argument("--require-emulator")
    parser.add_argument("--require-frame", type=int)
    parser.add_argument("--require-record-type", action="append", default=[])
    parser.add_argument("--require-top-level", action="append", default=[])
    parser.add_argument("--require-semantic-events", action="store_true")
    parser.add_argument("--min-render-packets", type=int, default=0)
    args = parser.parse_args()

    try:
        summaries = [validate_trace(path, args) for path in args.traces]
    except ValueError as exc:
        print(exc, file=sys.stderr)
        return 1

    print("trace\tstatus\tframe_index\temulator\tlines\trender_packet\tsemantic_event\ttop_level")
    for summary in summaries:
        print("{path}\t{status}\t{frame_index}\t{emulator}\t{lines}\t{render_packet}\t{semantic_event}\t{top_level}".format(**summary))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
