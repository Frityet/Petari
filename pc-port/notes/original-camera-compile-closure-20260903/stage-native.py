#!/usr/bin/env python3
"""Reconstruct the isolated native overlay without editing production sources."""
from pathlib import Path
import hashlib
import json
import subprocess

NOTE = Path(__file__).resolve().parent
ROOT = NOTE.parents[2]
OVERLAY = ROOT / 'build/original-camera-compile-closure-20260903/staged'
manifest = json.loads((NOTE / 'native-manifest.json').read_text())
for item in manifest['files']:
    destination = OVERLAY / Path(item['path']).relative_to('pc-port/src')
    destination.parent.mkdir(parents=True, exist_ok=True)
    if item['new']:
        destination.unlink(missing_ok=True)
    else:
        data = subprocess.check_output(
            ['git', 'show', manifest['baseline_commit'] + ':' + item['path']], cwd=ROOT)
        assert hashlib.sha256(data).hexdigest() == item['native_baseline_sha256']
        destination.write_bytes(data)
subprocess.run(['git', 'apply', '-p3', '--directory=' + str(OVERLAY.relative_to(ROOT)),
                str(NOTE / 'native.patch')], cwd=ROOT, check=True)
for item in manifest['files']:
    destination = OVERLAY / Path(item['path']).relative_to('pc-port/src')
    assert hashlib.sha256(destination.read_bytes()).hexdigest() == item['staged_sha256']
print('Staged', len(manifest['files']), 'native files in', OVERLAY)
