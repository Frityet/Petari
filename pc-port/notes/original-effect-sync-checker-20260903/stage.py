#!/usr/bin/env python3
from pathlib import Path
import hashlib, json, shutil
ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-effect-sync-checker-20260903'
headers = ['MultiEmitter', 'SyncBckEffectInfo', 'EffectSystem', 'EffectSystemUtil', 'ParticleResourceHolder',
           'MultiEmitterCallBack', 'MultiEmitterParticleCallBack', 'SingleEmitter', 'ParticleEmitter', 'SyncBckEffectChecker']
rows = []
for prefix, suffix, names in [('include', '.hpp', headers), ('src', '.cpp', ['MultiEmitter', 'SyncBckEffectInfo', 'SyncBckEffectChecker'])]:
    for name in names:
        source = ROOT / prefix / ('Game/Effect/' + name + suffix)
        target = BUILD / 'staged' / ('Game/Effect/' + name + suffix)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        rows.append({'source': str(source.relative_to(ROOT)), 'staged': str(target.relative_to(ROOT)),
                     'sha256': hashlib.sha256(source.read_bytes()).hexdigest(), 'root_identical': True})
(NOTES / 'native-manifest.json').write_text(json.dumps(rows, indent=2) + '\n')

for prefix, path in [('src', 'Game/LiveActor/EffectKeeper.cpp'), ('include', 'Game/LiveActor/EffectKeeper.hpp'), ('include', 'Game/Effect/AutoEffectInfo.hpp'), ('include', 'Game/Effect/SyncBckEffectChecker.hpp')]:
    source = ROOT / prefix / path; target = BUILD / 'staged' / path
    target.parent.mkdir(parents=True, exist_ok=True); shutil.copyfile(source, target)
    rows.append({'source':str(source.relative_to(ROOT)), 'staged':str(target.relative_to(ROOT)), 'sha256':hashlib.sha256(source.read_bytes()).hexdigest(), 'root_identical':True})
(NOTES / 'native-manifest.json').write_text(json.dumps(rows, indent=2)+'\n')
