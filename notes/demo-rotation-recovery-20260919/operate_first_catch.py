"""Supervised controller-only Gateway operator, with optional sequential JSON route.

Only the opt-in controller file is written. Actor traces are read-only; this
operator cannot set game memory, switches, actors, story state or positions.
The separate game runner owns process lifetime and completion verification.
"""
import argparse
import fcntl
import json
import math
import os
from pathlib import Path
import tempfile
import time

from follow_actor import controls


def state(actor, name):
    return (actor is not None and
            "Nrv" + name + "E" in (actor.get("nerve") or {}).get("type", ""))


def vector(value, field):
    if (not isinstance(value, list) or len(value) != 3 or
            any(isinstance(v, bool) or not isinstance(v, (float, int)) or not math.isfinite(v) for v in value)):
        raise ValueError(f"{field} must contain three finite numbers")
    return value


def extra_button_script(text):
    # Preflight copied from DebugWpadInputScript.cpp's public grammar. The
    # native parser remains authoritative; return valid input unchanged.
    if not isinstance(text, str):
        raise argparse.ArgumentTypeError("extra buttons must be a frame-span string")
    whitespace = " \t\n\r\v\f"
    names = {"A", "B", "UP", "DOWN", "LEFT", "RIGHT", "PLUS", "MINUS", "HOME",
             "C", "Z", "ONE", "1", "TWO", "2"}

    def frame(value):
        value = value.strip(whitespace)
        if not value or any(c < "0" or c > "9" for c in value):
            raise argparse.ArgumentTypeError("extra buttons require unsigned decimal frame indices")
        number = int(value)
        if number > 18446744073709551615:
            raise argparse.ArgumentTypeError("extra buttons frame index exceeds native uint64 range")
        return number

    for entry in text.split(";"):
        entry = entry.strip(whitespace)
        if not entry:
            continue
        span, colon, buttons = entry.partition(":")
        if not colon:
            raise argparse.ArgumentTypeError("extra buttons require frame-range:buttons entries separated by ';'")
        first, dash, last = span.partition("-")
        first = frame(first)
        if dash and last.strip(whitespace) and frame(last) < first:
            raise argparse.ArgumentTypeError("extra buttons frame range must not end before it starts")
        if any(button.strip(whitespace) not in names for button in buttons.split("+")):
            raise argparse.ArgumentTypeError("extra buttons require known button names joined by '+', with spans separated by ';'")
    return text


def load_plan(path):
    plan = json.loads(path.read_text())
    if not isinstance(plan, dict) or plan.get("version") != 1:
        raise ValueError("route plan requires version 1")
    route = plan.get("route")
    if not isinstance(route, list) or not route:
        raise ValueError("route plan requires a nonempty route list")
    labels, spawns = set(), set()
    for item in route:
        if not isinstance(item, dict) or not isinstance(item.get("label"), str) or not item["label"]:
            raise ValueError("each route entry requires a nonempty label")
        if item["label"] in labels:
            raise ValueError("route labels must be unique")
        labels.add(item["label"])
        vector(item.get("waypoint"), "waypoint")
        spawn = tuple(vector(item.get("rabbit_spawn"), "rabbit_spawn"))
        if spawn in spawns:
            raise ValueError("route rabbit spawns must be distinct")
        spawns.add(spawn)
    binding_frame = plan.get("binding_frame", 1400)
    if type(binding_frame) is not int or binding_frame < 0:
        raise ValueError("binding_frame must be a nonnegative integer")
    if type(plan.get("wait_for_rosetta", True)) is not bool:
        raise ValueError("wait_for_rosetta must be boolean")
    return plan


def available(source):
    """Yield every complete sample; keep incomplete append tails for next poll."""
    while True:
        offset = source.tell()
        line = source.readline()
        if not line.endswith("\n"):
            source.seek(offset)
            return
        yield json.loads(line)


