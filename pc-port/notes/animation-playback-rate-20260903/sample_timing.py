#!/usr/bin/env python3
"""Timestamp existing opt-in diagnostics; run from pc-port, no Game changes."""
import argparse
import json
import os
import pathlib
import re
import subprocess
import time

parser = argparse.ArgumentParser()
parser.add_argument('name')
parser.add_argument('--no-vsync', action='store_true')
parser.add_argument('--screenshot')
parser.add_argument('--walk', action='store_true')
args = parser.parse_args()
notes = pathlib.Path('notes/animation-playback-rate-20260903')
command = ['build/macosx/arm64/debug/smg-pc-showcase', 'gateway', '--disc',
           '../Super Mario Wii - Galaxy Adventure (Korea).rvz', '--max-frames', '241']
if args.screenshot:
    command += ['--screenshot', args.screenshot, '--screenshot-frame', '130']
overrides = {'SMGPC_AURORA_RENDER_STATS': '1', 'SMGPC_DEBUG_SIMULATION_TIMING': '1'}
if args.no_vsync:
    overrides['SMGPC_ENABLE_VSYNC'] = '0'
env = os.environ.copy()
env.update(overrides)
pattern = re.compile(r'\[smgpc:timing\] tick=(\d+) present=(\d+) bck=(\S+) bck_frame=([-+.\d]+) bck_rate=([-+.\d]+) bck_end=(\d+) position=\(([^)]+)\)')
present_pattern = re.compile(r'\[smgpc:render\] frame=(\d+)')
samples, presentations, keys = [], [], []
start = time.monotonic()
with (notes / (args.name + '.log')).open('w') as log:
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               text=True, bufsize=1, env=env)
    if args.walk:
        import ctypes
        cg = ctypes.CDLL('/System/Library/Frameworks/CoreGraphics.framework/CoreGraphics')
        cf = ctypes.CDLL('/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation')
        cg.CGEventCreateKeyboardEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint16, ctypes.c_bool]
        cg.CGEventCreateKeyboardEvent.restype = ctypes.c_void_p
        cg.CGEventPostToPid.argtypes = [ctypes.c_int32, ctypes.c_void_p]
        cf.CFRelease.argtypes = [ctypes.c_void_p]
        def post_key(down, tick):
            if down:
                activation = subprocess.run(
                    ['osascript', '-e', 'tell application "System Events" to set frontmost of '
                     f'first application process whose unix id is {process.pid} to true'],
                    capture_output=True, text=True)
                if activation.returncode:
                    raise RuntimeError(activation.stderr.strip())
            event = cg.CGEventCreateKeyboardEvent(None, 13, down)  # Physical W key.
            if not event:
                raise RuntimeError('CGEventCreateKeyboardEvent failed')
            cg.CGEventPostToPid(process.pid, event)
            cf.CFRelease(event)
            keys.append({'key': 'W', 'down': down, 'after_tick': tick,
                         'seconds': time.monotonic() - start})
    try:
        for line in process.stdout:
            log.write(line)
            now = time.monotonic() - start
            if match := present_pattern.search(line):
                presentations.append({'present': int(match[1]), 'seconds': now})
            if match := pattern.search(line):
                tick = int(match[1])
                samples.append({'tick': tick, 'present': int(match[2]), 'bck': match[3],
                                'bck_frame': float(match[4]), 'bck_rate': float(match[5]),
                                'bck_end': int(match[6]),
                                'position': [float(v) for v in match[7].split(',')],
                                'seconds': now})
                if args.walk and not keys and tick >= 60:
                    post_key(True, tick)
                elif args.walk and len(keys) == 1 and tick >= 150:
                    post_key(False, tick)
        code = process.wait()
    finally:
        if args.walk and len(keys) == 1 and process.poll() is None:
            post_key(False, samples[-1]['tick'])
        if process.poll() is None:
            process.terminate()
            process.wait()
result = {'command': command, 'environment': overrides, 'returncode': code,
          'wall_seconds': time.monotonic() - start, 'samples': samples,
          'presentations': presentations, 'input_events': keys}
steady = [entry for entry in samples if entry['tick'] >= 60]
if len(steady) > 1:
    first, last = steady[0], steady[-1]
    elapsed = last['seconds'] - first['seconds']
    result['steady'] = {'first_tick': first['tick'], 'last_tick': last['tick'],
                        'elapsed_seconds': elapsed,
                        'ticks_per_second': (last['tick'] - first['tick']) / elapsed,
                        'presents_per_second': (last['present'] - first['present']) / elapsed}
(notes / (args.name + '.json')).write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps({key: value for key, value in result.items()
                  if key not in ('samples', 'presentations')}, indent=2))
print('sample_count:', len(samples), 'actual BCKs:', sorted({v['bck'] for v in samples}))
raise SystemExit(code)
