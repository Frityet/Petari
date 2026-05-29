#!/usr/bin/env python3

import argparse
import json
import os
import shutil
import signal
import struct
import subprocess
import sys
import time
import zlib
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


PROJECT_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_PC_BIN = PROJECT_ROOT / "build/linux/x86_64/debug/smg-pc"
TRACE_VALIDATOR = PROJECT_ROOT / "scripts/validate_trace_ndjson.py"
TRACE_SCHEMA = "smgpc-trace-ndjson-v1"

ROUTE_BUTTON_SCRIPT = (
    "120-1700:A+B;"
    "1950-1960:A;2130-2140:A;2400-2410:A;2600-2610:A;2800-2810:A;"
    "3000-3010:A;3200-3210:A;3400-3410:A;3600-3610:A;3800-3810:A;"
    "4000-4010:A;4200-4210:A;4400-4410:A;4600-4610:A;4800-4810:A;"
    "5000-5010:A;5400-5410:A"
)

ROUTE_POINTER_SCRIPT = (
    "0-1899:0,0,false;"
    "1900-2020:212.935,152.482,true;"
    "2021-2079:0,0,false;"
    "2080-2200:436,364,true;"
    "2201-2299:0,0,false;"
    "2300-3699:187,205,true;"
    "3700-5200:436,364,true;"
    "5201-7000:438,404,true;"
    "7001-7899:0,0,false"
)


@dataclass(frozen=True)
class Scenario:
    name: str
    frame: int
    description: str
    min_nonblack_ratio: float
    min_render_packets: int
    expected_layouts: tuple[str, ...]
    button_script: str = ROUTE_BUTTON_SCRIPT
    pointer_script: str = ROUTE_POINTER_SCRIPT


SCENARIOS: dict[str, Scenario] = {
    "title": Scenario(
        name="title",
        frame=90,
        description="title screen before scripted A+B input",
        min_nonblack_ratio=0.003,
        min_render_packets=1,
        expected_layouts=("TitleLogo",),
    ),
    "title_decide": Scenario(
        name="title_decide",
        frame=420,
        description="title screen after scripted A+B input has been accepted",
        min_nonblack_ratio=0.003,
        min_render_packets=1,
        expected_layouts=("TitleLogo",),
    ),
    "file_select": Scenario(
        name="file_select",
        frame=1900,
        description="first stable file-select camera after title A+B",
        min_nonblack_ratio=0.01,
        min_render_packets=1,
        expected_layouts=("FileNumber",),
    ),
    "file_confirm": Scenario(
        name="file_confirm",
        frame=7000,
        description="selected-file confirmation path after scripted pointer/A input",
        min_nonblack_ratio=0.01,
        min_render_packets=1,
        expected_layouts=("FileNumber",),
    ),
    "picturebook": Scenario(
        name="picturebook",
        frame=7600,
        description="prologue picturebook first-page route frame",
        min_nonblack_ratio=0.01,
        min_render_packets=1,
        expected_layouts=("PrologueDemo", "IconAButton"),
    ),
    "picturebook_wait": Scenario(
        name="picturebook_wait",
        frame=7900,
        description="prologue picturebook wait frame with A-button prompt",
        min_nonblack_ratio=0.01,
        min_render_packets=1,
        expected_layouts=("PrologueDemo", "IconAButton"),
    ),
}

DEFAULT_SCENARIOS = ("title", "file_select", "picturebook")


@dataclass
class RgbImage:
    width: int
    height: int
    pixels: bytes


def run_command(command: list[str], *, cwd: Path = PROJECT_ROOT, env: dict[str, str] | None = None,
                stdout: int | None = None, stderr: int | None = None) -> subprocess.CompletedProcess:
    print("+ " + " ".join(command), flush=True)
    return subprocess.run(command, cwd=cwd, env=env, check=True, stdout=stdout, stderr=stderr)


def require_tool(name: str) -> str:
    path = shutil.which(name)
    if path is None:
        raise RuntimeError(f"missing required tool `{name}`")
    return path


def disc_image_from_args(args: argparse.Namespace) -> Path:
    if args.disc:
        return Path(args.disc)
    env_disc = os.environ.get("SMGPC_DISC_IMAGE")
    if env_disc:
        return Path(env_disc)
    raise RuntimeError("missing disc image: pass --disc <path> or set SMGPC_DISC_IMAGE")


