#!/usr/bin/env python3
"""Inventory/migrate explicit stdexcept throws, preserving argument source bytes.

Dry-run by default. The lexer skips comments and treats ordinary/raw literals as
single tokens; argument matching balances (), [] and {}. Only the nine standard
message-bearing exception classes accepted by aurora/exception.hpp qualify.
No Game or imported SDK source is eligible. --apply requires explicit paths.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
TYPES = {"logic_error", "domain_error", "invalid_argument", "length_error", "out_of_range",
         "runtime_error", "range_error", "overflow_error", "underflow_error"}
NATIVE = {"app", "camera", "common", "compat", "debug", "layout", "render", "resource", "runtime", "scene", "showcase"}
AURORA_HOST = {"aurora/lib/audio.cpp", "aurora/lib/j_audio_stream.cpp", "aurora/lib/j_audio_sound_archive.cpp",
               "aurora/lib/sysconf.cpp", "aurora/lib/nw4r/brlan.cpp", "aurora/include/JSystem/JAudio2/JAISound.hpp"}
HELD = {"src/runtime/RuntimeContext.hpp", "src/showcase/Showcase.cpp", "src/scene/GatewayDemoScene.cpp",
        "src/scene/StageCollisionService.cpp", "src/scene/StageCollisionService.hpp", "src/compat/HitInfoCompat.cpp"}
LITERAL = re.compile(r'(?:u8|u|U|L)?(?:R"([^ ()\\\t\r\n]{0,16})\(|["\'])')
IDENTIFIER = re.compile(r'[A-Za-z_][A-Za-z_0-9]*')


def tokens(source):
    i = 0
    while i < len(source):
        if source[i].isspace():
            i += 1
            continue
        if source.startswith('//', i):
            end = source.find('\n', i + 2)
            i = len(source) if end < 0 else end
            continue
        if source.startswith('/*', i):
            end = source.find('*/', i + 2)
            if end < 0:
                raise ValueError('unterminated block comment')
            i = end + 2
            continue
        # C++ digit separators contain apostrophes but are not character literals.
        number = re.match(r"(?:[0-9]|\.[0-9])[A-Za-z_0-9'.]*", source[i:])
        if number:
            end = i + number.end()
            yield ('number', i, end)
            i = end
            continue
        literal = LITERAL.match(source, i)
        if literal:
            start = i
            if literal.group(1) is not None:
                terminator = ')' + literal.group(1) + '"'
                end = source.find(terminator, literal.end())
                if end < 0:
                    raise ValueError('unterminated raw string')
                i = end + len(terminator)
            else:
                quote = source[literal.end() - 1]
                i = literal.end()
                while i < len(source) and source[i] != quote:
                    i += 2 if source[i] == '\\' else 1
                if i >= len(source):
                    raise ValueError('unterminated string/character literal')
                i += 1
            yield ('literal', start, i)
            continue
        identifier = IDENTIFIER.match(source, i)
        if identifier:
            end = identifier.end()
        elif source.startswith('::', i):
            end = i + 2
        else:
            end = i + 1
        yield (source[i:end], i, end)
        i = end


def occurrences(source):
    stream = list(tokens(source))
    result = []
    for i, token in enumerate(stream):
        if token[0] != 'throw' or i + 4 >= len(stream):
            continue
        head = [entry[0] for entry in stream[i:i + 5]]
        if head[1:3] != ['std', '::'] or head[3] not in TYPES or head[4] != '(':
            continue
        stack = [')']
        commas = []
        j = i + 5
        while j < len(stream) and stack:
            value = stream[j][0]
            if value in {'(', '[', '{'}:
                stack.append({'(': ')', '[': ']', '{': '}'}[value])
            elif value in {')', ']', '}'}:
                if stack.pop() != value:
                    raise ValueError('unbalanced exception argument')
            elif value == ',' and len(stack) == 1:
                commas.append(stream[j][1])
            j += 1
        if stack:
            raise ValueError('unterminated exception expression')
        if commas:
            raise ValueError('multiple top-level arguments require manual review')
        argument_start, argument_end = stream[i + 4][2], stream[j - 1][1]
        argument = source[argument_start:argument_end]
        result.append({
            'line': source.count('\n', 0, token[1]) + 1,
            'type': 'std::' + head[3],
            'start': token[1], 'prefix_end': argument_start, 'expression_end': stream[j - 1][2],
            'before': source[token[1]:argument_start],
            'after': 'aurora::throw_host_exception<std::' + head[3] + '>(',
            'argument_sha256': hashlib.sha256(argument.encode()).hexdigest(),
            'argument': argument,
        })
    return result


def inspect(path):
    relative = path.relative_to(ROOT).as_posix()
    source = path.read_text()
    try:
        found = occurrences(source)
    except ValueError as error:
        raise ValueError(f'{relative}: {error}') from error
    return {'path': relative, 'sha256': hashlib.sha256(source.encode()).hexdigest(),
            'held_by_parent': relative in HELD, 'count': len(found), 'occurrences': found}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('paths', nargs='*')
    parser.add_argument('--apply', action='store_true')
    parser.add_argument('--approved-manifest', help='Require each selected file to match this reviewed dry-run manifest')
    parser.add_argument('--allow-held', action='append', default=[], help='Explicitly release a parent-held path after coordination')
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    if args.apply and not args.paths:
        parser.error('--apply requires explicit reviewed paths')
    if args.apply and not args.approved_manifest:
        parser.error('--apply requires --approved-manifest to reject source drift')
    paths = [ROOT / path for path in args.paths] if args.paths else sorted(
        path for path in (ROOT / 'src').rglob('*')
        if path.suffix in {'.cpp', '.hpp', '.h', '.cc', '.mm'} and path.relative_to(ROOT).parts[1] in NATIVE)
    approved = {item['path']: item for item in json.loads(Path(args.approved_manifest).read_text())['manifest']} if args.approved_manifest else {}
    manifests = []
    for path in paths:
        if (path.relative_to(ROOT).parts[:2] not in [('src', directory) for directory in NATIVE]
                and path.relative_to(ROOT).as_posix() not in AURORA_HOST):
            raise ValueError(f'not native-owned source: {path}')
        item = inspect(path)
        if not item['count']:
            continue
        manifests.append(item)
        if args.apply:
            if item['held_by_parent'] and item['path'] not in args.allow_held:
                raise ValueError(f'parent has active edits: {item["path"]}')
            if item['path'] not in approved or item['sha256'] != approved[item['path']]['sha256']:
                raise ValueError(f'source changed since manifest review: {item["path"]}')
    # Validate the complete cohort before making any changes.
    if args.apply:
        for item in manifests:
            path = ROOT / item['path']
            source = path.read_text()
            for change in reversed(item['occurrences']):
                source = source[:change['start']] + change['after'] + source[change['prefix_end']:]
            if '#include <aurora/exception.hpp>' not in source:
                start = source.find('#pragma once\n')
                start = start + len('#pragma once\n') if start >= 0 else 0
                source = source[:start] + '#include <aurora/exception.hpp>\n' + source[start:]
            path.write_text(source)
            item['after_sha256'] = hashlib.sha256(source.encode()).hexdigest()
    counts = {}
    for item in manifests:
        for change in item['occurrences']:
            counts[change['type']] = counts.get(change['type'], 0) + 1
    Path(args.output).write_text(json.dumps({'applied': args.apply, 'files': len(manifests),
        'count': sum(item['count'] for item in manifests), 'types': counts, 'manifest': manifests}, indent=2) + '\n')


if __name__ == '__main__':
    main()