class StallJump:
    """Ordinary input-only obstacle attempt, inferred from observed movement."""
    def __init__(self):
        self.samples = []
        self.key = None
        self.next_frame = 0
        self.active = None

    def update(self, frame, position, key, safe, sample_allowed):
        if key != self.key or not safe:
            self.samples.clear()
            self.active = None
            self.key = key
        if not safe:
            return "", None
        if self.active and frame <= self.active['last_frame']:
            return self.active['script'], self.active.copy()
        self.active = None
        if not sample_allowed or frame < self.next_frame:
            self.samples.clear()
            return "", None
        if self.samples and not 0 < frame - self.samples[-1][0] <= 60:
            self.samples.clear()
        self.samples.append((frame, list(position)))
        while len(self.samples) > 1 and self.samples[1][0] <= frame - 240:
            self.samples.pop(0)
        if frame - self.samples[0][0] < 240:
            return "", None
        extent = [max(p[i] for _, p in self.samples) - min(p[i] for _, p in self.samples)
                  for i in range(3)]
        diagonal = math.sqrt(sum(v*v for v in extent))
        if diagonal >= 20:
            return "", None
        self.active = {'reason': 'nonzero_input_with_small_position_bounds',
                       'observed_first_frame': self.samples[0][0], 'observed_last_frame': frame,
                       'position_bounds_diagonal': diagonal,
                       'first_frame': frame + 1, 'last_frame': frame + 30,
                       'script': f'{frame + 1}-{frame + 30}:A'}
        self.next_frame = frame + 300
        self.samples.clear()
        return self.active['script'], self.active.copy()


