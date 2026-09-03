#!/usr/bin/env python3
"""Isolated real-disc complete-model ASan/UBSan against frozen native archives."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
PC = ROOT / 'pc-port'
BUILD = ROOT / 'build/original-j3d-model-resource-20260903/sanitizer'
CACHE = PC / 'build/.deps'
TARGET = 'smg-pc-original-j3d-model-resource-tests'
FLAGS = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
SOURCES = ['src/resource/' + name + '.cpp' for name in (
    'J3dModelResource', 'J3dJointData', 'J3dGeometryData', 'J3dMaterialTableData',
    'J3dMaterialBlockData', 'J3dTextureData', 'J3dNameData', 'J3dAllocationIdentity',
    'Mem1ResourceHeap', 'TplTexture')]
SOURCES += ['src/compat/' + name + '.cpp' for name in (
    'J3DModelLoaderCompat', 'J3DHierarchyCompat', 'J3DModelDataCompat', 'J3DJointTreeCompat',
    'J3DJointCompat', 'J3DShapeTableCompat', 'J3DShapeFactoryCompat', 'J3DShapeCompat',
    'J3DShapeDrawCompat', 'J3DShapeMtxCompat', 'J3DShapeMtxGameCompat', 'J3DMaterialLoaderFactoryCompat',
    'J3DMaterialFactoryCompat', 'J3DMaterialVariantsCompat', 'J3DMaterialHelpersCompat',
    'J3DMatBlockCompat', 'J3DTevsCompat', 'J3DTexMtxCompat', 'J3DTextureMtxCompat', 'J3DStructCompat',
    'J3DGDCompat', 'J3DSysCompat', 'J3DMtxBufferCompat', 'J3DTransformMtxCompat',
    'J3DVertexBufferCompat', 'J3DPacketCompat', 'J3DDrawBufferCompat', 'J3DJointEntryCompat',
    'JUTNameTabCompat', 'JKRExpHeapCompat', 'MemoryHeapScopeCompat', 'JkrAllocationDomain',
    'JkrAllocationProvenance', 'MetrowerksAlignedNew', 'JKRHeapCompat', 'JKRSolidHeapCompat',
    'JKRDisposerCompat', 'JSUListCompat', 'JkrDiagnostics')]
SOURCES += ['aurora/lib/dolphin/os/' + name + '.cpp' for name in (
    'OSExecution', 'OSMutex', 'OSReport', 'OSAlloc', 'OSArena', 'OSMemory', 'OSAddress', 'OSInit')]
SOURCES += ['tests/OriginalJ3DModelResourceTests.cpp']


def strings(text):
    return [json.loads(s) for s in re.findall(r'"(?:\\.|[^"\\])*"', text)]


def arguments(path):
    return strings(re.search(r'values = \{(.*?)\n    \}', path.read_text(), re.S).group(1))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, log, timeout=120):
    with log.open('w') as output:
        result = subprocess.run(command, cwd=PC, stdout=output, stderr=subprocess.STDOUT, timeout=timeout)
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--jobs', type=int, default=4)
    options = parser.parse_args()
    if options.jobs < 1 or options.jobs > 8:
        parser.error('--jobs must be between 1 and 8')
    disc = Path(os.environ.get('SMGPC_REAL_DISC', ROOT / 'Super Mario Wii - Galaxy Adventure (Korea).rvz'))
    if not disc.is_file():
        parser.error('Set SMGPC_REAL_DISC to the actual retained disc image')
    BUILD.mkdir(parents=True, exist_ok=True)
    compiled_sources = {source: sha(PC / source) for source in SOURCES}
    headers, cache_hashes, commands = {}, {}, []
    objects, compile_jobs = [], []
    for source in SOURCES:
        matches = sorted(CACHE.glob('*/macosx/arm64/debug/' + source + '.o.d'))
        if not matches:
            raise RuntimeError('Frozen native compile flags are unavailable for ' + source)
        preferred = [path for path in matches if path.parts[len(CACHE.parts)] == ('smg-pc-game' if source.startswith('src/') else TARGET)]
        cache = (preferred or matches)[0]
        cache_hashes[str(cache.relative_to(PC))] = sha(cache)
        cache_text = cache.read_text()
        files_match = re.search(r'files = \{(.*?)\n    \}', cache_text, re.S)
        dependencies = strings(files_match.group(1))
        depfiles = re.search(r'depfiles = "(.*?)",\n', cache_text, re.S)
        if depfiles:
            dependency_text = depfiles.group(1).replace('\\\n', '')
            dependencies += shlex.split(dependency_text.split(':', 1)[1])
        for item in dependencies:
            path = (PC / item).resolve()
            if path.is_relative_to(PC / 'src') or path.is_relative_to(PC / 'aurora/include') or path.is_relative_to(PC / 'aurora/lib'):
                if path.is_file():
                    headers[str(path.relative_to(PC))] = sha(path)
        output = BUILD / (source.replace('/', '_') + '.asan.o')
        command = arguments(cache) + FLAGS + ['-c', source, '-o', str(output)]
        commands.append(command)
        compile_jobs.append((command, BUILD / (Path(source).stem + '.compile.log')))
        objects.append(str(output))
    link_cache = CACHE / TARGET / 'macosx/arm64/debug' / (TARGET + '.d')
    link_args = arguments(link_cache)
    cache_hashes[str(link_cache.relative_to(PC))] = sha(link_cache)
    archive_paths = [PC / value for value in strings(re.search(r'files = \{(.*?)\n    \}', link_cache.read_text(), re.S).group(1)) if value.endswith('.a')]
    archives = {str(p.relative_to(PC)): sha(p) for p in archive_paths}
    compat = PC / 'build/.objs' / TARGET / 'macosx/arm64/debug/aurora/lib/compat.cpp.o'
    compat_hash = sha(compat)
    binary = BUILD / 'complete-model-tests-asan'
    link = [link_args[0], *FLAGS, *objects, str(compat), *link_args[1:], '-o', str(binary)]
    commands.append(link)
    (BUILD / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n')
    print(f'Compiling {len(compile_jobs)} isolated model/SDK/heap objects with ASan/UBSan', flush=True)
    with ThreadPoolExecutor(max_workers=options.jobs) as pool:
        results = list(pool.map(lambda job: run(*job), compile_jobs))
    failed = False
    for result, (_, log) in zip(results, compile_jobs):
        if result.returncode:
            print(log.read_text(), end='')
            failed = True
    if failed:
        raise RuntimeError('Isolated sanitizer compilation failed; see per-source logs')
    result = run(link, BUILD / 'link.log')
    if result.returncode:
        print((BUILD / 'link.log').read_text(), end='')
        result.check_returncode()

    def verify_frozen():
        for path, expected in {**compiled_sources, **headers, **archives, **cache_hashes}.items():
            assert sha(PC / path) == expected, 'Source/header/archive/cache changed during validation: ' + path
        assert sha(compat) == compat_hash

    verify_frozen()
    env = dict(os.environ, SMGPC_REAL_DISC=str(disc), ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    result = subprocess.run([str(binary)], cwd=PC, env=env, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True, timeout=120)
    (HERE / 'native-asan.log').write_text(result.stdout)
    print(result.stdout, end='')
    verify_frozen()
    report = {
        'scope': 'Isolated complete-model CPU system test with the real retained Mario BDL; no shared xmake or GPU run.',
        'instrumentation': 'Listed resource owners/decoders, original factories/finalizers/material blocks/matrix traversal, JKR heap and OS allocation/lifecycle providers and tests are ASan/UBSan instrumented. Other SDK, render/runtime, DVD/archive/disc decoding, GX/FIFO, math, standard/package libraries and retained Aurora compat object are frozen uninstrumented dependencies. This is not whole-program instrumentation.',
        'environment': {key: env[key] for key in ('SMGPC_REAL_DISC','ASAN_OPTIONS','UBSAN_OPTIONS')},
        'return_code': result.returncode,
        'group_count': 4,
        'actual_mario_resource': '[resource] complete mario.bdl flags=' in result.stdout,
        'actual_mario_flags': re.findall(r'\[resource\] complete mario\.bdl flags=([0-9a-f]+)', result.stdout),
        'binary_sha256': sha(binary),
        'instrumented_source_sha256': compiled_sources,
        'included_workspace_file_sha256': headers,
        'frozen_archive_sha256': archives,
        'frozen_compat_object_sha256': compat_hash,
        'frozen_build_flag_cache_sha256': cache_hashes,
    }
    (HERE / 'native-asan-evidence.json').write_text(json.dumps(report, indent=2) + '\n')
    result.check_returncode()
    assert '[pass] 4 original complete-model resource groups' in result.stdout
    assert report['actual_mario_flags'] == ['1200000','1201000','1202000']
    print('[pass] frozen complete-model source/header/archive hashes and real-disc sanitizer result')


if __name__ == '__main__':
    main()