def pick_display() -> int:
    for display in range(130, 230):
        if not Path(f"/tmp/.X11-unix/X{display}").exists():
            return display
    raise RuntimeError("could not find a free X11 display number in :130..:229")


class XvfbServer:
    def __init__(self, width: int, height: int, display: int | None, log_path: Path):
        self.display = display if display is not None else pick_display()
        self.width = width
        self.height = height
        self.log_path = log_path
        self.process: subprocess.Popen | None = None

    def __enter__(self) -> "XvfbServer":
        require_tool("Xvfb")
        self.log_path.parent.mkdir(parents=True, exist_ok=True)
        log_file = self.log_path.open("wb")
        command = [
            "Xvfb",
            f":{self.display}",
            "-screen",
            "0",
            f"{self.width}x{self.height}x24",
            "-nolisten",
            "tcp",
        ]
        print("+ " + " ".join(command), flush=True)
        self.process = subprocess.Popen(command, stdout=log_file, stderr=subprocess.STDOUT)
        time.sleep(0.5)
        if self.process.poll() is not None:
            raise RuntimeError(f"Xvfb exited early, see {self.log_path}")
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        if self.process is None:
            return
        self.process.terminate()
        try:
            self.process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            self.process.kill()
            self.process.wait(timeout=5)


def choose_xwd_header(values_by_endian: Iterable[tuple[str, tuple[int, ...]]]) -> tuple[str, tuple[int, ...]]:
    candidates: list[tuple[str, tuple[int, ...]]] = []
    for endian, values in values_by_endian:
        header_size = values[0]
        version = values[1]
        width = values[4]
        height = values[5]
        bits_per_pixel = values[11]
        bytes_per_line = values[12]
        ncolors = values[19]
        plausible = (
            100 <= header_size <= 65536
            and version == 7
            and 1 <= width <= 16384
            and 1 <= height <= 16384
            and bits_per_pixel in (8, 16, 24, 32)
            and bytes_per_line >= width * max(bits_per_pixel // 8, 1)
            and ncolors < 1_000_000
        )
        if plausible:
            candidates.append((endian, values))
    if not candidates:
        raise RuntimeError("could not parse XWD header")
    return candidates[0]


def mask_shift_and_bits(mask: int) -> tuple[int, int]:
    if mask == 0:
        return 0, 0
    shift = 0
    while ((mask >> shift) & 1) == 0:
        shift += 1
    bits = 0
    while ((mask >> (shift + bits)) & 1) == 1:
        bits += 1
    return shift, bits


def scale_channel(value: int, bits: int) -> int:
    if bits <= 0:
        return 0
    if bits >= 8:
        return value >> (bits - 8)
    return (value * 255) // ((1 << bits) - 1)


def load_xwd_rgb(path: Path) -> RgbImage:
    data = path.read_bytes()
    if len(data) < 100:
        raise RuntimeError(f"{path} is too small to be an XWD file")

    endian, values = choose_xwd_header(
        (
            (">", struct.unpack(">25I", data[:100])),
            ("<", struct.unpack("<25I", data[:100])),
        )
    )
    del endian

    header_size = values[0]
    width = values[4]
    height = values[5]
    byte_order = values[7]
    bits_per_pixel = values[11]
    bytes_per_line = values[12]
    red_mask = values[14]
    green_mask = values[15]
    blue_mask = values[16]
    ncolors = values[19]
    pixel_offset = header_size + ncolors * 12
    bytes_per_pixel = max(bits_per_pixel // 8, 1)
    if len(data) < pixel_offset + bytes_per_line * height:
        raise RuntimeError(f"{path} is truncated")

    pixel_byteorder = "little" if byte_order == 0 else "big"
    red_shift, red_bits = mask_shift_and_bits(red_mask)
    green_shift, green_bits = mask_shift_and_bits(green_mask)
    blue_shift, blue_bits = mask_shift_and_bits(blue_mask)
    pixels = bytearray(width * height * 3)

    for y in range(height):
        row_offset = pixel_offset + y * bytes_per_line
        for x in range(width):
            src = row_offset + x * bytes_per_pixel
            raw = int.from_bytes(data[src:src + bytes_per_pixel], pixel_byteorder, signed=False)
            if red_mask != 0 and green_mask != 0 and blue_mask != 0:
                r = scale_channel((raw & red_mask) >> red_shift, red_bits)
                g = scale_channel((raw & green_mask) >> green_shift, green_bits)
                b = scale_channel((raw & blue_mask) >> blue_shift, blue_bits)
            elif bytes_per_pixel >= 3:
                b, g, r = data[src:src + 3]
            else:
                r = g = b = data[src]
            dst = (y * width + x) * 3
            pixels[dst:dst + 3] = bytes((r, g, b))

    return RgbImage(width=width, height=height, pixels=bytes(pixels))


def png_chunk(chunk_type: bytes, payload: bytes) -> bytes:
    return (
        struct.pack(">I", len(payload))
        + chunk_type
        + payload
        + struct.pack(">I", zlib.crc32(chunk_type + payload) & 0xFFFFFFFF)
    )


def write_png(path: Path, image: RgbImage) -> None:
    rows = bytearray()
    stride = image.width * 3
    for y in range(image.height):
        rows.append(0)
        begin = y * stride
        rows.extend(image.pixels[begin:begin + stride])

    ihdr = struct.pack(">IIBBBBB", image.width, image.height, 8, 2, 0, 0, 0)
    png = b"\x89PNG\r\n\x1a\n" + png_chunk(b"IHDR", ihdr) + png_chunk(b"IDAT", zlib.compress(bytes(rows), 6)) + png_chunk(b"IEND", b"")
    path.write_bytes(png)


def load_png_rgb(path: Path) -> RgbImage:
    data = path.read_bytes()
    signature = b"\x89PNG\r\n\x1a\n"
    if not data.startswith(signature):
        raise RuntimeError(f"{path} is not a PNG file")

    offset = len(signature)
    width = 0
    height = 0
    color_type = -1
    bit_depth = 0
    idat = bytearray()
    while offset + 8 <= len(data):
        chunk_length = struct.unpack(">I", data[offset:offset + 4])[0]
        chunk_type = data[offset + 4:offset + 8]
        chunk_begin = offset + 8
        chunk_end = chunk_begin + chunk_length
        if chunk_end + 4 > len(data):
            raise RuntimeError(f"{path} has a truncated PNG chunk")
        payload = data[chunk_begin:chunk_end]
        if chunk_type == b"IHDR":
            width, height, bit_depth, color_type, compression, filter_method, interlace = struct.unpack(">IIBBBBB", payload)
            if bit_depth != 8 or color_type not in (2, 6) or compression != 0 or filter_method != 0 or interlace != 0:
                raise RuntimeError(f"{path} uses an unsupported PNG format")
        elif chunk_type == b"IDAT":
            idat.extend(payload)
        elif chunk_type == b"IEND":
            break
        offset = chunk_end + 4

    channels = 4 if color_type == 6 else 3
    stride = width * channels
    rows = zlib.decompress(bytes(idat))
    expected = (stride + 1) * height
    if width == 0 or height == 0 or len(rows) < expected:
        raise RuntimeError(f"{path} has invalid PNG image data")

    pixels = bytearray(width * height * 3)
    for y in range(height):
        row_begin = y * (stride + 1)
        if rows[row_begin] != 0:
            raise RuntimeError(f"{path} uses unsupported PNG row filter {rows[row_begin]}")
        source = row_begin + 1
        if channels == 3:
            pixels[y * width * 3:(y + 1) * width * 3] = rows[source:source + stride]
            continue
        for x in range(width):
            src = source + x * 4
            dst = (y * width + x) * 3
            pixels[dst:dst + 3] = rows[src:src + 3]

    return RgbImage(width=width, height=height, pixels=bytes(pixels))


def image_stats(image: RgbImage) -> dict[str, float | int | list[float]]:
    pixels = image.pixels
    total = image.width * image.height
    nonblack = 0
    max_channel = 0
    sums = [0, 0, 0]
    sample_unique: set[bytes] = set()
    for index in range(0, len(pixels), 3):
        r = pixels[index]
        g = pixels[index + 1]
        b = pixels[index + 2]
        if r > 8 or g > 8 or b > 8:
            nonblack += 1
        max_channel = max(max_channel, r, g, b)
        sums[0] += r
        sums[1] += g
        sums[2] += b
        if len(sample_unique) < 4096:
            sample_unique.add(pixels[index:index + 3])
    return {
        "width": image.width,
        "height": image.height,
        "nonblack_pixels": nonblack,
        "nonblack_ratio": nonblack / total if total else 0.0,
        "max_channel": max_channel,
        "mean_rgb": [round(value / total, 3) if total else 0.0 for value in sums],
        "sample_unique_rgb": len(sample_unique),
    }


def summarize_trace(path: Path) -> dict[str, object]:
    counts: dict[str, int] = {}
    layout_names: set[str] = set()
    semantic_names: list[str] = []
    frame_index = None
    with path.open("r", encoding="utf-8") as trace_file:
        for line in trace_file:
            if not line.strip():
                continue
            record = json.loads(line)
            if record.get("schema") != TRACE_SCHEMA:
                continue
            record_type = record.get("record_type")
            if isinstance(record_type, str):
                counts[record_type] = counts.get(record_type, 0) + 1
            if isinstance(record.get("frame_index"), int):
                frame_index = record["frame_index"]
            payload = record.get("payload")
            if record_type == "render_packet" and isinstance(payload, dict):
                layout_name = payload.get("layout_name")
                if isinstance(layout_name, str) and layout_name:
                    layout_names.add(layout_name)
            if record_type == "semantic_event" and isinstance(payload, dict):
                category = payload.get("category", "")
                name = payload.get("name", "")
                if isinstance(category, str) and isinstance(name, str):
                    semantic_names.append(f"{category}:{name}")
    return {
        "frame_index": frame_index,
        "record_counts": counts,
        "layout_names": sorted(layout_names),
        "semantic_names": semantic_names[-20:],
    }


def validate_trace(trace_path: Path, scenario: Scenario, log_path: Path) -> None:
    command = [
        sys.executable,
        str(TRACE_VALIDATOR),
        str(trace_path),
        "--require-emulator",
        "pc-port",
        "--require-frame",
        str(scenario.frame),
        "--require-record-type",
        "frame",
        "--require-record-type",
        "render_packet",
        "--require-record-type",
        "semantic_event",
        "--require-semantic-events",
        "--min-render-packets",
        str(scenario.min_render_packets),
    ]
    with log_path.open("w", encoding="utf-8") as output:
        run_command(command, stdout=output, stderr=subprocess.STDOUT)


def terminate_process(process: subprocess.Popen) -> None:
    if process.poll() is not None:
        return
    process.terminate()
    try:
        process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait(timeout=5)


def run_scenario(args: argparse.Namespace, scenario: Scenario, display: str, disc_image: Path, pc_bin: Path) -> dict[str, object]:
    scenario_dir = args.work_dir / scenario.name
    scenario_dir.mkdir(parents=True, exist_ok=True)
    save_dir = scenario_dir / "save"
    if args.reset_save and save_dir.exists():
        shutil.rmtree(save_dir)
    save_dir.mkdir(parents=True, exist_ok=True)

    trace_path = scenario_dir / f"{scenario.name}-frame-{scenario.frame}.trace.ndjson"
    xwd_path = scenario_dir / f"{scenario.name}-frame-{scenario.frame}.xwd"
    png_path = scenario_dir / f"{scenario.name}-frame-{scenario.frame}.png"
    app_log = scenario_dir / f"{scenario.name}-app.log"
    trace_log = scenario_dir / f"{scenario.name}-trace-validator.log"
    manifest_path = scenario_dir / "manifest.json"
    for path in (trace_path, xwd_path, png_path, app_log, trace_log):
        path.unlink(missing_ok=True)

    env = os.environ.copy()
    env.update(
        {
            "DISPLAY": display,
            "SMGPC_WINDOW_WIDTH": str(args.width),
            "SMGPC_WINDOW_HEIGHT": str(args.height),
            "SMGPC_ENABLE_VSYNC": "0",
            "SMGPC_FRAME_PACING": "1" if args.frame_pacing else "0",
            "SMGPC_SAVE_DIR": str(save_dir),
            "SMGPC_PARITY_TRACE_PATH": str(trace_path),
            "SMGPC_PARITY_TRACE_FRAME": str(scenario.frame),
            "SMGPC_SCREENSHOT_PATH": str(png_path),
            "SMGPC_SCREENSHOT_FRAME": str(scenario.frame),
            "SMGPC_EXIT_AFTER_SCREENSHOT": "1",
            "SMGPC_EXIT_AFTER_FRAME": str(scenario.frame + max(args.exit_margin_frames, 1)),
            "SMGPC_SKIP_RENDER_UNTIL_FRAME": str(max(1, scenario.frame - max(args.draw_warmup_frames, 0))),
            "SMGPC_DEBUG_HOLD_AFTER_TRACE_MS": str(args.hold_ms),
            "SMGPC_DEBUG_WPAD_BUTTON_SCRIPT": scenario.button_script,
            "SMGPC_DEBUG_WPAD_POINTER_SCRIPT": scenario.pointer_script,
            "SMGPC_SEMANTIC_ANCHOR_CATEGORY": "aurora_route_smoke",
            "SMGPC_SEMANTIC_ANCHOR_NAME": scenario.name,
            "SMGPC_SEMANTIC_ANCHOR_DETAIL": scenario.description,
        }
    )
    if args.disc is None:
        env["SMGPC_DISC_IMAGE"] = str(disc_image)

    command = [str(pc_bin)]
    if args.disc is not None:
        command += ["--disc", str(disc_image)]

    print(f"aurora-route-smoke: {scenario.name} frame={scenario.frame}", flush=True)
    with app_log.open("wb") as log_file:
        process = None
        for attempt in range(5):
            try:
                process = subprocess.Popen(command, cwd=PROJECT_ROOT, env=env, stdout=log_file, stderr=subprocess.STDOUT)
                break
            except PermissionError:
                if attempt == 4:
                    raise
                time.sleep(0.25)
        if process is None:
            raise RuntimeError("failed to start smg-pc")

    deadline = time.monotonic() + args.timeout
    try:
        while time.monotonic() < deadline:
            if trace_path.exists() and trace_path.stat().st_size > 0 and png_path.exists() and png_path.stat().st_size > 0:
                break
            if process.poll() is not None:
                missing = [str(path) for path in (trace_path, png_path) if not path.exists() or path.stat().st_size == 0]
                if missing:
                    raise RuntimeError(f"smg-pc exited before writing {', '.join(missing)}; see {app_log}")
                break
            time.sleep(0.025)
        else:
            raise RuntimeError(f"timed out waiting for {trace_path} and {png_path}; see {app_log}")
    finally:
        terminate_process(process)

    image = load_png_rgb(png_path)
    stats = image_stats(image)
    expected_heights = {args.height}
    if args.width == 640 and args.height in (456, 480):
        expected_heights.update((456, 480))
    if image.width != args.width or image.height not in expected_heights:
        expected = " or ".join(f"{args.width}x{height}" for height in sorted(expected_heights))
        raise RuntimeError(f"{png_path} captured {image.width}x{image.height}, expected {expected}")
    if stats["nonblack_ratio"] < scenario.min_nonblack_ratio:
        raise RuntimeError(
            f"{png_path} looks blank: nonblack_ratio={stats['nonblack_ratio']:.5f}, "
            f"required={scenario.min_nonblack_ratio:.5f}"
        )
    if stats["max_channel"] <= 8:
        raise RuntimeError(f"{png_path} has no visible color range")

    validate_trace(trace_path, scenario, trace_log)
    trace_summary = summarize_trace(trace_path)
    layout_names = set(trace_summary["layout_names"])
    missing_layouts = [layout for layout in scenario.expected_layouts if layout not in layout_names]
    if missing_layouts:
        raise RuntimeError(f"{trace_path} is missing expected layout packet(s): {', '.join(missing_layouts)}")
    manifest = {
        "scenario": scenario.name,
        "description": scenario.description,
        "status": "passed",
        "frame": scenario.frame,
        "expected_layouts": list(scenario.expected_layouts),
        "artifacts": {
            "png": str(png_path),
            "trace": str(trace_path),
            "app_log": str(app_log),
            "trace_validator_log": str(trace_log),
        },
        "image_stats": stats,
        "trace_summary": trace_summary,
        "input": {
            "button_script": scenario.button_script,
            "pointer_script": scenario.pointer_script,
        },
    }
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(
        f"aurora-route-smoke: {scenario.name} passed "
        f"nonblack={stats['nonblack_ratio']:.4f} render_packets={trace_summary['record_counts'].get('render_packet', 0)} "
        f"png={png_path}",
        flush=True,
    )
    return manifest


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Drive and capture Aurora-native SMG title/file-select/picturebook route frames.")
    parser.add_argument("scenarios", nargs="*", help="scenario names to run")
    parser.add_argument("--list-scenarios", action="store_true")
    parser.add_argument("--disc", help="SMG RVZ/WBFS/ISO path; defaults to SMGPC_DISC_IMAGE")
    parser.add_argument("--pc-bin", type=Path, default=DEFAULT_PC_BIN)
    parser.add_argument("--work-dir", type=Path, default=PROJECT_ROOT / ".cache/aurora-route-smoke/latest")
    parser.add_argument("--width", type=int, default=640)
    parser.add_argument("--height", type=int, default=480)
    parser.add_argument("--timeout", type=float, default=180.0)
    parser.add_argument("--hold-ms", type=int, default=1000, help="debug hold after trace write before the app continues")
    parser.add_argument("--capture-delay", type=float, default=0.05)
    parser.add_argument("--exit-margin-frames", type=int, default=60)
    parser.add_argument("--draw-warmup-frames", type=int, default=2, help="number of frames to draw before the trace/capture frame")
    parser.add_argument("--display", type=int, help="X11 display number to use for Xvfb")
    parser.add_argument("--frame-pacing", action="store_true", help="run at 60 Hz instead of fast debug route mode")
    parser.add_argument("--reset-save", dest="reset_save", action="store_true", default=True)
    parser.add_argument("--keep-save", dest="reset_save", action="store_false")
    parser.add_argument("--build", dest="build", action="store_true", default=True)
    parser.add_argument("--no-build", dest="build", action="store_false")
    parser.add_argument("--keep-going", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.list_scenarios:
        for scenario in SCENARIOS.values():
            print(f"{scenario.name}\tframe={scenario.frame}\t{scenario.description}")
        return 0

    scenario_names = args.scenarios or list(DEFAULT_SCENARIOS)
    unknown = [name for name in scenario_names if name not in SCENARIOS]
    if unknown:
        print(f"unknown scenario(s): {', '.join(unknown)}", file=sys.stderr)
        print("known scenarios: " + " ".join(SCENARIOS), file=sys.stderr)
        return 2

    try:
        require_tool("xwd")
        disc_image = disc_image_from_args(args)
        if args.build:
            run_command(["xmake", "build", "smg-pc"])
        pc_bin = args.pc_bin
        if not pc_bin.is_file():
            raise RuntimeError(f"missing smg-pc binary: {pc_bin}")

        args.work_dir.mkdir(parents=True, exist_ok=True)
        manifests = []
        failures = []
        for name in scenario_names:
            try:
                with XvfbServer(args.width, args.height, args.display, args.work_dir / name / "xvfb.log") as xvfb:
                    display = f":{xvfb.display}"
                    manifests.append(run_scenario(args, SCENARIOS[name], display, disc_image, pc_bin))
            except Exception as exc:
                failures.append({"scenario": name, "error": str(exc)})
                print(f"aurora-route-smoke: {name} failed: {exc}", file=sys.stderr, flush=True)
                if not args.keep_going:
                    raise

        aggregate = {
            "status": "failed" if failures else "passed",
            "scenarios": manifests,
            "failures": failures,
        }
        aggregate_path = args.work_dir / "manifest.json"
        aggregate_path.write_text(json.dumps(aggregate, indent=2) + "\n", encoding="utf-8")
        print(f"aurora-route-smoke: {aggregate['status']} manifest={aggregate_path}")
        return 1 if failures else 0
    except Exception as exc:
        print(f"aurora-route-smoke: failed: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