class Operator:
    def __init__(self, route, start, end, binding_frame=None, wait_for_rosetta=False, rabbit_id=None, extra_buttons=""):
        self.route, self.start, self.end = route, start, end
        self.extra_buttons = extra_button_script(extra_buttons)
        self.binding_frame, self.wait_for_rosetta = binding_frame, wait_for_rosetta
        self.rabbit_ids = None if binding_frame is not None else [rabbit_id]
        self.binding_evidence = None
        self.index, self.guide_id = 0, None
        self.phase, self.next_press, self.current_press = "opening", start, ""
        self.caught_frame = self.tico_id = self.tico_talk_frame = None
        self.existing_talks, self.completed = set(), []
        self.finished_reason = None
        self.rosetta_id = None
        self.stall_jump = StallJump()

    def bind(self, snapshot):
        if self.rabbit_ids is not None or snapshot["frame_index"] < self.binding_frame:
            return
        rabbits = [a for a in snapshot["actors"] if a["type"] == "13RunawayRabbit"]
        if not rabbits or not all(state(a, "NoActive") for a in rabbits):
            raise RuntimeError("Route binding requires the early static NoActive rabbit snapshot")
        ids, evidence = [], []
        for item in self.route:
            matches = [(math.dist(a["position"], item["rabbit_spawn"]), a) for a in rabbits]
            distance, actor = min(matches, key=lambda pair: pair[0])
            if distance >= 10 or actor["id"] in ids:
                raise RuntimeError(f"Cannot uniquely bind {item['label']} to its authored spawn within 10 units")
            if sum(d < 10 for d, _ in matches) != 1:
                raise RuntimeError(f"Ambiguous authored spawn for {item['label']}")
            ids.append(actor["id"])
            evidence.append({"label": item["label"], "rabbit_id": actor["id"], "distance": distance})
        self.rabbit_ids = ids
        self.binding_evidence = {"frame": snapshot["frame_index"], "matches": evidence}

    def rabbit(self, by_id):
        return by_id.get(self.rabbit_ids[self.index]) if self.rabbit_ids is not None and self.index < len(self.route) else None

    def observe(self, snapshot):
        self.bind(snapshot)
        frame, actors = snapshot["frame_index"], snapshot["actors"]
        if frame < self.start or self.finished_reason:
            return
        by_id = {a["id"]: a for a in actors}
        if self.guide_id is None:
            guide = next((a for a in actors if a["type"] == "10DemoRabbit" and
                          any(state(a, n) for n in ("Talk0", "Guide", "Wait", "Goal", "Talk1"))), None)
            if guide is not None:
                self.guide_id = guide["id"]
        guide = by_id.get(self.guide_id)
        collector = next((a for a in actors if a["type"] == "20RunawayRabbitCollect"), None)
        if self.phase in ("opening", "guide", "dialogue"):
            if state(collector, "Active"):
                if self.rabbit_ids is None:
                    raise RuntimeError("Collector became active before authored rabbit binding")
                self.phase = "reveal"
            elif state(guide, "Talk1"):
                self.phase = "dialogue"
            elif any(state(guide, n) for n in ("Wait", "Guide", "Goal")):
                self.phase = "guide"
        if self.phase == "reveal":
            candidates = [self.rabbit(by_id)] if self.rabbit_ids[self.index] is not None else actors
            rabbit = next((a for a in candidates if a and a["type"] == "13RunawayRabbit" and
                           not a["dead"] and any(state(a, n) for n in ("Appear", "Runaway", "Stop", "BlowDamage"))), None)
            if rabbit is not None:
                self.rabbit_ids[self.index] = rabbit["id"]
                self.phase = "chase"
        rabbit = self.rabbit(by_id)
        talking = {a["id"] for a in actors if a["type"] == "11RunawayTico" and not a["dead"] and state(a, "Talk")}
        if self.phase == "chase" and any(state(rabbit, n) for n in ("TryCaughtDemo", "Caught", "CaughtTalk", "CaughtEnd")):
            self.phase, self.caught_frame = "caught", frame
            self.next_press, self.existing_talks = frame + 120, talking.copy()
            self.tico_id = self.tico_talk_frame = None
        if self.phase == "caught":
            newly_talking = talking - self.existing_talks
            if self.tico_id is None and newly_talking:
                if len(newly_talking) != 1:
                    raise RuntimeError("Ambiguous post-catch Tico dialogue")
                self.tico_id = next(iter(newly_talking))
                self.tico_talk_frame = frame
            tico = by_id.get(self.tico_id)
            # Observe the original Talk -> Wait/WhiteOut transition. Elapsed
            # time, an absent actor, or rabbit death alone cannot complete it.
            talk_completed = tico is not None and any(state(tico, n) for n in ("Wait", "WhiteOut"))
            if rabbit is not None and rabbit["dead"] and talk_completed:
                self.completed.append({"label": self.route[self.index]["label"], "rabbit_id": rabbit["id"],
                                       "caught_frame": self.caught_frame, "tico_id": self.tico_id,
                                       "tico_talk_frame": self.tico_talk_frame, "completed_frame": frame})
                self.index += 1
                self.current_press = ""
                if self.index < len(self.route):
                    self.phase = "reveal"
                    self.tico_id = self.tico_talk_frame = None
                elif self.wait_for_rosetta:
                    self.phase = "await_rosetta"
                else:
                    self.phase, self.finished_reason = "complete", "route_dialogue_complete"
        if self.phase == "await_rosetta":
            rosetta = next((a for a in actors if a["type"] == "7Rosetta" and
                            a.get("dead") is False and a.get("hidden") is False), None)
            if rosetta is not None:
                self.rosetta_id = rosetta["id"]
                self.phase, self.finished_reason = "complete", "rosetta_alive_not_hidden"
        if frame >= self.end and not self.finished_reason:
            self.finished_reason = "frame_limit"

    def decision(self, snapshot):
        frame, actors = snapshot["frame_index"], snapshot["actors"]
        player = next((a for a in actors if "player" in a), None)
        if player is None or frame < self.start:
            return None
        by_id = {a["id"]: a for a in actors}
        target, distance, x, y = None, None, 0, 0
        if self.phase == "guide":
            target = by_id.get(self.guide_id)
        elif self.phase == "reveal":
            target = {"position": self.route[self.index]["waypoint"]}
        elif self.phase == "chase":
            target = self.rabbit(by_id)
        if target is not None:
            x, y, distance = controls(player, target, 0 if self.phase == "chase" else 130,
                                      full_speed=self.phase == "chase")
        tico_talk = any(a["type"] == "11RunawayTico" and not a["dead"] and state(a, "Talk") for a in actors)
        if tico_talk:
            x = y = 0
        allow_press = self.phase in ("opening", "dialogue", "caught") or tico_talk
        if allow_press and self.phase != "await_rosetta" and frame >= self.next_press:
            self.current_press, self.next_press = f"{frame + 1}-{frame + 10}:A", frame + 120
        if not allow_press or self.phase == "await_rosetta":
            self.current_press = ""
        if self.finished_reason:
            x = y = 0
            self.current_press = ""
        pipe_active = any(a['type'] == '11EarthenPipe' and not a['dead'] and
                          any(state(a, n) for n in ('Ready', 'PlayerIn', 'PlayerOut')) for a in actors)
        info = player.get('player', {})
        accepted_input = (info.get('stick_position', [0, 0, 0])[2] > 0.05 and
                          math.sqrt(sum(v*v for v in info.get('world_pad_direction', [0, 0, 0]))) > 0.05)
        safe_jump = (self.phase in ('reveal', 'chase') and not self.finished_reason and
                     not tico_talk and not pipe_active and info.get('status') == 0 and accepted_input)
        sample_jump = math.hypot(x, y) > 0.05 and info.get('ground_triangle') is not None
        jump_script, automatic_jump = self.stall_jump.update(
            frame, player['position'], (self.phase, self.index), safe_jump, sample_jump)
        buttons = "" if self.finished_reason else ";".join(filter(None, (self.current_press, self.extra_buttons, jump_script)))
        command = {"buttons": buttons, "pointer": "",
                   "stick": f"{frame + 1}-{frame + 90}:{x:.6f}:{y:.6f}"}
        return {"frame": frame, "phase": self.phase, "route_index": self.index, "guide_id": self.guide_id,
                "rabbit_id": None if self.index == len(self.route) or self.rabbit_ids is None else self.rabbit_ids[self.index],
                "binding": self.binding_evidence, "completed_catches": self.completed.copy(), "tico_talk_id": self.tico_id,
                "rosetta_id": self.rosetta_id, "player": player["position"],
                "target": None if target is None else {k: target[k] for k in ("id", "position", "nerve") if k in target},
                "distance": distance, "extra_buttons": self.extra_buttons, "automatic_jump": automatic_jump,
                "command": command, "finished": bool(self.finished_reason),
                "finished_reason": self.finished_reason}


