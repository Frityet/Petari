"""One-time, source-preserving CP932 annotation. Not part of the build."""
from pathlib import Path
import hashlib
import json
import re
from inventory import tokens

ROOT = Path('notes/explicit-game-encoding-20260919')
INCLUDE = '#include "compat/Cp932Literal.hpp"\n'

def migrate(source):
    lexed = list(tokens(source))
    edits = []
    index = 0
    while index < len(lexed):
        token = lexed[index]
        if token['kind'] not in ('string', 'raw'):
            index += 1
            continue
        first = index
        while index < len(lexed) and lexed[index]['kind'] in ('string', 'raw'):
            index += 1
        group = lexed[first:index]
        if not any(any(ord(c) > 127 for c in t['text']) or re.search(r'\\[uUN]', t['text']) for t in group):
            continue
        assert all(t['text'].startswith(('"', 'R"')) for t in group), group
        if first >= 2 and [t['text'] for t in lexed[first-2:first]] == ['CP932', '(']:
            continue
        start, end = group[0]['start'], group[-1]['end']
        edits.append((start, end))
    result = source
    for start, end in reversed(edits):
        result = result[:start] + 'CP932(' + result[start:end] + ')' + result[end:]
    if edits and not result.startswith(INCLUDE):
        result = INCLUDE + result
    return result, edits

if __name__ == '__main__':
    manifest = []
    for path in sorted(Path('src/Game').rglob('*')):
        if path.suffix not in ('.cpp', '.hpp', '.h', '.inl'):
            continue
        before = path.read_text()
        after, edits = migrate(before)
        if not edits:
            continue
        snapshot = ROOT / 'before' / path
        assert not snapshot.exists(), f'Existing migration snapshot: {snapshot}'
        snapshot.parent.mkdir(parents=True, exist_ok=True)
        snapshot.write_text(before)
        path.write_text(after)
        # Reversing the exact insertions must recover every original byte.
        restored = after[len(INCLUDE):]
        added = 0
        for start, end in edits:
            begin = start + added
            stop = end + added + len('CP932(')
            assert restored[begin:begin+6] == 'CP932(' and restored[stop] == ')'
            added += 7
        for start, end in reversed(edits):
            # Use the already verified original spans to build the independent
            # expected annotated text; no token or whitespace normalization.
            before = before[:start] + 'CP932(' + before[start:end] + ')' + before[end:]
        assert restored == before
        assert migrate(after) == (after, []), path
        manifest.append({'path': str(path), 'wrappers': len(edits),
                         'before_sha256': hashlib.sha256(snapshot.read_bytes()).hexdigest(),
                         'after_sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
    (ROOT / 'migration.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps({'files': len(manifest), 'wrappers': sum(r['wrappers'] for r in manifest)}))
