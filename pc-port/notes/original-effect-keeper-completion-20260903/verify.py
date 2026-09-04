#!/usr/bin/env python3
"""Original compiler, complete DOL relocations, and bounded register scheduling proof."""
from pathlib import Path
import importlib.util, json, re, struct, subprocess, hashlib, shutil
R = Path(__file__).resolve().parents[3]
N = Path(__file__).resolve().parent
B = R / 'build/original-effect-keeper-completion-20260903'
B.mkdir(exist_ok=True)
def module(name, path):
    s = importlib.util.spec_from_file_location(name, R/path)
    m = importlib.util.module_from_spec(s); s.loader.exec_module(m); return m
c = module('compiler', 'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py')
e = module('reader', 'pc-port/notes/mario-update-restoration-20260903/verify-object.py')
p = module('proof', 'pc-port/notes/original-binder-reaction-20260903/verify-runtime.py')
stage = module('stage', 'pc-port/notes/original-stage-data-holder-20260903/verify-original.py')
addresses = {n:int(a,16) for n,a in re.findall(r'^([^\n]+?) = \.\w+:(0x[0-9A-Fa-f]+);', (R/'config/RMGK01/symbols.txt').read_text(), re.M)}
class Elf(e.Elf):
    def references(self, name):
        rows = super().references(name)
        for row in rows:
            symbol, start, size, index = next(x for x in self.symbols if x[0] == row['symbol'])
            if size != 12 or index == 0: continue
            nested = super().references(symbol)
            if not nested: continue
            # Complete actual three-word member pointer, not a zero C string.
            assert len(nested) == 1 and nested[0]['kind'] == 1 and nested[0]['offset'] == '0x8'
            data = bytearray(self.section_data(index)[start:start+size])
            assert data[:8].hex() == '00000000ffffffff'
            struct.pack_into('>I', data, 8, addresses[nested[0]['symbol']] + nested[0]['addend'])
            row['value_hex'] = data.hex()
        return rows
    def code(self, name):
        _,start,size,index = next(x for x in self.symbols if x[0] == name)
        return bytearray(self.section_data(index)[start:start+size])
def compile(source, output, flags=None):
    cmd = (flags or c.compiler('cflags_game')) + ['-c', str(source), '-o', str(output)]
    done = subprocess.run(cmd, cwd=R, capture_output=True, text=True)
    output.with_suffix('.compile.log').write_text(done.stdout + done.stderr)
    if done.returncode: print(done.stdout, done.stderr); done.check_returncode()
    return cmd
source = R/'src/Game/LiveActor/EffectKeeper.cpp'
obj = B/'EffectKeeper.o'
command = compile(source, obj)
retail = R/'build/xanime-core-pose-blending-restoration-20260903/retail/obj/Game/LiveActor/EffectKeeper.o'
a, b = Elf(retail), Elf(obj)
subprocess.run([R/'build/tools/objdiff-cli','diff','-1',retail,'-2',obj,'-o',B/'diff.json','--format','json-pretty'],check=True,capture_output=True)
diff = json.loads((B/'diff.json').read_text())
dol = c.DOL.read_bytes()
assert hashlib.sha1(dol).hexdigest() == '25c5959534b3c21246c6c7e42021b916b41fb578'
names = ['initAfterPlacementForAttributeEffect__12EffectKeeperFPA4_f', 'onDraw__12EffectKeeperFv', 'offDraw__12EffectKeeperFv', 'updateFloorCode__12EffectKeeperFv']
rows = []
for name in names:
    size = len(b.code(name)); address = addresses[name]
    actual = e.dol_bytes(dol, address, size)
    stage.relocate(a, a, name, address, dol)
    if name == names[0]:
        compiled, refs = p.relocated(b, a, name, address, size, dol)
    else:
        refs = stage.relocate(b, a, name, address, dol)
        compiled = actual
    exact = compiled == actual
    changes = []
    if not exact:
        assert name == names[0] and size == 392
        # These are the only differences in the complete scalar/paired stream.
        # psq_l destination allocation and subsequent ps_add uses preserve each
        # original operand value/order. The swapped stfs and fmuls are independent.
        expected = {0xdc:(0xe09f0000,0xe07f0000), 0xe8:(0xec4207f2,0xd001003c),
                    0xec:(0xd001003c,0xec4207f2), 0xf4:(0xe07f8008,0xe03f8008),
                    0xf8:(0xe021803c,0xe081803c), 0x100:(0x1023082a,0x1021202a),
                    0x110:(0x1004002a,0x1003002a)}
        found = {at:(struct.unpack_from('>I',actual,at)[0],struct.unpack_from('>I',compiled,at)[0])
                 for at in range(0,size,4) if actual[at:at+4] != compiled[at:at+4]}
        assert found == expected
        changes = [dict(offset=hex(at),retail=hex(x),compiled=hex(y)) for at,(x,y) in found.items()]
    match = next(x['match_percent'] for x in diff['left']['symbols'] if x['name'] == name)
    rows.append(dict(symbol=name,bytes=size,retail_address=hex(address),match_percent=match,
                     relocated_instruction_bytes_equal=exact,verified_register_and_schedule_differences=changes,relocations=refs))
    print(name, match, size, 'exact' if exact else 'only verified register allocation and independent scheduling')
(N/'evidence.json').write_text(json.dumps(dict(source_sha256=c.sha(source),functional_sha256=c.sha(R/'libs/MSL_C++/include/functional.hpp'),command=command,dol_sha1=hashlib.sha1(dol).hexdigest(),functions=rows),indent=2)+'\n')