def publish(path, command):
    fd, temporary = tempfile.mkstemp(prefix=path.name + ".", suffix=".next", dir=path.parent)
    try:
        with os.fdopen(fd, "w") as output:
            output.write(json.dumps(command) + "\n")
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path)
    parser.add_argument("input", type=Path)
    choice = parser.add_mutually_exclusive_group(required=True)
    choice.add_argument("--waypoint", type=float, nargs=3)
    choice.add_argument("--plan", type=Path, help="Version-1 JSON sequential route with authored rabbit spawns")
    parser.add_argument("--extra-buttons", type=extra_button_script, default="",
                        help="Explicit additional controller spans, e.g. 15000-15025:A; no automatic jumps")
    parser.add_argument("--extra-buttons-file", type=Path, help="Supervised ordinary-button spans, re-read before each decision")
    parser.add_argument("--manual-input-file", type=Path, help="Optional supervised normalized stick override with until_frame; ordinary controller only")
    parser.add_argument("--rabbit-id", type=int, help="Current runtime rabbit ID, single-waypoint mode only")
    parser.add_argument("--start", type=int, default=1400)
    parser.add_argument("--end", type=int, default=16000)
    parser.add_argument("--timeout", type=float, default=600)
    args = parser.parse_args()
    if args.start < 0 or args.end <= args.start or not math.isfinite(args.timeout) or args.timeout <= 0:
        parser.error("require nonnegative start, end > start, and positive finite timeout")
    if args.plan:
        if args.rabbit_id is not None:
            parser.error("--rabbit-id cannot be used with --plan")
        plan = load_plan(args.plan)
        operator = Operator(plan["route"], args.start, args.end, plan.get("binding_frame", 1400), plan.get("wait_for_rosetta", True), extra_buttons=args.extra_buttons)
    else:
        vector(args.waypoint, "waypoint")
        operator = Operator([{"label": "single", "waypoint": args.waypoint}], args.start, args.end, rabbit_id=args.rabbit_id, extra_buttons=args.extra_buttons)
    args.input = args.input.resolve()
    deadline = time.monotonic() + args.timeout
    with args.input.with_name(args.input.name + ".operator-lock").open("a") as owner:
        fcntl.flock(owner, fcntl.LOCK_EX | fcntl.LOCK_NB)
        try:
            while not args.trace.exists() and time.monotonic() < deadline:
                time.sleep(0.1)
            if not args.trace.exists():
                raise TimeoutError("Actor trace did not appear before wall-clock limit")
            with args.trace.open() as source:
                while time.monotonic() < deadline:
                    snapshot = None
                    for snapshot in available(source):
                        operator.observe(snapshot)
                    if snapshot is None:
                        time.sleep(0.08)
                        continue
                    if args.extra_buttons_file:
                        operator.extra_buttons = extra_button_script(args.extra_buttons_file.read_text().strip())
                    result = operator.decision(snapshot)
                    if result is None:
                        continue
                    if args.manual_input_file:
                        manual = json.loads(args.manual_input_file.read_text())
                        if snapshot["frame_index"] < manual.get("until_frame", 0):
                            x, y = manual["stick"]
                            if not all(isinstance(v, (int, float)) and math.isfinite(v) and -1 <= v <= 1 for v in (x, y)):
                                raise ValueError("Manual stick values must be finite and normalized")
                            until = min(manual["until_frame"], snapshot["frame_index"] + 90)
                            result["command"]["stick"] = f"{snapshot['frame_index'] + 1}-{until}:{x}:{y}"
                            result["manual_input"] = manual
                    publish(args.input, result["command"])
                    print(json.dumps(result), flush=True)
                    if result["finished"]:
                        return 0 if operator.finished_reason != "frame_limit" else 2
            raise TimeoutError("Controller operator reached its wall-clock limit")
        finally:
            publish(args.input, {"buttons": "", "pointer": "", "stick": ""})


if __name__ == "__main__":
    raise SystemExit(main())
