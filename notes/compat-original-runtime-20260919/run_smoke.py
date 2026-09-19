#!/usr/bin/env python3
"""Bounded fresh-save proof of the sole original-process application entry."""
import argparse
from datetime import datetime, timezone
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import time

root = Path(__file__).resolve().parents[2]
notes = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('label')
parser.add_argument('--frames', type=int, required=True)
parser.add_argument('--stage')
parser.add_argument('--scenario', type=int, default=1)
parser.add_argument('--screenshot-frame', type=int)
parser.add_argument('--timeout', type=float, default=180)
parser.add_argument('--expect-strict-failure', action='store_true')
args = parser.parse_args()
if Path(args.label).name != args.label or args.label in ('.', '..') or args.frames < 1:
    parser.error('use a single-component fresh label and positive frame count')
if args.expect_strict_failure and not args.stage:
    parser.error('strict placement proof requires a stage')
if any(notes.glob(args.label + '*')):
    parser.error('label already exists; preserve evidence and use a fresh save directory')
binary = root / 'build/macosx/arm64/debug/smg-pc.app/Contents/MacOS/smg-pc'
disc = root / 'Super Mario Wii - Galaxy Adventure (Korea).rvz'
command = [str(binary), '--disc', str(disc), '--max-frames', str(args.frames)]
if args.stage:
    command += ['--stage', args.stage, '--scenario', str(args.scenario)]
settings = {
    'SMGPC_SAVE_DIR': str(notes / (args.label + '-save')),
    'SMGPC_NAND_DIR': '',
    'AURORA_BACKEND': 'metal',
    'SMGPC_WINDOW_WIDTH': '1280', 'SMGPC_WINDOW_HEIGHT': '720',
    'SMGPC_DEBUG_FRAME_TIMING': '1',
    'SMGPC_ORIGINAL_PLACEMENT_REPORT_PATH': str(notes / (args.label + '-placements.json')),
    'SMGPC_STRICT_PLACEMENT': '1' if args.expect_strict_failure else '',
}
if args.screenshot_frame is not None:
    settings['SMGPC_SCREENSHOT_PATH'] = str(notes / (args.label + '-frame' + str(args.screenshot_frame) + '.png'))
    settings['SMGPC_SCREENSHOT_FRAME'] = str(args.screenshot_frame)
# Do not inherit earlier controller scripts, selected stages, captures or traces.
environment = {k: v for k, v in os.environ.items() if not k.startswith('SMGPC_')}
environment.update(settings)
record = {'command': command, 'environment': settings, 'input': 'neutral; no scripted controller input',
          'inherited_smgpc_environment_removed': True,
          'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
          'started_at': datetime.now(timezone.utc).isoformat(), 'requested_frames': args.frames}
log = notes / (args.label + '.log')
start = time.monotonic()
with log.open('w') as output:
    process = subprocess.Popen(command, cwd=root, env=environment, stdout=output, stderr=subprocess.STDOUT)
    record['pid'] = process.pid
    (notes / (args.label + '-launch.json')).write_text(json.dumps(record, indent=2) + '\n')
    try:
        record['exit_code'] = process.wait(timeout=args.timeout)
        record['timeout_terminated'] = False
    except subprocess.TimeoutExpired:
        process.terminate()
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
        record['exit_code'] = process.returncode
        record['timeout_terminated'] = True
record['elapsed_seconds'] = time.monotonic() - start
try:
    os.kill(process.pid, 0)
    record['process_still_exists_after_wait'] = True
except ProcessLookupError:
    record['process_still_exists_after_wait'] = False
text = log.read_text(errors='replace')
completed = re.findall(r'Original GameSystem stopped after (\d+) completed frames', text)
record['completed_frames'] = int(completed[-1]) if completed else None
record['binary_sha256_after'] = hashlib.sha256(binary.read_bytes()).hexdigest()
record['binary_unchanged'] = record['binary_sha256'] == record['binary_sha256_after']
record['verified_bounded_completion'] = (record['exit_code'] == 0 and not record['timeout_terminated']
    and not record['process_still_exists_after_wait'] and record['completed_frames'] == args.frames
    and record['binary_unchanged'])
report_path = Path(settings['SMGPC_ORIGINAL_PLACEMENT_REPORT_PATH'])
report = json.loads(report_path.read_text()) if report_path.exists() else None
record['placement_summary'] = {k: report[k] for k in ['stage', 'scenario', 'supported', 'known_unlinked', 'unknown', 'metadata']} if report else None
record['strict_failure_message'] = next((line for line in text.splitlines() if 'Original stage contains an unlinked retail actor:' in line), None)
record['verified_strict_failure'] = (args.expect_strict_failure and record['exit_code'] not in (None, 0)
    and not record['timeout_terminated'] and not record['process_still_exists_after_wait']
    and record['completed_frames'] is None and record['binary_unchanged']
    and record['strict_failure_message'] is not None and report is not None and report['known_unlinked'] > 0)
record['screenshot_exists'] = Path(settings['SMGPC_SCREENSHOT_PATH']).is_file() if 'SMGPC_SCREENSHOT_PATH' in settings else None
record['timing_summary'] = [line for line in text.splitlines() if '[original-frame-timing]' in line or 'Frame timing' in line]
with (notes / (args.label + '.log.gz')).open('wb') as output:
    with gzip.GzipFile(filename='', mode='wb', fileobj=output, mtime=0) as compressed:
        compressed.write(log.read_bytes())
(notes / (args.label + '.json')).write_text(json.dumps(record, indent=2) + '\n')
print(json.dumps(record, indent=2))
raise SystemExit(0 if (record['verified_strict_failure'] if args.expect_strict_failure else record['verified_bounded_completion']) else 1)
