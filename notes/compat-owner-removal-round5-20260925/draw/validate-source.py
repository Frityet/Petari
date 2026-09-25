from pathlib import Path
from collections import Counter, defaultdict
import hashlib
import json
from source_parser import functions, normalize

root = Path(__file__).resolve().parents[3]
notes = Path(__file__).resolve().parent
checks = []

def check(label, result):
    checks.append({'check': label, 'passed': bool(result)})
    if not result:
        raise AssertionError(label)

def mwerks_only(source):
    # These owner files only use this conditional for paired-single bodies.
    output, active = [], [True]
    for line in source.splitlines(True):
        directive = line.strip()
        if directive == '#ifdef __MWERKS__':
            active.append(active[-1])
        elif directive == '#else':
            active[-1] = active[-2] and not active[-1]
        elif directive == '#endif':
            active.pop()
        elif active[-1]:
            output.append(line)
    assert len(active) == 1
    return ''.join(output)

base = root / 'src/JSystem/J3DGraphBase'
donor = root / 'decomp/src/JSystem/J3DGraphBase'
files = ['J3DDrawBuffer.cpp', 'J3DStruct.cpp', 'J3DGD.cpp']
for name in files:
    native = (base / name).read_text()
    original = (donor / name).read_text()
    check(name + ': complete donor function set',
          Counter(f['symbol'] for f in functions(native)) == Counter(f['symbol'] for f in functions(original)))

check('DrawBuffer is byte-exact complete donor',
      (base / files[0]).read_bytes() == (donor / files[0]).read_bytes())
check('Struct retains every donor statement on its original architecture',
      mwerks_only((base / files[1]).read_text()) == mwerks_only((donor / files[1]).read_text()))
gd = (base / files[2]).read_text()
for name in ['GDIndirect', 'GDLight', 'GDPixel', 'GDTev', 'GDTexture']:
    gd = gd.replace('#include <dolphin/gd/' + name + '.h>', '#include <revolution/gd/' + name + '.h>')
gd = gd.replace('void J3DGDSetFogRangeAdj(u8 enable,', 'void J3DGDSetFogRangeAdj(GXBool enable,')
check('GD complete donor except native includes and original-byte GXBool boundary',
      gd == (donor / files[2]).read_text())

original_header = root / 'decomp/libs/JSystem/include/JSystem/J3DGraphBase/J3DGD.hpp'
header = base / 'J3DGD.hpp'
xf_names = ['J3DGDWriteXFCmd', 'J3DGDWriteXFCmdHdr']
for symbol in xf_names:
    ours = [f['normalized'] for f in functions(header.read_text()) if f['symbol'] == symbol]
    orig = [f['normalized'] for f in functions(original_header.read_text()) if f['symbol'] == symbol]
    check(symbol + ': exact original inline header owner', ours == orig and len(ours) == 1)

expected = {}
for name in files:
    for function in functions((base / name).read_text()):
        check(function['symbol'] + ': not an accidental extra local copy', function['symbol'] not in expected)
        expected[function['symbol']] = str((base / name).relative_to(root))
for name in xf_names:
    expected[name] = str(header.relative_to(root))
providers = defaultdict(list)
for path in (root / 'src').rglob('*'):
    if path.suffix not in ('.cpp', '.hpp'):
        continue
    content = path.read_text(errors='replace')
    if not any(name in content for name in expected):
        continue
    for f in functions(content):
        if f['symbol'] in expected:
            providers[f['symbol']].append(str(path.relative_to(root)))
for name, owner in expected.items():
    check(name + ': one actual definition in correct owner', providers[name] == [owner])

before = notes / 'before/src/compat'
for name in ['J3DDrawBufferCompat.cpp', 'J3DStructCompat.cpp', 'J3DGDCompat.cpp']:
    check(name + ': deleted with recorded snapshot', not (root / 'src/compat' / name).exists() and (before / name).exists())

struct = normalize((base / 'J3DStruct.cpp').read_text())
check('Native matrix extension writes the exact homogeneous final row',
      normalize('''JMath::gekko_ps_copy12(mEffectMtx, param_0);
      mEffectMtx[3][0] = kIdentityZero; mEffectMtx[3][1] = kIdentityZero;
      mEffectMtx[3][2] = kIdentityZero; mEffectMtx[3][3] = kIdentityW;''') in struct)
check('Native center copy retains previous architecture adapter',
      'JMathInlineVEC::PSVECCopy(&param_0.mCenter,&mCenter);' in struct)
check('Native indirect matrix loads six components before any store',
      normalize('''f32 a = param_0.field_0x0[0][0], b = param_0.field_0x0[0][1], c = param_0.field_0x0[0][2];
      f32 d = param_0.field_0x0[1][0], e = param_0.field_0x0[1][1], f = param_0.field_0x0[1][2];
      field_0x0[0][0] = a; field_0x0[0][1] = b; field_0x0[0][2] = c;
      field_0x0[1][0] = d; field_0x0[1][1] = e; field_0x0[1][2] = f;''') in struct)
paths = [base / name for name in files] + [header]
result = {'kind': 'source verification, not compilation or runtime test', 'checks': checks,
          'definition_count': len(expected),
          'sha256': {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}}
(notes / 'source-validation.json').write_text(json.dumps(result, indent=2) + '\n')
print(f'{len(checks)} source checks passed; {len(expected)} single-owner definitions verified')
