#!/usr/bin/env python3
"""Original loader recovery proof and native animation ownership regression."""
import ast
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import types

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BUILD = ROOT / 'build/original-resource-holder-20260903'
DOL = ROOT / 'build/compat-math-oracle/main.dol'
BUILD.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location('proof', HERE.parent / 'original-binder-reaction-20260903/verify-runtime.py')
proof = importlib.util.module_from_spec(spec)
spec.loader.exec_module(proof)
reader = proof.reaction.reader

def run(cmd, log):
    result = subprocess.run([str(x) for x in cmd], cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            env=dict(os.environ, ASAN_OPTIONS='halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1'))
    (HERE / log).write_text(result.stdout)
    result.check_returncode()
    return result.stdout

raw = DOL.read_bytes()
assert hashlib.sha1(raw).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
for node in ast.parse((ROOT / 'configure.py').read_text()).body:
    if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'cflags_jsys' for t in node.targets):
        flags = eval(compile(ast.Expression(node.value), 'configure.py', 'eval'),
                     {'config': types.SimpleNamespace(version='RMGK01'), 'version_num': 0})
        break
base = ['build/tools/wibo', 'build/tools/sjiswrap.exe', 'build/compilers/GC/3.0a3/mwcceppc.exe']
for flag in flags: base.extend(shlex.split(flag))
symbols = {}
for match in re.finditer(r'^(\S+) = \.text:(0x[\dA-Fa-f]+); // type:function size:(0x[\dA-Fa-f]+)',
                         (ROOT / 'config/RMGK01/symbols.txt').read_text(), re.M):
    symbols[match[1]] = (int(match[2], 16), int(match[3], 16))
units = [('J3DAnmLoader', 'J3DGraphLoader'), ('J3DAnimation', 'J3DGraphAnimator')]
records, exact, commands = [], [], {}
for unit, directory in units:
    source = f'src/JSystem/{directory}/{unit}.cpp'
    obj = BUILD / (unit + '.o')
    command = base + ['-c', source, '-o', obj]
    commands[unit] = [str(x) for x in command]
    run(command, unit + '-compile.log')
    target_path = BUILD / f'retail/obj/JSystem/{directory}/{unit}.o'
    if not target_path.exists(): raise RuntimeError('Run original-resource-holder verify-archive.py to prepare dtk objects first')
    diff_path = BUILD / (unit + '-loader-proof.json')
    run(['build/tools/objdiff-cli', 'diff', '-1', target_path, '-2', obj, '-o', diff_path, '--format', 'json-pretty'], unit + '-objdiff.log')
    diff = json.loads(diff_path.read_text())
    target, compiled = reader.Elf(target_path), reader.Elf(obj)
    for item in diff['left']['symbols']:
        name = item['name']
        if item.get('match_percent') is None or name not in symbols: continue
        if unit == 'J3DAnimation' and not any(x in name for x in ['getVisibility__', 'J3DAnmVtxColor', 'getWeight__']): continue
        if unit == 'J3DAnmLoader' and not any(x in name for x in ['load__', 'setResource__', 'setAnm']): continue
        address, size = symbols[name]
        record = {'name': name, 'address': hex(address), 'retail_size': size, 'objdiff_match_percent': item['match_percent']}
        if item['match_percent'] == 100:
            code, refs = proof.relocated(compiled, target, name, address, size, raw)
            assert code == reader.dol_bytes(raw, address, size), name
            record['retail_bytes_equal_after_relocation'] = True
            record['references'] = refs
            exact.append(name)
        records.append(record)
    print(unit, 'original compiler comparison complete')
for name in ['load__20J3DAnmFullLoader_v15FPCv','load__19J3DAnmKeyLoader_v15FPCv',
             'setResource__20J3DAnmFullLoader_v15FP10J3DAnmBasePCv','setResource__19J3DAnmKeyLoader_v15FP10J3DAnmBasePCv',
             'getVisibility__20J3DAnmVisibilityFullCFUsPUc']:
    assert name in exact

original = (ROOT / 'src/JSystem/J3DGraphLoader/J3DAnmLoader.cpp').read_text()
expected = original.replace('#include "JSystem/JSupport/JSupport.hpp"', '#include "JSystem/JSupport/JSupport.hpp"\n#include "resource/J3dAnimationResource.hpp"\n#include <cstdint>')
expected = expected.replace('J3DAnmBase* J3DAnmLoaderDataBase::load(', 'J3DAnmBase* smgpc::resource::detail::load_native_animation(', 1)
expected = expected.replace('const JUTDataBlockHeader* block = &header->mFirstBlock;', 'const JUTDataBlockHeader* block = smgpc::resource::detail::first_animation_block(header);')
expected = expected.replace('block = block->getNext();', 'block = smgpc::resource::detail::next_animation_block(header, block);')
for index in (0, 1):
    before = f'(void*)((s32)indexPtr{index} + (s32)dst->mAnmVtxColorIndexData[{index}][i].mpData * 2)'
    after = f'(void*)((std::uintptr_t)indexPtr{index} + (std::uintptr_t)dst->mAnmVtxColorIndexData[{index}][i].mpData * 2)'
    assert expected.count(before) == 2
    expected = expected.replace(before, after)
