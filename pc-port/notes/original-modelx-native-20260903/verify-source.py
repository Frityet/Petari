#!/usr/bin/env python3
"""Recreate the staged ModelX patch and prove its original-compiler correspondence.

This writes only build/original-modelx-native-proof-20260903. It does not apply
ModelX to the production tree or activate it in the native build.
"""
from pathlib import Path
import hashlib
import importlib.util
import json
import re
import shutil
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BUILD = ROOT / 'build/original-modelx-native-proof-20260903'
PATCHED = BUILD / 'proposed'
BASELINE = BUILD / 'baseline'
spec = importlib.util.spec_from_file_location('maker', HERE.parent / 'original-model-manager-owner-20260903/verify-source.py')
maker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(maker)


def run(command, log):
    result = subprocess.run([str(x) for x in command], cwd=ROOT, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (BUILD / log).write_text(result.stdout)
    result.check_returncode()


def body(text, signature):
    start = text.index(signature)
    at = text.index('{', start)
    depth, end = 1, at + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


def tokens(text):
    return re.sub(r'\s+', '', re.sub(r'//[^\n]*|/\*.*?\*/', '', text, flags=re.S))


def main():
    BUILD.mkdir(parents=True, exist_ok=True)
    manifest = json.loads((HERE / 'modelx-manifest.json').read_text())
    for directory in (PATCHED, BASELINE):
        if directory.exists(): shutil.rmtree(directory)
        directory.mkdir()
    for entry in manifest['files']:
        data = subprocess.check_output(['git', 'show', manifest['baseline_commit'] + ':' + entry['path']], cwd=ROOT) if entry['exists_at_baseline'] else b''
        assert hashlib.sha256(data).hexdigest() == entry['before_sha256'], entry['path']
        if entry['exists_at_baseline']:
            for directory in (PATCHED, BASELINE):
                output = directory / entry['path']
                output.parent.mkdir(parents=True, exist_ok=True)
                output.write_bytes(data)
    run(['git', 'apply', '--directory', PATCHED.relative_to(ROOT), HERE / 'modelx-activation.patch'], 'apply.log')
    for entry in manifest['files']:
        assert hashlib.sha256((PATCHED / entry['path']).read_bytes()).hexdigest() == entry['after_sha256'], entry['path']
    original = PATCHED / 'src/Game/Player/J3DModelX.cpp'
    assert original.read_bytes() == (PATCHED / 'pc-port/src/Game/Player/J3DModelX.cpp').read_bytes()
    assert (PATCHED / 'include/Game/Player/J3DModelX.hpp').read_bytes() == (PATCHED / 'pc-port/src/Game/Player/J3DModelX.hpp').read_bytes()

    # The baseline already had one typed SDK prototype; its stale local
    # redeclaration fails GC3.0a3 before any proposed adaptation can be tested.
    before = (BASELINE / 'src/Game/Player/J3DModelX.cpp').read_text()
    before = before.replace('void GDSetTexCoordGen(int, int, int, int, int);\n', '')
    before = before.replace('GDSetTexCoordGen(0, 1, 1, 1, 0x3C);', 'GDSetTexCoordGen(static_cast< GXTexCoordID >(0), static_cast< GXTexGenType >(1), static_cast< GXTexGenSrc >(1), 1, 0x3C);')
    (BASELINE / 'src/Game/Player/J3DModelX.cpp').write_text(before)
    options = ['-nodefaults', '-proc', 'gekko', '-align', 'powerpc', '-enum', 'int', '-fp', 'hardware', '-Cpp_exceptions', 'off', '-O4,s', '-inline', 'auto', '-pragma', 'cats off', '-pragma', 'warn_notinlined off', '-maxerrors', '1', '-nosyspath', '-RTTI', 'off', '-str', 'reuse', '-enc', 'SJIS', '-sdata', '4', '-sdata2', '4', '-ipa', 'file', '-sym', 'on']
    includes = ['include', 'libs/JSystem/include', 'libs/MSL_C++/include', 'libs/MSL_C/include', 'libs/MetroTRK/include', 'libs/RVLFaceLib/include', 'libs/RVL_SDK/include', 'libs/Runtime/include', 'libs/nw4r/include', 'build/RMGK01/include']
    for label, directory in [('baseline', BASELINE), ('proposed', PATCHED)]:
        command = ['build/tools/wibo', 'build/tools/sjiswrap.exe', 'build/compilers/GC/3.0a3/mwcceppc.exe', *options,
                   '-i', directory / 'include', '-i', directory / 'libs/RVL_SDK/include']
        for include in includes: command += ['-i', include]
        command += ['-DVERSION=0', '-c', directory / 'src/Game/Player/J3DModelX.cpp', '-o', BUILD / (label + '.o')]
        run(command, label + '.compile.log')
    before, after = (maker.reader.Elf(BUILD / (name + '.o')) for name in ('baseline', 'proposed'))
    records = []
    def references(obj, name):
        refs = obj.references(name)
        for ref in refs:
            if 'value_hex' in ref and ref['symbol'].startswith('@'):
                ref['symbol'] = 'constant_' + ref['value_hex']
        return refs
    for name, start, size, section in before.symbols:
        if not size or name.startswith('.') or not ('__9J3DModelX' in name or '__13J3DMtxBuffer2' in name): continue
        _, other_start, other_size, other_section = next(x for x in after.symbols if x[0] == name)
        a = before.section_data(section)[start:start + size]
        b = after.section_data(other_section)[other_start:other_start + other_size]
        assert a == b, name
        assert references(before, name) == references(after, name), name
        records.append({'name': name, 'bytes': size, 'references': len(references(after, name))})
    assert len(records) == 21

    copies = [
        ('src/Game/Player/MarioActorDraw.cpp', 'J3DModelXDrawCompat.cpp', ['void J3DModelX::copyExtraMtxBuffer(', 'void J3DModelX::swapDrawBuffer(', 'void J3DModelX::setDynamicDL(']),
        ('src/Game/Util/ModelUtil.cpp', 'ModelCreationCompat.cpp', ['void invalidateMtxCalc(', 'void invalidateJointCallback(', 'J3DModel* newJ3DModel(', 'XanimePlayer* newXanimePlayer(', 'XanimeResourceTable* newXanimeResourceTable(']),
        ('src/Game/Util/DirectDraw.cpp', 'ModelFogCompat.cpp', ['void mixFogColor(', 'void setGXColor(']),
        ('src/Game/Util/ModelUtil.cpp', 'ModelFogCompat.cpp', ['void calcFogStartEnd(']),
    ]
    for source, native, signatures in copies:
        a = (ROOT / source).read_text()
        b = (PATCHED / 'pc-port/src/compat' / native).read_text()
        for signature in signatures: assert tokens(body(a, signature)) == tokens(body(b, signature)), signature

    dol = (ROOT / 'build/compat-math-oracle/main.dol').read_bytes()
    assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
    # Preserve the unusual nonnull viewCalc3 path: its incoming pointer is
    # stored in a stack slot, and the address of that slot reaches calcDrawMtx.
    assert maker.reader.dol_bytes(dol, 0x802A6B88, 4).hex() == '90a10008'
    assert maker.reader.dol_bytes(dol, 0x802A6BCC, 4).hex() == '38c10008'
    result = {'compiler': 'GC3.0a3', 'baseline_commit': manifest['baseline_commit'],
              'modelx_objects': records, 'literal_extracted_helpers': 11,
              'retail_viewCalc3_stack_pointer_path': ['0x802A6B88: stw r5,8(r1)', '0x802A6BCC: addi r6,r1,8']}
    (HERE / 'source-evidence.json').write_text(json.dumps(result, indent=2) + '\n')
    print('PASS: 20 functions + vtable,', sum(x['bytes'] for x in records), 'unchanged bytes;', sum(x['references'] for x in records), 'unchanged references; 11 literal helpers; retail viewCalc3 pointer path')

if __name__ == '__main__': main()
