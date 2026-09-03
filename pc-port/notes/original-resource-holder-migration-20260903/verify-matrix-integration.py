#!/usr/bin/env python3
"""Build the actual matrix-library consumers and run focused CPU regressions."""
import hashlib
import argparse
import json
import os
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
PC = HERE.parents[1]
TARGETS = [
    'smg-pc-original-resource-holder-tests', 'smg-pc-showcase',
    'smg-pc-original-camera-vector-math-tests', 'smg-pc-game-math-rotation-tests',
    'smg-pc-original-camera-runtime-tests', 'smg-pc-only-camera-tests',
    'smg-pc-camera-view-interpolator-tests', 'smg-pc-camera-view-service-tests',
    'smg-pc-stage-start-camera-tests', 'smg-pc-actor-event-camera-tests',
    'smg-pc-original-xanime-core-tests', 'smg-pc-original-xanime-player-tests',
    'smg-pc-original-j3d-joint-traversal-tests', 'smg-pc-fixed-step-clock-tests',
    'smg-pc-original-j3d-model-resource-tests',
]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--targets', nargs='+', choices=TARGETS)
    selected = parser.parse_args().targets or TARGETS
    env = dict(os.environ, SMGPC_REAL_DISC=str(PC.parent / 'Super Mario Wii - Galaxy Adventure (Korea).rvz'))
    results = []
    for target in selected:
        build = ['xmake', 'build', target]
        output = subprocess.run(build, cwd=PC, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        (HERE / (target + '.matrix-build.log')).write_text(output.stdout)
        entry = {'target': target, 'build': build, 'build_returncode': output.returncode}
        results.append(entry)
        print(target + ' build: ' + str(output.returncode), flush=True)
        if output.returncode:
            break
        binary = PC / 'build/macosx/arm64/debug' / target
        entry['binary_sha256'] = hashlib.sha256(binary.read_bytes()).hexdigest()
        if target == 'smg-pc-showcase':
            continue
        output = subprocess.run([str(binary)], cwd=PC, env=env, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, text=True, timeout=120)
        (HERE / (target + '.matrix-run.log')).write_text(output.stdout)
        entry['run_returncode'] = output.returncode
        entry['skips'] = [line for line in output.stdout.splitlines() if 'SKIP' in line]
        print(target + ' run: ' + str(output.returncode), flush=True)
        if output.returncode:
            print(output.stdout, flush=True)
    path = HERE / 'matrix-integration-evidence.json'
    recorded = {row['target']: row for row in json.loads(path.read_text())} if path.exists() else {}
    recorded.update({row['target']: row for row in results})
    path.write_text(json.dumps([recorded[t] for t in TARGETS if t in recorded], indent=2) + '\n')
    assert len(results) == len(selected)
    assert all(row['build_returncode'] == 0 and row.get('run_returncode', 0) == 0 for row in results)


if __name__ == '__main__':
    main()
