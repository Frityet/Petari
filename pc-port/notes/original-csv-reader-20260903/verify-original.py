#!/usr/bin/env python3
"""Verify the recovered scalar CSV readers and complete imported helper slice."""
from pathlib import Path
import hashlib
import importlib.util
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-csv-reader-20260903'
PROOF = ROOT / 'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py'
spec = importlib.util.spec_from_file_location('original_object', PROOF)
proof = importlib.util.module_from_spec(spec)
spec.loader.exec_module(proof)
BUILD.mkdir(parents=True, exist_ok=True)
compiled = BUILD / 'ObjUtil.o'
command = proof.compiler('cflags_game') + ['-c', 'src/Game/Util/ObjUtil.cpp', '-o', str(compiled)]
result = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
(BUILD / 'root-compile.log').write_text(result.stdout)
result.check_returncode()
# Reuse an already split original object; the DOL identity is checked below.
retail = ROOT / 'build/xanime-core-pose-blending-restoration-20260903/retail/obj/Game/Util/ObjUtil.o'
if not retail.exists():
    raise SystemExit('Run the original Xanime core proof to produce the RMGK01 retail split object first.')
subprocess.run([str(ROOT/'build/tools/objdiff-cli'), 'diff', '-1', str(retail), '-2', str(compiled),
                '-o', str(BUILD/'objdiff.json'), '--format', 'json-pretty'], cwd=ROOT, check=True)
diff = json.loads((BUILD/'objdiff.json').read_text())
selected = [s for s in diff['left']['symbols'] if any(k in s['name'] for k in
            ('isExistFileInArc', 'createCsvParser', 'tryCreateCsvParser', 'getCsvData'))
            and 'getCsvDataColor' not in s['name']]
assert len(selected) == 15
assert all(s['match_percent'] == 100 for s in selected if 'getCsvDataVec' not in s['name'])
objects = [proof.Elf(retail), proof.Elf(compiled)]
checks = []
dol = proof.DOL.read_bytes()
assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
for symbol, address, size in [('getCsvDataF32__2MRFPfPC8JMapInfoPCcl', 0x803F1B88, 120),
                             ('getCsvDataBool__2MRFPbPC8JMapInfoPCcl', 0x803F1C00, 136)]:
    actual = [obj.function(symbol) for obj in objects]
    assert actual[0] == actual[1] and len(actual[0][0]) == size
    checks.append(dict(symbol=symbol, address=hex(address), size=size,
                       relocation_normalized_bytes_equal=True, relocations=actual[0][1],
                       retail_sha256=hashlib.sha256(proof.dol_bytes(dol,address,size)).hexdigest()))
def vector_code(elf):
    import struct
    name = 'getCsvDataVec__2MRFP3VecPC8JMapInfoPCcl'
    _, begin, size, section_index = next(x for x in elf.symbols if x[0] == name)
    code = bytearray(elf.section_data(section_index)[begin:begin+size])
    references = []
    for section in elf.sections:
        if section[1] != 4 or section[7] != section_index: continue
        for offset in range(section[4],section[4]+section[5],section[9]):
            at,info,addend = struct.unpack_from('>IIi',elf.data,offset)
            if not begin <= at < begin+size: continue
            symbol, value, _, symbol_section = elf.symbols[info >> 8]
            kind = info & 255
            word = struct.unpack_from('>I',code,at-begin)[0]
            if kind == 10:
                word &= 0xfc000003
                target = symbol
            elif kind == 109:
                raw = (proof.dol_bytes(dol,int(symbol.removeprefix('lbl_'),16)+addend,4)
                       if symbol_section == 0 else elf.section_data(symbol_section)[value+addend:])
                target = raw[:raw.index(0)+1].decode('ascii')
                assert target in ('%sX\0','%sY\0','%sZ\0')
                word &= 0xffe00000
                addend = 0
            else: raise AssertionError(kind)
            struct.pack_into('>I',code,at-begin,word)
            references.append(dict(offset=at-begin,kind=kind,target=target,addend=addend))
    return bytes(code),references
vectors = [vector_code(e) for e in objects]
assert vectors[0] == vectors[1] and len(vectors[0][0]) == 192
assert proof.dol_bytes(dol,0x806B2680,12) == b'%sX\0%sY\0%sZ\0'
checks.append(dict(symbol='getCsvDataVec__2MRFP3VecPC8JMapInfoPCcl',address='0x803f1c88',size=192,
                   relocation_normalized_bytes_equal=True,relocations=vectors[0][1],
                   string_data_address='0x806b2680',string_data_hex='257358002573590025735a00'))
source = (ROOT/'src/Game/Util/ObjUtil.cpp').read_text()
anon = source[source.index('namespace {'):source.index('namespace MR {')]
slice = source[source.index('    bool isExistFileInArc(const ResourceHolder*'):source.index('    void getCsvDataColor')]
start = slice.index('    JMapInfo* createCsvParser(const char*')
end = slice.index('    JMapInfo* tryCreateCsvParser(const LiveActor*', start)
slice = slice[:start] + slice[end:]
native = (ROOT/'pc-port/src/compat/OriginalCsvReader.cpp').read_text()
assert native[native.index('namespace {'):] == anon + 'namespace MR {\n' + slice + '}  // namespace MR\n'
header = ROOT/'pc-port/src/Game/LiveActor/ModelManager.hpp'
assert header.read_bytes() == (ROOT/'include/Game/LiveActor/ModelManager.hpp').read_bytes()
manager = (ROOT/'src/Game/LiveActor/ModelManager.cpp').read_text()
actor = (ROOT/'src/Game/Util/LiveActorUtil.cpp').read_text()
resource = (ROOT/'pc-port/src/compat/OriginalActorResource.cpp').read_text()
method = manager[manager.index('ResourceHolder* ModelManager::getResourceHolder()'):manager.index('ResourceHolder* ModelManager::getModelResourceHolder()')]
actor_method = actor[actor.index('    ResourceHolder* getResourceHolder(const LiveActor*'):actor.index('    ResourceHolder* getModelResourceHolder(')]
assert resource[resource.index('ResourceHolder* ModelManager::getResourceHolder()'):] == method + 'namespace MR {\n' + actor_method + '}  // namespace MR\n'
evidence = dict(dol_sha1=hashlib.sha1(dol).hexdigest(),
                root_source_sha256=hashlib.sha256(source.encode()).hexdigest(),
                imported_native_slice_sha256=hashlib.sha256(native.encode()).hexdigest(),
                root_compile_command=command, recovered_numeric=checks,
                original_helpers=[dict(symbol=s['name'],size=s['size'],objdiff_percent=s['match_percent']) for s in selected],
                native_literal_helpers=14, actual_actor_resource_methods=2,
                native_actor_resource_sha256=hashlib.sha256(resource.encode()).hexdigest(),
                root_identical_model_manager_header_sha256=hashlib.sha256(header.read_bytes()).hexdigest(),
                inactive_archive_name_overload='100% original match; depends on inactive original ResourceHolderManager singleton')
(NOTES/'source-evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
print('PASS: 14 exact original matches, vector instruction/constant correspondence, 14 literal native helpers')
