#!/usr/bin/env python3
"""Recompile frozen and corrected reference TUs; compare with verified retail."""
import hashlib
import json
from pathlib import Path
import subprocess


NOTES = Path(__file__).resolve().parent
ROOT = NOTES.parent.parent
DECOMP = ROOT / "decomp"
PREVIOUS = NOTES.parent / "original-player-state-recovery-20260907"
NAMES = ("MarioHang", "MarioTeresa")


def run(command, log):
    result = subprocess.run(command, cwd=DECOMP, capture_output=True, text=True)
    log.write_text(result.stdout + result.stderr)
    result.check_returncode()
    return {"command": command, "result": result.returncode}


def main():
    old_compile = json.loads((PREVIOUS / "compile-results.json").read_text())
    old_diff = json.loads((PREVIOUS / "objdiff-results.json").read_text())
    commands = []
    proof = []
    for name in NAMES:
        for variant in ("baseline", "restored"):
            source = str(NOTES / "baseline" / f"{name}.cpp") if variant == "baseline" else f"src/Game/Player/{name}.cpp"
            obj = str(NOTES / f"{name}.{variant}.o")
            command = list(next(row for row in old_compile if row["unit"] == name)["command"])
            command[command.index("-c") + 1] = source
            command[command.index("-o") + 1] = obj
            commands.append({"unit": name, "variant": variant, "operation": "compile", **run(command, NOTES / f"{name}.{variant}.compile.log")})
            command = list(next(row for row in old_diff if row["unit"] == name)["command"])
            command[command.index("-2") + 1] = obj
            command[command.index("-o") + 1] = str(NOTES / f"{name}.{variant}.objdiff.json")
            commands.append({"unit": name, "variant": variant, "operation": "objdiff", **run(command, NOTES / f"{name}.{variant}.objdiff.log")})

        before = json.loads((NOTES / f"{name}.baseline.objdiff.json").read_text())
        after = json.loads((NOTES / f"{name}.restored.objdiff.json").read_text())
        old_symbols = {s["name"]: s for s in before["left"]["symbols"]}
        new_symbols = {s["name"]: s for s in after["right"]["symbols"]}
        changed = []
        regressions = []
        for symbol in after["left"]["symbols"]:
            if not symbol.get("instructions") or symbol["name"] not in old_symbols:
                continue
            old_score = old_symbols[symbol["name"]].get("match_percent")
            new_score = symbol.get("match_percent")
            if old_score is None or new_score is None or old_score == new_score:
                continue
            row = {
                "symbol": symbol["name"],
                "before": old_score,
                "after": new_score,
                "retail_bytes": int(symbol["size"]),
                "candidate_bytes": int(new_symbols[symbol["name"]]["size"]),
            }
            changed.append(row)
            if new_score < old_score:
                regressions.append(row)
        proof.append({
            "unit": name,
            "reference_checkpoint": "7c1c6b055",
            "source_sha256": hashlib.sha256((DECOMP / f"src/Game/Player/{name}.cpp").read_bytes()).hexdigest(),
            "changed_functions": changed,
            "regressions": regressions,
        })
    (NOTES / "validation-commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    (NOTES / "function-proof.json").write_text(json.dumps(proof, indent=2) + "\n")
    print(json.dumps(proof, indent=2))


if __name__ == "__main__":
    main()
