#!/usr/bin/env python3
from pathlib import Path
import hashlib, json, shutil
ROOT = Path(__file__).resolve().parents[3]
NOTES = Path(__file__).resolve().parent
BUILD = ROOT / 'build/original-multi-emitter-sync-20260903'
headers = ['MultiEmitter', 'SyncBckEffectInfo', 'EffectSystem', 'EffectSystemUtil', 'ParticleResourceHolder',
           'MultiEmitterCallBack', 'MultiEmitterParticleCallBack', 'SingleEmitter', 'ParticleEmitter']
rows = []
for prefix, suffix, names in [('include', '.hpp', headers), ('src', '.cpp', ['MultiEmitter', 'SyncBckEffectInfo'])]:
    for name in names:
        source = ROOT / prefix / ('Game/Effect/' + name + suffix)
        target = BUILD / 'staged' / ('Game/Effect/' + name + suffix)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        rows.append({'source': str(source.relative_to(ROOT)), 'staged': str(target.relative_to(ROOT)),
                     'sha256': hashlib.sha256(source.read_bytes()).hexdigest(), 'root_identical': True})
(NOTES / 'native-manifest.json').write_text(json.dumps(rows, indent=2) + '\n')
