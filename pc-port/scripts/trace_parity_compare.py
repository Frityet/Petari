#!/usr/bin/env python3
"""Compare SMG PC-port and Dolphin parity traces.

This is intentionally a report tool, not a renderer oracle. Dolphin currently
emits raw backend draw events while the PC port emits higher-level runtime/J3D
packets, so the useful output is the first explainable state difference with
enough phase/draw/material context for the next debugging pass.
"""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any


@dataclass(frozen=True)
class Finding:
    severity: str
    path: str
    message: str
    dolphin: Any = None
    pc: Any = None
    context: str = ""


@dataclass(frozen=True)
class PacketSignature:
    texgen_count: Any
    color_channel_count: Any
    tev_stage_count: Any
    indirect_stage_count: Any
    cull_mode: Any
    vertex_count: Any
    textures: tuple[tuple[int, str, Any, Any], ...]
    tev_orders: tuple[tuple[Any, Any, Any, Any, bool], ...]
    requested_light_mask: Any


def load_trace(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as trace_file:
        trace = json.load(trace_file)
    if not isinstance(trace, dict):
        raise ValueError(f"{path} did not contain a JSON object")
    return trace


def at(trace: dict[str, Any], path: str, default: Any = None) -> Any:
    value: Any = trace
    for part in path.split("."):
        if not isinstance(value, dict) or part not in value:
            return default
        value = value[part]
    return value


def copy_event_kind_sequence(trace: dict[str, Any]) -> list[str]:
    events = trace.get("copy_events", [])
    if not isinstance(events, list):
        return []
    return [str(event.get("kind", "<missing>")) for event in events if isinstance(event, dict)]


def packet_context(packet: dict[str, Any], emulator: str, index: int) -> str:
    render_pass = packet.get("render_pass", packet.get("draw_pass", "<missing>"))
    model = packet.get("model_name")
    material = packet.get("material_name")
    if emulator == "dolphin":
        return (
            f"dolphin[{index}] draw={packet.get('draw_index', '<missing>')} "
            f"pass={render_pass} primitive={packet.get('primitive_type', '<missing>')}"
        )
    return (
        f"pc[{index}] model={model or '<none>'} material={material or '<none>'} "
        f"pass={render_pass} mode={packet.get('packet_mode', '<missing>')}"
    )


def packet_tev_stage_count(packet: dict[str, Any]) -> Any:
    if "active_tev_stage_count" in packet:
        return packet["active_tev_stage_count"]
    if "declared_tev_stage_count" in packet:
        return packet["declared_tev_stage_count"]
    if "tev_stage_count" in packet:
        return packet["tev_stage_count"]
    gen_mode = packet.get("gen_mode")
    if isinstance(gen_mode, dict):
        return gen_mode.get("tev_stage_count")
    return None


def packet_color_channel_count(packet: dict[str, Any]) -> Any:
    if "color_channel_count" in packet:
        return packet["color_channel_count"]
    gen_mode = packet.get("gen_mode")
    if isinstance(gen_mode, dict):
        return gen_mode.get("color_channel_count")
    color_channels = packet.get("color_channels")
    if isinstance(color_channels, list):
        return len(color_channels)
    return None


def packet_texgen_count(packet: dict[str, Any]) -> Any:
    if "texgen_count" in packet:
        return packet["texgen_count"]
    gen_mode = packet.get("gen_mode")
    if isinstance(gen_mode, dict):
        return gen_mode.get("texgen_count")
    return None


def packet_cull_mode(packet: dict[str, Any]) -> Any:
    gen_mode = packet.get("gen_mode")
    if isinstance(gen_mode, dict) and "cull_mode" in gen_mode:
        return gen_mode.get("cull_mode")

    cull_mode = packet.get("cull_mode")
    if isinstance(cull_mode, str):
        return {
            "None": 0,
            "Back": 1,
            "Front": 2,
            "FrontAndBack": 3,
        }.get(cull_mode, cull_mode)
    return cull_mode


def packet_indirect_stage_count(packet: dict[str, Any]) -> Any:
    if "indirect_stage_count" in packet:
        return packet["indirect_stage_count"]
    gen_mode = packet.get("gen_mode")
    if isinstance(gen_mode, dict):
        return gen_mode.get("indirect_stage_count")
    return None


def packet_vertex_count(packet: dict[str, Any]) -> Any:
    if "source_vertex_count" in packet:
        return packet["source_vertex_count"]
    if "num_vertices" in packet:
        return packet["num_vertices"]
    if "vertex_count" in packet:
        return packet["vertex_count"]
    return None


def packet_texture_count(packet: dict[str, Any]) -> int:
    bindings = packet.get("texture_bindings")
    if isinstance(bindings, list):
        return len(bindings)
    slots = packet.get("used_texture_slots")
    if isinstance(slots, list):
        return len(slots)
    return 0


def packet_texture_slots(packet: dict[str, Any]) -> list[int]:
    slots = packet.get("used_texture_slots")
    if isinstance(slots, list):
        return sorted(int(slot) for slot in slots if isinstance(slot, int))

    bindings = packet.get("texture_bindings")
    if not isinstance(bindings, list):
        return []
    out: list[int] = []
    for binding in bindings:
        if isinstance(binding, dict) and isinstance(binding.get("slot"), int):
            out.append(int(binding["slot"]))
    return sorted(set(out))


def packet_texture_formats(packet: dict[str, Any]) -> list[str]:
    bindings = packet.get("texture_bindings")
    if not isinstance(bindings, list):
        return []
    formats: list[str] = []
    for binding in bindings:
        if isinstance(binding, dict):
            texture_format = binding.get("format")
            if texture_format is not None:
                formats.append(str(texture_format))
    return formats


def packet_texture_dimensions(packet: dict[str, Any]) -> list[tuple[Any, Any]]:
    bindings = packet.get("texture_bindings")
    if not isinstance(bindings, list):
        return []
    dimensions: list[tuple[Any, Any]] = []
    for binding in bindings:
        if isinstance(binding, dict):
            dimensions.append((binding.get("width"), binding.get("height")))
    return dimensions


def packet_texture_signature(packet: dict[str, Any]) -> tuple[tuple[int, str, Any, Any], ...]:
    bindings = packet.get("texture_bindings")
    if not isinstance(bindings, list):
        return tuple((slot, "", None, None) for slot in packet_texture_slots(packet))

    textures: list[tuple[int, str, Any, Any]] = []
    for binding in bindings:
        if not isinstance(binding, dict) or not isinstance(binding.get("slot"), int):
            continue
        textures.append((int(binding["slot"]), str(binding.get("format", "")), binding.get("width"), binding.get("height")))
    return tuple(sorted(textures))


def packet_light_indices(packet: dict[str, Any]) -> list[int]:
    lights = packet.get("lights")
    if not isinstance(lights, list):
        return []
    out: list[int] = []
    for light in lights:
        if isinstance(light, int):
            out.append(light)
        elif isinstance(light, dict) and isinstance(light.get("index"), int):
            out.append(int(light["index"]))
    return sorted(set(out))


def packet_requested_light_mask(packet: dict[str, Any]) -> Any:
    if "requested_light_mask" in packet:
        return packet["requested_light_mask"]
    return packet.get("loaded_light_mask")


def packet_tev_order_signature(packet: dict[str, Any]) -> tuple[tuple[Any, Any, Any, Any, bool], ...]:
    orders = packet.get("tev_orders")
    if not isinstance(orders, list):
        return tuple()

    active_stage_count = packet_tev_stage_count(packet)
    signature: list[tuple[Any, Any, Any, Any, bool]] = []
    for order in orders:
        if not isinstance(order, dict):
            continue
        stage = order.get("stage")
        if isinstance(active_stage_count, int) and isinstance(stage, int) and stage >= active_stage_count:
            continue

        tex_map = order.get("tex_map")
        tex_coord = order.get("tex_coord")
        texture_enabled = order.get("texture_enabled")
        if texture_enabled is None:
            texture_enabled = tex_map != 0xFF
        texture_enabled = bool(texture_enabled)
        if not texture_enabled:
            tex_coord = 0xFF
            tex_map = 0xFF

        signature.append((stage, tex_coord, tex_map, order.get("color_channel"), texture_enabled))

    return tuple(signature)


def packet_signature(packet: dict[str, Any]) -> PacketSignature:
    return PacketSignature(
        texgen_count=packet_texgen_count(packet),
        color_channel_count=packet_color_channel_count(packet),
        tev_stage_count=packet_tev_stage_count(packet),
        indirect_stage_count=packet_indirect_stage_count(packet),
        cull_mode=packet_cull_mode(packet),
        vertex_count=packet_vertex_count(packet),
        textures=packet_texture_signature(packet),
        tev_orders=packet_tev_order_signature(packet),
        requested_light_mask=packet_requested_light_mask(packet),
    )


def signature_summary(signature: PacketSignature) -> str:
    return (
        f"texgens={signature.texgen_count} colors={signature.color_channel_count} tev={signature.tev_stage_count} "
        f"indirect={signature.indirect_stage_count} cull={signature.cull_mode} vertices={signature.vertex_count} "
        f"textures={list(signature.textures)} "
        f"tev_orders={list(signature.tev_orders)} "
        f"requested_lights={signature.requested_light_mask}"
    )


def packet_summary(packet: dict[str, Any], emulator: str, index: int) -> str:
    return (
        f"{packet_context(packet, emulator, index)} "
        f"texgens={packet_texgen_count(packet)} colors={packet_color_channel_count(packet)} "
        f"tev={packet_tev_stage_count(packet)} indirect={packet_indirect_stage_count(packet)} "
        f"cull={packet_cull_mode(packet)} vertices={packet_vertex_count(packet)} "
        f"textures={packet_texture_count(packet)} slots={packet_texture_slots(packet)} "
        f"formats={packet_texture_formats(packet)} dims={packet_texture_dimensions(packet)} "
        f"tev_orders={list(packet_tev_order_signature(packet))} "
        f"requested_lights={packet_requested_light_mask(packet)} "
        f"loaded_lights={packet.get('loaded_light_mask', '<missing>')} "
        f"light_indices={packet_light_indices(packet)}"
    )


def compare_scalar(findings: list[Finding], path: str, message: str, dolphin: Any, pc: Any) -> None:
    if dolphin != pc:
        findings.append(Finding("mismatch", path, message, dolphin, pc))


def compare_frame(findings: list[Finding], dolphin: dict[str, Any], pc: dict[str, Any]) -> None:
    compare_scalar(findings, "frame.index", "frame index differs", at(dolphin, "frame.index"), at(pc, "frame.index"))
    compare_scalar(
        findings,
        "frame.framebuffer.width",
        "framebuffer width differs",
        at(dolphin, "frame.framebuffer.width"),
        at(pc, "frame.framebuffer.width"),
    )
    compare_scalar(
        findings,
        "frame.framebuffer.height",
        "framebuffer height differs",
        at(dolphin, "frame.framebuffer.height"),
        at(pc, "frame.framebuffer.height"),
    )


def compare_copy_events(findings: list[Finding], dolphin: dict[str, Any], pc: dict[str, Any]) -> None:
    dolphin_events = dolphin.get("copy_events", [])
    pc_events = pc.get("copy_events", [])
    if not isinstance(dolphin_events, list):
        dolphin_events = []
    if not isinstance(pc_events, list):
        pc_events = []
    if len(dolphin_events) != len(pc_events):
        findings.append(
            Finding(
                "mismatch",
                "copy_events",
                "copy event count differs",
                len(dolphin_events),
                len(pc_events),
                f"dolphin kinds={copy_event_kind_sequence(dolphin)} pc kinds={copy_event_kind_sequence(pc)}",
            )
        )

    for index, (dolphin_event, pc_event) in enumerate(zip(dolphin_events, pc_events)):
        if not isinstance(dolphin_event, dict) or not isinstance(pc_event, dict):
            continue
        for field in ("kind", "source_rect.width", "source_rect.height", "output_size.width", "output_size.height"):
            compare_scalar(
                findings,
                f"copy_events[{index}].{field}",
                f"copy event {index} {field} differs",
                at(dolphin_event, field),
                at(pc_event, field),
            )


def compare_render_packets(findings: list[Finding], dolphin: dict[str, Any], pc: dict[str, Any], max_packet_comparisons: int) -> list[str]:
    dolphin_packets = dolphin.get("render_packets", [])
    pc_packets = pc.get("render_packets", [])
    if not isinstance(dolphin_packets, list):
        dolphin_packets = []
    if not isinstance(pc_packets, list):
        pc_packets = []
    if len(dolphin_packets) != len(pc_packets):
        findings.append(Finding("mismatch", "render_packets", "render packet count differs", len(dolphin_packets), len(pc_packets)))

    summaries: list[str] = []
    for index, packet in enumerate(dolphin_packets[:max_packet_comparisons]):
        if isinstance(packet, dict):
            summaries.append(packet_summary(packet, "dolphin", index))
    for index, packet in enumerate(pc_packets[:max_packet_comparisons]):
        if isinstance(packet, dict):
            summaries.append(packet_summary(packet, "pc", index))

    for index, (dolphin_packet, pc_packet) in enumerate(zip(dolphin_packets, pc_packets)):
        if index >= max_packet_comparisons:
            break
        if not isinstance(dolphin_packet, dict) or not isinstance(pc_packet, dict):
            continue
        context = f"{packet_context(dolphin_packet, 'dolphin', index)} | {packet_context(pc_packet, 'pc', index)}"
        comparisons = [
            ("render_pass", dolphin_packet.get("render_pass"), pc_packet.get("render_pass")),
            ("view_id", dolphin_packet.get("view_id"), pc_packet.get("view_id")),
            ("color_channel_count", packet_color_channel_count(dolphin_packet), packet_color_channel_count(pc_packet)),
            ("tev_stage_count", packet_tev_stage_count(dolphin_packet), packet_tev_stage_count(pc_packet)),
            ("texgen_count", packet_texgen_count(dolphin_packet), packet_texgen_count(pc_packet)),
            ("indirect_stage_count", packet_indirect_stage_count(dolphin_packet), packet_indirect_stage_count(pc_packet)),
            ("cull_mode", packet_cull_mode(dolphin_packet), packet_cull_mode(pc_packet)),
            ("vertex_count", packet_vertex_count(dolphin_packet), packet_vertex_count(pc_packet)),
            ("used_texture_slots", packet_texture_slots(dolphin_packet), packet_texture_slots(pc_packet)),
            ("tev_orders", packet_tev_order_signature(dolphin_packet), packet_tev_order_signature(pc_packet)),
            ("requested_light_mask", packet_requested_light_mask(dolphin_packet), packet_requested_light_mask(pc_packet)),
            ("light_indices", packet_light_indices(dolphin_packet), packet_light_indices(pc_packet)),
        ]
        for field, dolphin_value, pc_value in comparisons:
            if dolphin_value != pc_value:
                findings.append(
                    Finding(
                        "mismatch",
                        f"render_packets[{index}].{field}",
                        f"first comparable render packet {field} differs",
                        dolphin_value,
                        pc_value,
                        context,
                    )
                )
                return summaries

    return summaries


def correlate_render_packets(dolphin: dict[str, Any], pc: dict[str, Any], max_correlations: int, max_candidates: int) -> list[str]:
    dolphin_packets = dolphin.get("render_packets", [])
    pc_packets = pc.get("render_packets", [])
    if not isinstance(dolphin_packets, list) or not isinstance(pc_packets, list):
        return []

    candidates_by_signature: dict[PacketSignature, list[tuple[int, dict[str, Any]]]] = {}
    for index, packet in enumerate(dolphin_packets):
        if not isinstance(packet, dict):
            continue
        candidates_by_signature.setdefault(packet_signature(packet), []).append((index, packet))

    lines: list[str] = []
    for pc_index, pc_packet in enumerate(pc_packets[:max_correlations]):
        if not isinstance(pc_packet, dict):
            continue
        signature = packet_signature(pc_packet)
        candidates = candidates_by_signature.get(signature, [])
        prefix = f"{packet_context(pc_packet, 'pc', pc_index)} key=({signature_summary(signature)})"
        if not candidates:
            lines.append(f"{prefix} candidates=0")
            continue

        candidate_text = []
        for dolphin_index, dolphin_packet in candidates[:max_candidates]:
            candidate_text.append(packet_context(dolphin_packet, "dolphin", dolphin_index))
        suffix = "; ".join(candidate_text)
        if len(candidates) > max_candidates:
            suffix += f"; +{len(candidates) - max_candidates} more"
        lines.append(f"{prefix} candidates={len(candidates)} first={suffix}")

    return lines


def compare_scene_diagnostics(findings: list[Finding], pc: dict[str, Any]) -> None:
    snapshot = pc.get("scene_snapshot", [])
    if not isinstance(snapshot, list):
        snapshot = []
    live_actor_count = 0
    for entry in snapshot:
        if isinstance(entry, dict) and entry.get("live_actor") is not None:
            live_actor_count += 1
    if live_actor_count == 0:
        findings.append(Finding("missing", "pc.scene_snapshot.live_actor", "PC trace has no generic live actor diagnostics"))


def make_report(dolphin_path: Path, pc_path: Path, max_packet_comparisons: int, max_correlations: int, max_candidates: int) -> tuple[str, int]:
    dolphin = load_trace(dolphin_path)
    pc = load_trace(pc_path)
    findings: list[Finding] = []

    compare_frame(findings, dolphin, pc)
    compare_copy_events(findings, dolphin, pc)
    packet_summaries = compare_render_packets(findings, dolphin, pc, max_packet_comparisons)
    packet_correlations = correlate_render_packets(dolphin, pc, max_correlations, max_candidates)
    compare_scene_diagnostics(findings, pc)

    mismatch_count = sum(1 for finding in findings if finding.severity in {"mismatch", "missing"})
    lines = [
        "trace_diff_schema: smgpc-trace-diff-v1",
        f"dolphin_trace: {dolphin_path}",
        f"pc_trace: {pc_path}",
        f"dolphin_frame: {at(dolphin, 'frame.index')}",
        f"pc_frame: {at(pc, 'frame.index')}",
        f"dolphin_render_packets: {len(dolphin.get('render_packets', [])) if isinstance(dolphin.get('render_packets'), list) else 0}",
        f"pc_render_packets: {len(pc.get('render_packets', [])) if isinstance(pc.get('render_packets'), list) else 0}",
        f"dolphin_copy_events: {copy_event_kind_sequence(dolphin)}",
        f"pc_copy_events: {copy_event_kind_sequence(pc)}",
        f"mismatch_count: {mismatch_count}",
    ]

    runtime_services = pc.get("runtime_services")
    if isinstance(runtime_services, dict):
        rfl = runtime_services.get("rfl", {})
        save = runtime_services.get("save", {})
        lines.append(
            f"pc_runtime_services: rfl_initialized={rfl.get('initialized')} "
            f"rfl_valid_mii_count={rfl.get('valid_mii_count')} save_file_count={save.get('file_count')}"
        )

    if packet_summaries:
        lines.append("packet_summaries:")
        lines.extend(f"  - {summary}" for summary in packet_summaries)

    if packet_correlations:
        lines.append("packet_correlations:")
        lines.extend(f"  - {correlation}" for correlation in packet_correlations)

    if findings:
        lines.append("findings:")
        for finding in findings:
            lines.append(f"  - severity: {finding.severity}")
            lines.append(f"    path: {finding.path}")
            lines.append(f"    message: {finding.message}")
            lines.append(f"    dolphin: {json.dumps(finding.dolphin, ensure_ascii=False)}")
            lines.append(f"    pc: {json.dumps(finding.pc, ensure_ascii=False)}")
            if finding.context:
                lines.append(f"    context: {finding.context}")
    else:
        lines.append("findings: []")

    return "\n".join(lines) + "\n", mismatch_count


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Compare Dolphin and PC-port SMG parity traces.")
    parser.add_argument("dolphin_trace", type=Path)
    parser.add_argument("pc_trace", type=Path)
    parser.add_argument("--max-packet-comparisons", type=int, default=4)
    parser.add_argument("--max-correlations", type=int, default=12)
    parser.add_argument("--max-candidates", type=int, default=6)
    parser.add_argument("--fail-on-mismatch", action="store_true")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    report, mismatch_count = make_report(
        args.dolphin_trace,
        args.pc_trace,
        max(0, args.max_packet_comparisons),
        max(0, args.max_correlations),
        max(1, args.max_candidates),
    )
    sys.stdout.write(report)
    return 1 if args.fail_on_mismatch and mismatch_count != 0 else 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
