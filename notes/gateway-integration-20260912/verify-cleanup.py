import json, os, subprocess, time
from pathlib import Path
root = Path(__file__).resolve().parents[2]
out = Path(__file__).resolve().parent
names = ["original-message-holder", "file-select-name-real-or-absent", "message-real-or-absent", "original-layout-group", "picture-font-tag", "original-resource-holder", "aurora-native", "npc-actor-real-or-absent", "sceneobj-holder-real-or-absent", "original-shadow-controller-owner"]
results = []
env = dict(os.environ, SMGPC_REAL_DISC=str(root / "Super Mario Wii - Galaxy Adventure (Korea).rvz"))
for name in names:
    target = "smg-pc-" + name + "-tests"
    record = dict(target=target)
    start = time.monotonic()
    with (out / (name + ".cleanup-build.log")).open("w") as log:
        record["build_exit"] = subprocess.run(["xmake", "build", "-j8", target], cwd=root, stdout=log, stderr=subprocess.STDOUT).returncode
    record["build_seconds"] = time.monotonic() - start
    if record["build_exit"] == 0:
        start = time.monotonic()
        with (out / (name + ".cleanup-run.log")).open("w") as log:
            try:
                record["run_exit"] = subprocess.run([str(root / "build/macosx/arm64/debug" / target)], cwd=root, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=55).returncode
            except subprocess.TimeoutExpired:
                record["run_exit"] = "timeout"
        record["run_seconds"] = time.monotonic() - start
    results.append(record)
    (out / "cleanup-results.json").write_text(json.dumps(results, indent=2) + "\n")
    print(json.dumps(record), flush=True)
