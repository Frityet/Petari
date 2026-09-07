#!/usr/bin/env python3
"""Prove native empty Nerve construction preserves identity while removing unused roots."""
import json
import re
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[2]
work = Path(__file__).resolve().parent
header = root / 'src/Game/LiveActor/Nerve.hpp'
text = header.read_text()
if 'NERVE_DEFAULT_CONSTRUCTOR' not in text:
    pattern = r'name\(\) NO_INLINE\{\};'
    assert len(re.findall(pattern, text)) == 6
    text = re.sub(pattern, 'NERVE_DEFAULT_CONSTRUCTOR(name);', text)
    pattern = r'name\(\) NO_INLINE \{[^\n]*\\\n\s*\}'
    assert len(re.findall(pattern, text)) == 1
    text = re.sub(pattern, 'NERVE_DEFAULT_CONSTRUCTOR(name)', text)
    marker = '/* Defines a basic nerve class */'
    text = text.replace(marker, '''// Native constant initialization keeps unused singleton vtables eligible for section GC.
// Every macro constructor below is empty; retain the original constructor on Wii.
#if defined(TARGET_PC)
#define NERVE_DEFAULT_CONSTRUCTOR(name) constexpr name() {}
#else
#define NERVE_DEFAULT_CONSTRUCTOR(name) name() NO_INLINE {}
#endif

''' + marker)
shadow = work / 'include/Game/LiveActor/Nerve.hpp'
shadow.parent.mkdir(parents=True, exist_ok=True)
shadow.write_text(text)
rows = json.loads((root / 'compile_commands.json').read_text())
row = next(r for r in rows if r['file'].endswith('/Animation/XanimePlayer.cpp'))
base = row['arguments'][:row['arguments'].index('-o')]
base.insert(1, '-I' + str(work / 'include'))
commands = []
def run(args, name):
    result = subprocess.run(args, cwd=root, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (work / (name + '.log')).write_text(result.stdout)
    commands.append({'name': name, 'arguments': args, 'returncode': result.returncode})
    (work / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n')
    print(name, result.returncode, result.stdout[-2000:])
    if result.returncode:
        raise SystemExit(result.returncode)

run(base + ['-o', str(work / 'MarioJump.constexpr.o'), 'src/Game/Player/MarioJump.cpp'], 'MarioJump.constexpr')
for source in ['proof-a.cpp', 'proof-b.cpp']:
    run(base + ['-o', str(work / (source + '.o')), str(work / source)], source)
for source in ['Nerve', 'Spine']:
    run(base + ['-o', str(work / (source + '.o')), 'src/Game/LiveActor/' + source + '.cpp'], source)
linkbase = [arg for arg in base if arg != '-c']
run(linkbase + ['-Wl,-dead_strip', '-o', str(work / 'proof'), *[str(work / s) for s in ['proof-a.cpp.o', 'proof-b.cpp.o', 'Nerve.o', 'Spine.o']]], 'proof-link')
run([str(work / 'proof')], 'proof-run')
run(['/opt/homebrew/opt/llvm/bin/llvm-objdump', '--section-headers', str(work / 'MarioJump.constexpr.o')], 'constexpr-sections')
run(['/opt/homebrew/opt/llvm/bin/llvm-nm', '--demangle', str(work / 'MarioJump.constexpr.o')], 'constexpr-symbols')
assert '__mod_init_func' not in (work / 'constexpr-sections.log').read_text()
assert '__cxx_global_var_init' not in (work / 'constexpr-symbols.log').read_text()
assert '_GLOBAL__sub_I' not in (work / 'constexpr-symbols.log').read_text()
print('PASS: no dynamic singleton initializers remain in the whole original MarioJump object')
