#!/usr/bin/env python3
"""Conservative source provenance and actual strong-symbol ownership report.

A source token match is not a semantic/compiler equivalence proof. An archive
provider is not evidence that the linker selected that member. Unknown and
ambiguous providers are deliberately retained in the output.
"""
import argparse
from collections import Counter, defaultdict
import csv
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

# Strings and comments precede punctuation; braces in either never affect depth.
TOKEN = re.compile(r'''//[^\n]*|/\*[\s\S]*?\*/|(?:u8|u|U|L)?R"([^ ()\\\t\n]{0,16})\([\s\S]*?\)\1"|(?:u8|u|U|L)?"(?:\\[\s\S]|[^"\\])*"|(?:u8|u|U|L)?'(?:\\[\s\S]|[^'\\])*'|[A-Za-z_][\w]*|(?:\d[\w.]*|\.\d[\w.]*)|::|->|&&|\|\||<<|>>|==|!=|<=|>=|\+\+|--|\+=|-=|\*=|/=|[^\s]''')


def tokens(text):
    return [m.group(0) for m in TOKEN.finditer(text)
            if not m.group(0).startswith(('//', '/*'))]


def definition(text, anchor):
    """Extract exactly one explicitly anchored definition, including its signature.

    This is a limited lexical check for the reviewed manifest, not a C++ parser.
    Unsupported/ambiguous declarations fail closed. Preprocessor branches are
    not removed, parameter names/literals are not rewritten.
    """
    source, needle = tokens(text), tokens(anchor)
    found = []
    for start in range(len(source) - len(needle) + 1):
        if source[start:start + len(needle)] != needle:
            continue
        opening = None
        parens = 0
        for i in range(start, len(source)):
            value = source[i]
            if value == '(':
                parens += 1
            elif value == ')':
                parens -= 1
            elif value == ';' and parens == 0:
                break
            elif value == '{' and parens == 0:
                opening = i
                break
        if opening is None:
            continue
        depth = 0
        for i in range(opening, len(source)):
            depth += (source[i] == '{') - (source[i] == '}')
            if depth == 0:
                found.append(source[start:i + 1])
                break
    if len(found) != 1:
        raise ValueError(f'expected one anchored definition, found {len(found)}: {anchor}')
    return found[0]


def check_provenance(root, records):
    result = []
    for record in records:
        row = dict(record)
        try:
            original = definition((root / record['original']).read_text(), record['anchor'])
            native = definition((root / record['provider']).read_text(), record['anchor'])
            row['status'] = 'source-tokens-exact' if original == native else 'source-tokens-differ'
            for name, value in [('original', original), ('provider', native)]:
                row[name + '_tokens_sha256'] = hashlib.sha256(json.dumps(value).encode()).hexdigest()
        except (OSError, UnicodeError, ValueError) as error:
            row['status'] = 'unresolved'
            row['reason'] = str(error)
        result.append(row)
    return result


def parse_nm(text):
    rows = []
    for line in text.splitlines():
        match = re.match(r'(.+)\[(.+)\]:\s+(\S+)\s+(\S)\s+', line)
        if match:
            archive, member, symbol, kind = match.groups()
            rows.append(dict(archive=archive, member=member, symbol=symbol, kind=kind))
        elif match := re.match(r'(.+):\s+(\S+)\s+(\S)\s+', line):
            obj, symbol, kind = match.groups()
            rows.append(dict(archive=obj, member=Path(obj).name, symbol=symbol, kind=kind))
        elif line.strip():
            raise ValueError('unrecognized archive nm output: ' + line)
    return rows


def duplicates(rows):
    owners = defaultdict(list)
    for row in rows:
        owners[row['symbol']].append(row)
    return {name: entries for name, entries in owners.items() if len(entries) > 1}


def symbol_provenance(records, provider, symbol):
    # Only an explicitly reviewed ABI identity can inherit a body comparison.
    # Name-family/prefix matching would incorrectly approve new overloads.
    identity = symbol[1:] if symbol.startswith('__Z') else symbol
    matches = [record for record in records if record['provider'] == provider
               and record.get('abi_symbol') == identity]
    return '|'.join(sorted({r['status'] for r in matches})) if matches else 'unreviewed'


