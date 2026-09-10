import hashlib
import json
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[3]
notes = Path(__file__).resolve().parent
phase = sys.argv[1]
gtest = root / 'dolphin/Externals/gtest/googletest'
binary = notes / (phase + '-brlan-tests')
command = [
    '/opt/homebrew/opt/llvm/bin/clang++', '-std=c++23', '-g', '-O2', '-pthread',
    '-ffp-contract=off', '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
    '-Iaurora/include', '-I' + str(gtest / 'include'), '-I' + str(gtest),
    'aurora/tests/brlan_test.cpp', 'aurora/lib/nw4r/brlan.cpp',
    str(gtest / 'src/gtest-all.cc'), str(gtest / 'src/gtest_main.cc'), '-o', str(binary),
]
result = subprocess.run(command, cwd=root, capture_output=True, text=True)
(notes / (phase + '-build.log')).write_text(result.stdout + result.stderr)
evidence = dict(command=command, build_exit=result.returncode,
                sources=[dict(path=p, sha256=hashlib.sha256((root / p).read_bytes()).hexdigest())
                         for p in ['aurora/tests/brlan_test.cpp', 'aurora/lib/nw4r/brlan.cpp']])
if result.returncode == 0:
    run_command = [str(binary), '--gtest_output=xml:' + str(notes / (phase + '-tests.xml'))]
    result = subprocess.run(run_command, cwd=root, capture_output=True, text=True)
    (notes / (phase + '-run.log')).write_text(result.stdout + result.stderr)
    evidence.update(runtime_command=run_command, runtime_exit=result.returncode,
                    binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest())
(notes / (phase + '-results.json')).write_text(json.dumps(evidence, indent=2) + '\n')
print(json.dumps(evidence, indent=2))
