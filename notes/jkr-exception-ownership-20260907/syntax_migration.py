#!/usr/bin/env python3
import concurrent.futures
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
NOTES = Path(__file__).resolve().parent
database = json.loads((ROOT / 'compile_commands.json').read_text())
commands = {entry['file']: entry for entry in database}
files = []
for name in ['native-migration-manifest.json', 'aurora-migration-manifest.json']:
    files += [entry['path'] for entry in json.loads((NOTES / name).read_text())['manifest']]


def check(path):
    entry = commands.get(path)
    if entry is None:
        # The inactive ScreenshotService and headers use a real command from
        # their owning library. Record that substitution explicitly.
        prefix = '/'.join(path.split('/')[:2]) + '/'
        if path.startswith('aurora/include/'):
            prefix = 'src/compat/'
        entry = next(row for row in database if row['file'].startswith(prefix))
    args = []
    source = entry['file']
    skip = False
    for argument in entry['arguments']:
        if skip:
            skip = False
            continue
        if argument == '-o':
            skip = True
        elif argument not in {'-c', source}:
            args.append(argument)
    # Matches the new public common-header include path without requiring a
    # competing Xmake configure job just to refresh compile_commands.json.
    args += ['-Iaurora/include', '-x', 'c++', '-fsyntax-only', path]
    log = NOTES / 'syntax' / (path.replace('/', '__') + '.log')
    with log.open('w') as output:
        run = subprocess.run(args, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT)
    return {'file': path, 'sha256': hashlib.sha256((ROOT / path).read_bytes()).hexdigest(),
            'command_source': source, 'arguments': args, 'exit': run.returncode,
            'log': str(log.relative_to(ROOT))}


(NOTES / 'syntax').mkdir(exist_ok=True)
with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool:
    results = list(pool.map(check, files))
(NOTES / 'syntax-results.json').write_text(json.dumps(results, indent=2) + '\n')
failures = [row for row in results if row['exit']]
print(json.dumps({'checked': len(results), 'passed': len(results) - len(failures),
                  'failures': [{'file': row['file'], 'exit': row['exit'], 'log': row['log']} for row in failures]}, indent=2))
raise SystemExit(bool(failures))
