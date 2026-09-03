#!/usr/bin/env python3
"""Compile root material-animation source and verify native correspondence."""
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess

NOTE = Path(__file__).resolve().parent
ROOT = NOTE.parents[2]
BUILD = ROOT/'build/original-material-animation-20260903'
spec = importlib.util.spec_from_file_location('model_proof', NOTE.parent/'original-j3d-model-owner-20260903/verify-original.py')
base = importlib.util.module_from_spec(spec)
spec.loader.exec_module(base)
spec = importlib.util.spec_from_file_location('transform_proof', NOTE.parent/'original-j3d-transform-animation-20260903/verify.py')
transform = importlib.util.module_from_spec(spec)
spec.loader.exec_module(transform)

UNITS = {
    'MaterialAnmBuffer': ('src/Game/Animation/MaterialAnmBuffer.cpp', 'J3DModelX'),
    'J3DMaterialAnm': ('src/JSystem/J3DGraphAnimator/J3DMaterialAnm.cpp', 'SDK'),
    'J3DAnimation': ('src/JSystem/J3DGraphAnimator/J3DAnimation.cpp', 'SDK'),
    'JUTNameTab': ('src/JSystem/JUtility/JUTNameTab.cpp', 'SDK'),
}
FAMILIES = ('J3DAnmColor', 'J3DAnmTextureSRTKey', 'J3DAnmTexPattern', 'J3DAnmTevRegKey')


def selected(unit, name):
    if unit == 'MaterialAnmBuffer': return 'MaterialAnmBuffer' in name or 'DiffFlag' in name
    if unit == 'J3DAnimation': return any(f in name for f in FAMILIES)
    if unit == 'J3DMaterialAnm': return 'J3DMaterialAnm' in name
    return 'JUTNameTab' in name


def verify_source():
    result = []
    for source, native in [
        ('src/Game/Animation/MaterialAnmBuffer.cpp', 'Game/Animation/MaterialAnmBuffer.cpp'),
        ('include/Game/Animation/MaterialAnmBuffer.hpp', 'Game/Animation/MaterialAnmBuffer.hpp'),
        ('src/JSystem/J3DGraphAnimator/J3DMaterialAnm.cpp', 'compat/J3DMaterialAnmCompat.cpp'),
        ('src/JSystem/JUtility/JUTNameTab.cpp', 'compat/JUTNameTabCompat.cpp'),
    ]:
        assert (ROOT/source).read_bytes() == (ROOT/'pc-port/src'/native).read_bytes()
        result.append('Byte identity: '+source+' -> '+native)
    original = (ROOT/UNITS['J3DAnimation'][0]).read_text()
    native = (ROOT/'pc-port/src/compat/J3DMaterialAnimationCompat.cpp').read_text()
    methods = re.findall(r'^(?:void )?((?:J3DAnmColor(?:Full|Key)?|J3DAnmTextureSRTKey|J3DAnmTexPattern|J3DAnmTevRegKey)::[^\n{]+)\s*\{', original, re.M)
    for marker in methods:
        expected = transform.normalized(transform.block(original, marker))
        expected = expected.replace('intframe=(int)(mFrame+0.5f);', 'intframe=truncatePpcInteger(mFrame+0.5f);')
        expected = expected.replace('mAnmTable[index].mOffset+(int)mFrame', 'mAnmTable[index].mOffset+truncatePpcInteger(mFrame)')
        expected = expected.replace('mRotData[entryRot->mRotationInfo.mOffset]<<mDecShift',
                                    'shiftPpcRotation(mRotData[entryRot->mRotationInfo.mOffset],mDecShift)')
        expression = 'J3DGetKeyFrameInterpolation(frame,&entryRot->mRotationInfo,&mRotData[entryRot->mRotationInfo.mOffset])'
        expected = expected.replace('(int)'+expression+'<<mDecShift', 'shiftPpcRotation(truncatePpcInteger('+expression+'),mDecShift)')
        assert expected == transform.normalized(transform.block(native, marker)), marker
        result.append('Original method with declared PPC conversion substitutions: '+marker.strip())
    checked, portable = transform.source_correspondence()
    dol = base.DOL.read_bytes()
    graph, _ = transform.retail_graph(base.dol_bytes(dol, 0x804354E8, 0x54))
    assert graph == transform.portable_graph(portable)
    result.append('Existing transform source and shared signed16 Hermite retail dataflow remain verified')
    return result


def main():
    BUILD.mkdir(parents=True, exist_ok=True)
    dol = base.DOL.read_bytes()
    assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
    symbols = {}
    for match in re.finditer(r'^(\S+) = \.text:(0x[\dA-Fa-f]+); // type:function size:(0x[\dA-Fa-f]+)',
                             (ROOT/'config/RMGK01/symbols.txt').read_text(), re.M):
        symbols[match[1]] = (int(match[2], 16), int(match[3], 16))
    evidence = {'dol_sha1': hashlib.sha1(dol).hexdigest(), 'source_sha256': {}, 'functions': [],
                'compiler': 'GC3.0a3 with actual configured Game/SDK flags, no include overlays',
                'source_correspondence': verify_source()}
    commands = {}
    for unit, (source, flag_group) in UNITS.items():
        obj = BUILD/(unit+'.o')
        command = base.compiler(flag_group)+['-c', source, '-o', str(obj)]
        commands[unit] = command
        before = base.sha(ROOT/source)
        p = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        (BUILD/(unit+'-compile.log')).write_text(p.stdout)
        p.check_returncode()
        assert before == base.sha(ROOT/source)
        evidence['source_sha256'][source] = before
        target = base.RET/source[4:].replace('.cpp', '.o')
        out = BUILD/(unit+'-objdiff.json')
        subprocess.run(['build/tools/objdiff-cli', 'diff', '-1', str(target), '-2', str(obj),
                        '-o', str(out), '--format', 'json-pretty'], cwd=ROOT, check=True, capture_output=True)
        diff = json.loads(out.read_text())
        right = {s['name']: s for s in diff['right']['symbols']}
        for original in diff['left']['symbols']:
            name = original['name']
            if name not in symbols or name not in right or not selected(unit, name): continue
            address, size = symbols[name]
            item = {'unit': unit, 'name': name, 'address': hex(address), 'retail_size': size,
                    'compiled_size': int(right[name]['size']), 'objdiff_match_percent': original.get('match_percent'),
                    'retail_sha256': hashlib.sha256(base.dol_bytes(dol, address, size)).hexdigest()}
            evidence['functions'].append(item)
            print(f"{unit}: {name}: {item['objdiff_match_percent']}% ({item['compiled_size']}/{size})")
    for name in ['include/Game/Animation/MaterialAnmBuffer.hpp',
                 'libs/JSystem/include/JSystem/J3DGraphAnimator/J3DAnimation.hpp',
                 'pc-port/src/compat/J3DAnimationInterpolation.hpp']:
        evidence['source_sha256'][name] = base.sha(ROOT/name)
    (BUILD/'compiler-evidence.json').write_text(json.dumps(evidence, indent=2)+'\n')
    (BUILD/'original-commands.json').write_text(json.dumps(commands, indent=2)+'\n')
    (BUILD/'source-correspondence.txt').write_text('\n'.join(evidence['source_correspondence'])+'\n')
    print('All root/native source correspondence and moved transform helpers verified.')


if __name__ == '__main__': main()
