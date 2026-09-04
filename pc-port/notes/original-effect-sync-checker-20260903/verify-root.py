#!/usr/bin/env python3
from pathlib import Path
import hashlib, importlib.util, json, re, struct, subprocess
ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-effect-sync-checker-20260903'
BUILD.mkdir(parents=True, exist_ok=True)
def module(name, path):
    spec = importlib.util.spec_from_file_location(name, ROOT / path)
    result = importlib.util.module_from_spec(spec); spec.loader.exec_module(result); return result
c = module('compiler', 'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
reader = module('reader', 'pc-port/notes/mario-update-restoration-20260903/verify-object.py')
proof = module('proof', 'pc-port/notes/original-binder-reaction-20260903/verify-runtime.py')
rel = module('rel', 'pc-port/notes/original-stage-data-holder-20260903/verify-original.py')
addresses = {n: int(a, 16) for n, a in re.findall(r'^([^\n]+?) = \.\w+:(0x[0-9A-Fa-f]+);', (ROOT / 'config/RMGK01/symbols.txt').read_text(), re.M)}
dol = c.DOL.read_bytes()
assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
class Elf(reader.Elf):
    def code(self, name):
        _, start, size, section = next(s for s in self.symbols if s[0] == name)
        return bytearray(self.section_data(section)[start:start + size])
    def references(self, name):
        refs = super().references(name)
        for r in refs:
            if r['symbol'] == 'lbl_80531BD0':
                value = reader.dol_bytes(dol, addresses[r['symbol']], 8)
                assert value.hex() == '4330000080000000'; r['value_hex'] = value.hex()
            elif r['symbol'] == 'cAttributeEffectTag__26@unnamed@EffectKeeper_cpp@':
                nested = super().references(r['symbol'])
                assert len(nested) == 1 and nested[0]['kind'] == 1 and nested[0]['value_hex'] == '4174747200'
                value = reader.dol_bytes(dol, addresses[r['symbol']], 4)
                pointer = struct.unpack('>I', value)[0]
                assert reader.dol_bytes(dol, pointer, 5) == b'Attr\0'
                r['value_hex'] = value.hex()
            else:
                symbol = next(s for s in self.symbols if s[0] == r['symbol'])
                _, start, size, section = symbol
                if size != 12 or section == 0: continue
                nested = super().references(r['symbol'])
                if not nested: continue
                assert len(nested) == 1 and nested[0]['kind'] == 1 and nested[0]['offset'] == '0x8'
                value = bytearray(self.section_data(section)[start:start + size])
                assert value[:8].hex() == '00000000ffffffff'
                struct.pack_into('>I', value, 8, addresses[nested[0]['symbol']] + nested[0]['addend'])
                r['value_hex'] = value.hex()
        return refs

def compile(source, output):
    command = c.compiler('cflags_game') + ['-c', str(source), '-o', str(output)]
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    output.with_suffix('.compile.log').write_text(result.stdout + result.stderr)
    result.check_returncode(); return command

rows, commands, preservation = [], [], []
for unit, category in [('SyncBckEffectChecker', 'Effect'), ('MultiEmitter', 'Effect'), ('EffectKeeper', 'LiveActor')]:
    source = ROOT / f'src/Game/{category}/{unit}.cpp'; obj = BUILD / f'{unit}.o'
    commands.append(compile(source, obj))
    retail = ROOT / f'build/xanime-core-pose-blending-restoration-20260903/retail/obj/Game/{category}/{unit}.o'
    output = BUILD / f'{unit}-diff.json'
    subprocess.run([ROOT / 'build/tools/objdiff-cli', 'diff', '-1', retail, '-2', obj, '-o', output, '--format', 'json-pretty'], check=True)
    a, b = Elf(retail), Elf(obj); diff = json.loads(output.read_text())
    for name, start, size, section in a.symbols:
        if not size or section == 0 or not a.sections[section][2] & 4: continue
        if unit == 'MultiEmitter' and not name.startswith('setDrawOrder'): continue
        if unit == 'EffectKeeper' and not name.startswith('syncEffectBck'): continue
        rel.relocate(a, a, name, addresses[name], dol)
        exact = not name.startswith('updateAfter')
        if exact:
            refs = rel.relocate(b, a, name, addresses[name], dol)
        else:
            code, refs = proof.relocated(b, a, name, addresses[name], size, dol)
            original = reader.dol_bytes(dol, addresses[name], size)
            # Same nine instructions/data flow; only independent copied-name
            # and zero temporaries use different registers before r4 is reused.
            expected = {0: (0x80a3000c, 0x8003000c), 4: (0x38000000, 0x38800000),
                        8: (0x98030008, 0x98830008), 16: (0x90a30010, 0x90030010)}
            found = {i: (struct.unpack_from('>I', original, i)[0], struct.unpack_from('>I', code, i)[0])
                     for i in range(0, size, 4) if original[i:i+4] != code[i:i+4]}
            assert found == expected
        score = next(s['match_percent'] for s in diff['left']['symbols'] if s['name'] == name)
        rows.append({'name': name, 'address': hex(addresses[name]), 'size': size, 'objdiff_percent': score,
                     'relocated_bytes_equal': exact, 'relocations': refs,
                     'checked_temporary_register_difference': None if exact else expected})
        print(name, score, size, 'exact=' + str(exact))
    before_obj = BUILD / f'{unit}-before.o'
    commands.append(compile(NOTES / f'baseline/{unit}.cpp', before_obj))
    before = reader.Elf(before_obj); after = reader.Elf(obj)
    def identity(elf, name):
        _, start, size, section = next(s for s in elf.symbols if s[0] == name)
        refs = elf.references(name)
        for ref in refs:
            if 'value_hex' in ref:
                symbol = next(s for s in elf.symbols if s[0] == ref['symbol'])
                _, offset, length, index = symbol
                nested = elf.references(symbol[0])
                for sub in nested:
                    if 'value_hex' in sub: sub.pop('symbol')
                ref['symbol'] = {'bytes': elf.section_data(index)[offset:offset + length].hex(), 'relocations': nested}
        return elf.section_data(section)[start:start+size], refs
    for name, start, size, section in before.symbols:
        if not size or section == 0 or not before.sections[section][2] & 4 or name.startswith('syncEffectBck'): continue
        assert identity(before, name) == identity(after, name), name
        preservation.append({'unit': unit, 'name': name, 'size': size})
assert len(rows) == 10
(NOTES / 'root-evidence.json').write_text(json.dumps({'dol_sha1': hashlib.sha1(dol).hexdigest(), 'commands': commands,
    'source_sha256': {p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in (
        'src/Game/Effect/MultiEmitter.cpp', 'src/Game/Effect/SyncBckEffectChecker.cpp', 'src/Game/LiveActor/EffectKeeper.cpp')},
    'functions': rows, 'preserved_existing_methods': preservation}, indent=2) + '\n')
print(f'{len(rows)} functions / {sum(r["size"] for r in rows)} bytes verified; {len(preservation)} existing methods unchanged')