expected += '\nJ3DAnmBase* J3DAnmLoaderDataBase::load(const void* data, J3DAnmLoaderDataBaseFlag flag) {\n    return smgpc::resource::load_registered_j3d_animation(data, flag);\n}\n'
assert expected == (ROOT / 'pc-port/src/compat/J3DAnmLoaderCompat.cpp').read_text()
assert (ROOT / 'libs/JSystem/include/JSystem/J3DGraphLoader/J3DAnmLoader.hpp').read_bytes() == (ROOT / 'pc-port/src/JSystem/J3DGraphLoader/J3DAnmLoader.hpp').read_bytes()
original = (ROOT / 'src/JSystem/J3DGraphAnimator/J3DAnimation.cpp').read_text()
a = original.index('void J3DAnmVisibilityFull::getVisibility(')
b = original.index('J3DAnmColor::J3DAnmColor()', a)
expected = original[a:b].replace('(int)(0.5f + mFrame)', 'truncatePpcInteger(0.5f + mFrame)').replace('(int)(mFrame + 0.5f)', 'truncatePpcInteger(mFrame + 0.5f)')
native = (ROOT / 'pc-port/src/compat/J3DAdditionalAnimationCompat.cpp').read_text()
assert native[native.index('void J3DAnmVisibilityFull::getVisibility('):] == expected

sources = ['pc-port/tests/OriginalJ3DAnimationResourceTests.cpp', 'pc-port/src/resource/J3dAnimationResource.cpp',
           'pc-port/src/resource/J3dNameData.cpp', 'pc-port/src/resource/J3dTransformAnimation.cpp',
           'pc-port/src/compat/J3DAnmLoaderCompat.cpp', 'pc-port/src/compat/J3DAdditionalAnimationCompat.cpp',
           'pc-port/src/compat/J3DTransformAnimationCompat.cpp', 'pc-port/src/compat/J3DMaterialAnimationCompat.cpp',
           'pc-port/src/compat/JUTNameTabCompat.cpp', 'pc-port/src/resource/RarcArchive.cpp', 'pc-port/src/resource/Yaz0.cpp']
heap_sources = [f'pc-port/src/compat/{name}.cpp' for name in (
    'JKRExpHeapCompat', 'MemoryHeapScopeCompat', 'JkrAllocationDomain', 'JkrAllocationProvenance',
    'MetrowerksAlignedNew', 'JKRHeapCompat', 'JKRSolidHeapCompat', 'JKRDisposerCompat', 'JSUListCompat', 'JkrDiagnostics')]
heap_sources += [f'pc-port/aurora/lib/dolphin/os/{name}.cpp' for name in ('OSExecution', 'OSMutex', 'OSReport')]
compiler = '/opt/homebrew/opt/llvm/bin/clang++'
common = [compiler, '-std=c++23', '-O1', '-g', '-DTARGET_PC', '-DAURORA', '-Wl,-dead_strip',
          '-Ipc-port/src', '-Ipc-port/aurora/include', '-Ipc-port/aurora/lib/dolphin',
          '-Wno-multichar', '-Wno-inconsistent-missing-override', '-Wno-macro-redefined',
          '-fno-builtin-sprintf', '-fno-builtin-snprintf', '-fno-builtin-vsprintf', '-fno-builtin-vsnprintf']
tests = {}
for name, flags in [('normal', []), ('address-undefined', ['-fsanitize=address,undefined']), ('thread', ['-fsanitize=thread'])]:
    binary = BUILD / ('animation-tests-' + name)
    # This TU deliberately calls host snprintf for numeric conversion; never
    # force-include the Game alias header while compiling the provider itself.
    msl = BUILD / ('MslPrintf-animation-' + name + '.o')
    run([compiler, '-std=c++23', '-O1', '-g', '-Ipc-port/src', *flags, '-c',
         'pc-port/src/compat/MslPrintfCompat.cpp', '-o', msl], name + '-msl-compile.log')
    run(common + flags + ['-include', 'pc-port/src/compat/MetrowerksStdCompat.hpp',
         *sources, *heap_sources, msl, '-pthread', '-o', binary], name + '-compile.log')
    log = run([binary], name + '.log')
    cases = [line.removeprefix('PASS ') for line in log.splitlines() if line.startswith('PASS ')]
    assert len(cases) == 14 and 'runtime error:' not in log and 'WARNING: ThreadSanitizer' not in log
    tests[name] = {'cases': cases, 'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest()}
    print(name, '14/14 passed', flush=True)
archive_path = ROOT / 'build/original-resource-holder-20260903/MarioAnime.arc'
real_archive = None
if archive_path.exists():
    log = run([BUILD / 'animation-tests-address-undefined', archive_path], 'real-MarioAnime.log')
    families = {line.split()[1]: int(line.split()[2]) for line in log.splitlines() if line.startswith('ARCHIVE ')}
    assert families == {'bck1':456, 'btk1':4, 'btp1':71}
    real_archive = {'archive_sha256':hashlib.sha256(archive_path.read_bytes()).hexdigest(), 'families':families,
                    'total':sum(families.values()), 'sanitizers':'address,undefined'}
    print('real MarioAnime', sum(families.values()), 'animations passed')
sources += heap_sources + ['pc-port/src/compat/MslPrintfCompat.cpp',
            'pc-port/src/compat/JkrAllocationDomain.hpp', 'pc-port/src/resource/J3dAnimationResource.hpp', 'pc-port/src/resource/J3dNativeBlock.hpp',
            'libs/JSystem/include/JSystem/J3DGraphLoader/J3DAnmLoader.hpp',
            'src/JSystem/J3DGraphLoader/J3DAnmLoader.cpp', 'src/JSystem/J3DGraphAnimator/J3DAnimation.cpp']
(HERE / 'evidence.json').write_text(json.dumps({'compiler_commands': commands, 'original_functions': records,
    'native_source_correspondence': 'Exact original methods with enumerated pointer-traversal/ownership/PPC-conversion adaptations only',
    'native_tests': tests, 'real_archive':real_archive, 'source_sha256': {p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in sources},
    'scope': 'Actual original animation loader dispatch/sampling and retained decoded data. Actual JKR domain allocation and retention are verified; ResourceHolder activation remains separate.'}, indent=2) + '\n')
