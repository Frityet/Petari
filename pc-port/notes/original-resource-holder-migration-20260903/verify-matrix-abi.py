#!/usr/bin/env python3
"""Verify native integer widths in the real Xmake and CMake matrix targets."""
import hashlib
import json
from pathlib import Path
import re
import subprocess

HERE = Path(__file__).resolve().parent
PC = HERE.parents[1]
ROOT = PC.parent


def main():
    cache = PC / 'build/.deps/aurora-mtx/macosx/arm64/debug/aurora/lib/dolphin/mtx/mtx44.c.o.d'
    values = re.search(r'values = \{(.*?)\n    \}', cache.read_text(), re.S).group(1)
    args = [json.loads(v) for v in re.findall(r'"(?:\\.|[^"\\])*"', values)]
    assert '-DTARGET_PC' in args and '-DAURORA' in args
    source = 'aurora/lib/dolphin/mtx/mtx44.c'
    good = args + ['-fsyntax-only', source]
    bad = [arg for arg in good if arg != '-DTARGET_PC']
    results = []
    for name, command, cwd in [
        ('matrix-abi-good', good, PC),
        ('matrix-abi-missing-target-pc', bad, PC),
        ('matrix-cmake-build', ['cmake', '--build', 'build/aurora-upstream-merge-tests', '--target', 'mtx44_tests', '-j', '4'], ROOT),
        ('matrix-cmake-test', [str(ROOT / 'build/aurora-upstream-merge-tests/tests/mtx44_tests')], ROOT),
    ]:
        result = subprocess.run(command, cwd=cwd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        (HERE / (name + '.log')).write_text(result.stdout)
        results.append({'name': name, 'command': command, 'cwd': str(cwd), 'returncode': result.returncode})
        if name == 'matrix-abi-missing-target-pc':
            assert result.returncode != 0 and 'Aurora matrix library requires' in result.stdout
        else:
            assert result.returncode == 0, result.stdout
        if name == 'matrix-cmake-test':
            assert result.stdout.count('PASS ') == 4
    definitions = subprocess.check_output(['ninja', '-C', 'build/aurora-upstream-merge-tests', '-t', 'commands', 'mtx44_tests'], cwd=ROOT, text=True)
    compiled = [line for line in definitions.splitlines() if '/lib/dolphin/mtx/mtx44.c' in line and ' -c ' in line]
    assert len(compiled) == 1 and '-DTARGET_PC' in compiled[0] and '-DAURORA' in compiled[0]
    link = [line for line in definitions.splitlines() if ' -o tests/mtx44_tests ' in line]
    assert len(link) == 1 and 'libaurora_mtx.a' in link[0]
    sources = ['aurora/xmake.lua', 'aurora/cmake/aurora_mtx.cmake', 'aurora/lib/dolphin/mtx/mtx44.c',
               'aurora/include/dolphin/types.h', 'aurora/tests/CMakeLists.txt', 'aurora/tests/mtx44_test.cpp']
    report = {'results': results, 'cmake_matrix_compile': compiled[0], 'cmake_test_link': link[0],
              'source_sha256': {s: hashlib.sha256((PC / s).read_bytes()).hexdigest() for s in sources},
              'xmake_library_sha256': hashlib.sha256((PC / 'build/macosx/arm64/debug/libaurora-mtx.a').read_bytes()).hexdigest()}
    (HERE / 'matrix-abi-evidence.json').write_text(json.dumps(report, indent=2) + '\n')
    print('PASS real Xmake/CMake native matrix widths; missing-definition compile rejection; all4 real-library matrix tests')


if __name__ == '__main__':
    main()
