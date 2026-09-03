#!/usr/bin/env python3
"""Stage original DrawBuffer source and compile in isolation; no publication."""
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
PC = ROOT / 'pc-port'
BUILD = ROOT / 'build/original-model-manager-render-20260903'
STAGE = BUILD / 'staged'
NAMES = ['DrawBuffer', 'DrawBufferExecuter', 'DrawBufferGroup', 'DrawBufferHolder']


def main():
    stage = STAGE / 'Game/System'
    stage.mkdir(parents=True, exist_ok=True)
    source_hashes = {}
    for name in NAMES:
        for prefix, suffix in [('src', '.cpp'), ('include', '.hpp')]:
            source = ROOT / prefix / 'Game/System' / (name + suffix)
            target = stage / (name + suffix)
            target.write_bytes(source.read_bytes())
            source_hashes[str(source.relative_to(ROOT))] = hashlib.sha256(source.read_bytes()).hexdigest()
    # The root's recovered MSL algorithm header supplies this pointer visitor.
    # Import it literally at the native standard-library boundary so all four
    # Game sources and headers remain byte-identical to their root originals.
    algorithm = ROOT / 'libs/MSL_C++/include/algorithm'
    algorithm_text = algorithm.read_text()
    begin = algorithm_text.index('    template < class InputIterator, class Function >\n    inline Function for_each_array')
    end = algorithm_text.index('\n    template', begin + 1)
    helper = STAGE / 'MetrowerksDrawAlgorithm.hpp'
    helper.write_text('#pragma once\n\nnamespace std {\n' + algorithm_text[begin:end] + '\n}\n')
    source_hashes[str(algorithm.relative_to(ROOT))] = hashlib.sha256(algorithm.read_bytes()).hexdigest()
    base = next(row['arguments'] for row in json.loads((PC / 'compile_commands.json').read_text())
                if row['file'] == 'src/Game/System/ResourceHolder.cpp')
    args = []
    skip = False
    for arg in base:
        if skip:
            skip = False
        elif arg == '-o':
            skip = True
        elif arg not in ('-c', 'src/Game/System/ResourceHolder.cpp'):
            args.append(arg)
    args[1:1] = ['-I' + str(STAGE), '-I' + str(ROOT / 'build/original-model-manager-native-20260903')]
    args += ['-include', str(helper)]
    commands = [[*args, '-c', str(stage / (name + '.cpp')), '-o', str(BUILD / (name + '.o'))] for name in NAMES]

    def compile_one(item):
        name, command = item
        result = subprocess.run(command, cwd=PC, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        (HERE / (name + '.compile.log')).write_text(result.stdout)
        print(name + ': ' + str(result.returncode), flush=True)
        return {'name': name, 'command': command, 'returncode': result.returncode}

    with ThreadPoolExecutor(max_workers=2) as pool:
        results = list(pool.map(compile_one, zip(NAMES, commands)))
    (HERE / 'draw-native-evidence.json').write_text(json.dumps({'source_sha256': source_hashes, 'results': results}, indent=2) + '\n')
    assert all(row['returncode'] == 0 for row in results), 'See scoped per-source compilation logs'
    nm = '/opt/homebrew/opt/llvm/bin/llvm-nm'
    own_defined = set()
    undefined = set()
    for name in NAMES:
        symbols = subprocess.check_output([nm, '--format=posix', '--extern-only', str(BUILD / (name + '.o'))], text=True)
        for line in symbols.splitlines():
            fields = line.split()
            if len(fields) >= 2:
                (undefined if fields[1] == 'U' else own_defined).add(fields[0])
    providers = {}
    for name in ['smg-pc-game', 'smg-pc-common', 'smg-pc-render']:
        library = PC / 'build/macosx/arm64/debug' / ('lib' + name + '.a')
        symbols = subprocess.check_output([nm, '--defined-only', '--format=posix', '--extern-only', str(library)], text=True)
        for line in symbols.splitlines():
            fields = line.split()
            if len(fields) >= 2:
                providers.setdefault(fields[0], []).append(name)
    external = sorted(undefined - own_defined)
    demangler = '/opt/homebrew/opt/llvm/bin/llvm-cxxfilt'
    readable = subprocess.check_output([demangler, '--strip-underscore', *external], text=True).splitlines()
    rows = [{'symbol': raw, 'readable': pretty, 'provider_archives': providers.get(raw, [])}
            for raw, pretty in zip(external, readable)]
    (HERE / 'draw-symbol-evidence.json').write_text(json.dumps(rows, indent=2) + '\n')
    for row in rows:
        if not row['provider_archives']:
            print('Not supplied by current game/common/render archives: ' + row['readable'])


if __name__ == '__main__':
    main()
