#!/usr/bin/env python3
"""Repeat the recorded native compilation and isolated boundary fixture.

The captured host command lines also use the parent's camera-owner overlay;
this is compilation evidence, not a complete CameraDirector link/runtime test.
"""
from pathlib import Path
import json
import shutil
import subprocess

NOTE = Path(__file__).resolve().parent
ROOT = NOTE.parents[2]
BUILD = ROOT / 'build/original-camera-compile-closure-20260903'
subprocess.run(['python3', str(NOTE / 'stage-native.py')], cwd=ROOT, check=True)
results = []
for item in json.loads((NOTE / 'native-commands.json').read_text()):
    result = subprocess.run(item['command'], cwd=ROOT / 'pc-port',
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (BUILD / (item['source'] + '.native.log')).write_text(result.stdout)
    results.append({'source': item['source'], 'exit': result.returncode})
    if result.returncode:
        print(item['source'], result.stdout)
assert len(results) == 127 and all(item['exit'] == 0 for item in results)
for name in ['CameraCompileBoundaryTests.cpp', 'GeneralParamCopy.cpp']:
    shutil.copyfile(NOTE / name, BUILD / name)
subprocess.run(json.loads((NOTE / 'boundary-build.json').read_text()),
               cwd=ROOT / 'pc-port', check=True)
subprocess.run([str(BUILD / 'camera-boundary-tests')], check=True)
print('All 127 original camera translation units compile; native boundaries pass.')