def missing_reviewed_symbols(records, rows):
    available = {(row['sources'], row['symbol'][1:] if row['symbol'].startswith('__Z') else row['symbol'])
                 for row in rows if row['mapping'] == 'unique'}
    return [dict(provider=r['provider'], abi_symbol=r['abi_symbol']) for r in records
            if r.get('abi_symbol') and (r['provider'], r['abi_symbol']) not in available]


def run(*args, **kwargs):
    return subprocess.run(args, text=True, check=True, capture_output=True, **kwargs).stdout


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_tsv(path, columns, rows):
    with path.open('w') as stream:
        writer = csv.DictWriter(stream, columns, delimiter='\t', extrasaction='ignore', lineterminator='\n')
        writer.writeheader()
        writer.writerows(rows)


def audit(root, build_map, output, nm, cxxfilt, provenance_path):
    output.mkdir(parents=True, exist_ok=True)
    provenance = check_provenance(root, json.loads(provenance_path.read_text()))
    mapped = defaultdict(list)
    sourcefiles = set()
    artifacts, unavailable, rows = [], [], []
    for target in build_map['targets']:
        artifact = Path(target['artifact'])
        for pair in target['sources']:
            sourcefiles.add(pair['source'])
            mapped[(str(artifact), Path(pair['object']).name)].append(pair)
        if not artifact.is_file():
            unavailable.append(str(artifact))
            continue
        artifacts.append(dict(path=str(artifact), sha256=digest(artifact), mtime_ns=artifact.stat().st_mtime_ns))
        rows.extend(parse_nm(run(nm, '--format=posix', '--print-file-name', '--defined-only',
                                 '--extern-only', '--no-weak', str(artifact))))
    for pair in build_map.get('direct_objects', []):
        obj = Path(pair['object'])
        sourcefiles.add(pair['source'])
        mapped[(str(obj), obj.name)].append(pair)
        if not obj.is_file():
            unavailable.append(str(obj))
            continue
        artifacts.append(dict(path=str(obj), kind='executable-object', sha256=digest(obj), mtime_ns=obj.stat().st_mtime_ns))
        rows.extend(parse_nm(run(nm, '--format=posix', '--print-file-name', '--defined-only',
                                 '--extern-only', '--no-weak', str(obj))))
    executable = Path(build_map['executable'])
    present = set()
    if executable.is_file():
        present = set(run(nm, '--format=just-symbols', '--defined-only', '--extern-only', str(executable)).splitlines())
        artifacts.append(dict(path=str(executable), sha256=digest(executable), mtime_ns=executable.stat().st_mtime_ns))
    else:
        unavailable.append(str(executable))
    symbols = sorted({row['symbol'] for row in rows})
    # Mach-O prefixes Itanium manglings with an additional underscore.
    names = run(cxxfilt, input='\n'.join(s[1:] if s.startswith('__Z') else s for s in symbols) + '\n').splitlines()
    demangled = dict(zip(symbols, names))
    # Resolve duplicate object basenames only through actual object symbols.
    object_symbols = {}
    for row in rows:
        candidates = mapped.get((row['archive'], row['member']), [])
        if len(candidates) > 1:
            resolved = []
            for candidate in candidates:
                obj = candidate['object']
                if obj not in object_symbols:
                    object_symbols[obj] = (set(run(nm, '--format=just-symbols', '--defined-only', '--extern-only',
                                                   '--no-weak', obj).splitlines()) if Path(obj).is_file() else set())
                if row['symbol'] in object_symbols[obj]:
                    resolved.append(candidate)
            candidates = resolved
        row['name'] = demangled.get(row['symbol'], row['symbol'])
        row['sources'] = '|'.join(sorted({p['source'] for p in candidates}))
        row['mapping'] = 'unique' if len(candidates) == 1 else 'ambiguous' if candidates else 'unresolved-member'
        row['present_in_executable'] = 'yes' if row['symbol'] in present else 'no' if executable.is_file() else 'unavailable'
        row['source_newer_than_object'] = 'unresolved'
        row['provenance'] = 'unreviewed'
        if len(candidates) == 1:
            pair = candidates[0]
            obj, source = Path(pair['object']), root / pair['source']
            if obj.is_file() and source.is_file():
                row['source_newer_than_object'] = str(source.stat().st_mtime_ns > obj.stat().st_mtime_ns).lower()
            row['provenance'] = symbol_provenance(provenance, pair['source'], row['symbol'])
            if row['provenance'] == 'unreviewed' and pair['source'].startswith('src/Game/'):
                row['provenance'] = 'canonical-path-see-source-closeness-report'
    duplicate_rows = []
    for entries in duplicates(rows).values():
        duplicate_rows.extend(entries)
    columns = ['symbol', 'name', 'archive', 'member', 'sources', 'mapping', 'present_in_executable',
               'source_newer_than_object', 'provenance']
    write_tsv(output / 'symbol-providers.tsv', columns, rows)
    write_tsv(output / 'duplicate-strong-providers.tsv', columns, duplicate_rows)
    excluded = []
    for original in sorted((root / 'decomp/src/Game').rglob('*.cpp')):
        mirror = 'src/Game/' + str(original.relative_to(root / 'decomp/src/Game'))
        if mirror not in sourcefiles:
            imports = [p for p in provenance if p['original'] == str(original.relative_to(root))]
            excluded.append(dict(original=str(original.relative_to(root)), mirror=mirror,
                                 mirror_exists=(root / mirror).is_file(),
                                 reviewed_imports='|'.join(sorted({p['provider'] for p in imports})),
                                 coverage='partial-explicit-imports' if imports else 'unresolved'))
    write_tsv(output / 'excluded-original-units.tsv', ['original', 'mirror', 'mirror_exists', 'reviewed_imports', 'coverage'], excluded)
    (output / 'relocated-source-provenance.json').write_text(json.dumps(provenance, indent=2) + '\n')
    summary = dict(schema_version=1, scope=build_map.get("scope", "Legacy selected project archive inventory"), artifacts=artifacts, unavailable_artifacts=unavailable,
                   strong_symbols=len(rows), duplicate_strong_symbols=len(duplicates(rows)),
                   stale_source_providers=sum(r['source_newer_than_object'] == 'true' for r in rows),
                   unavailable_owner_inputs=sum(r['source_newer_than_object'] == 'unresolved' for r in rows),
                   provider_mapping=dict(Counter(r['mapping'] for r in rows)),
                   provider_provenance=dict(Counter(r['provenance'] for r in rows)),
                   source_checks=dict(Counter(r['status'] for r in provenance)),
                   excluded_original_units=len(excluded),
                   missing_reviewed_symbols=missing_reviewed_symbols(provenance, rows),
                   limits=['No inferred compatibility approval from directories or filenames.',
                           'Explicit provenance checks compare complete anchored signatures/bodies as tokens; no preprocessor or semantic equivalence claim.',
                           'Unlisted source imports remain unreviewed. Excluded-TU mappings are partial, not complete API coverage.',
                           'Executable symbol presence does not identify which duplicate archive member the linker selected.',
                           'Artifact hashes and source/object timestamps are not a full build attestation; header/flag freshness requires a successful build.'])
    (output / 'provider-audit.json').write_text(json.dumps(summary, indent=2) + '\n')
    print(json.dumps({k: v for k, v in summary.items() if k not in ('artifacts', 'limits')}))
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path.cwd())
    parser.add_argument('--build-map', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--nm', default=shutil.which('llvm-nm'))
    parser.add_argument('--cxxfilt', default=shutil.which('llvm-cxxfilt'))
    args = parser.parse_args()
    if not args.nm or not args.cxxfilt:
        parser.error('LLVM llvm-nm and llvm-cxxfilt are required; pass their paths explicitly')
    audit(args.root, json.loads(args.build_map.read_text()), args.output, args.nm, args.cxxfilt,
          args.root / 'scripts/source_provider_provenance.json')


if __name__ == '__main__':
    main()
