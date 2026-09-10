#!/usr/bin/env python3
"""Compile original Game narrow literals as CP932 after Clang macro expansion.

Editable source stays UTF-8. Clang records literal provenance and emits normal
preprocessed output; temporary output is removed after compilation.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import unicodedata
import tempfile
import sys

NARROW = {'string_literal', 'char_constant'}
STRINGS = {'string_literal', 'wide_string_literal', 'utf8_string_literal', 'utf16_string_literal', 'utf32_string_literal'}

class EncodingError(ValueError): pass

def sha(data: bytes) -> str: return hashlib.sha256(data).hexdigest()
def octal(data: bytes) -> str: return ''.join('\\%03o' % byte for byte in data)
def cp932(text: str) -> str:
    try: return octal(text.encode('cp932', errors='strict'))
    except UnicodeEncodeError as error: raise EncodingError(f'not representable in original CP932: {text!r}') from error

def splice_end(text: str, index: int) -> int | None:
    if text.startswith('\\\r\n', index): return index + 3
    if text.startswith('\\\n', index) or text.startswith('\\\r', index): return index + 2
    return None

def physical_splices(text: str) -> str:
    output=[];index=0
    while index<len(text):
        end=splice_end(text,index)
        if end is not None: output.append(text[index:end]);index=end
        else: index+=1
    return ''.join(output)

def unsplice(text: str) -> str:
    output=[];index=0
    while index<len(text):
        end=splice_end(text,index)
        if end is not None: index=end
        else: output.append(text[index]);index+=1
    return ''.join(output)

def convert(token: str) -> str:
    """Token is known by Clang to be an ordinary narrow string or character."""
    logical=unsplice(token)
    raw=logical.startswith('R"')
    if raw:
        # Raw contents revert phase-2 line splicing. Only the prefix/delimiter
        # participates in ordinary source processing; keep raw body bytes.
        quote=token.index('"');opening=token.index('(',quote+1)
        delimiter=unsplice(token[quote+1:opening])
        closing=')'+delimiter+'"';end=token.rfind(closing)
        if end<opening: raise EncodingError('malformed raw literal')
        body=token[opening+1:end]
        if not any(ord(c)>127 for c in body): return token
        result='"' + physical_splices(token[:opening+1])
        index=0
        while index<len(body):
            char=body[index]
            if char in '\r\n':
                # Translation phase 1 normalizes physical source newlines,
                # including inside raw strings; phase 2 splicing is reverted.
                newline='\r\n' if body.startswith('\r\n',index) else char
                result+=cp932('\n')+'\\'+newline
                index+=len(newline)
            else:
                result+=cp932(char);index+=1
        result+='"'+token[end+len(closing):]
        return result
    quote=token.index('"') if '"' in token and (not token.startswith("'")) else token.index("'")
    delimiter=token[quote];output=[token[:quote+1]];index=quote+1;changed=False
    while index<len(token):
        char=token[index]
        if char==delimiter:
            output.append(token[index:]);return ''.join(output) if changed else token
        if char=='\\':
            end=splice_end(token,index)
            if end is not None: output.append(token[index:end]);index=end;continue
            if index+1>=len(token): raise EncodingError('unterminated escape')
            escaped=token[index+1]
            if escaped in 'uUN':
                # UCN digits may themselves contain phase-2 splices. Decode the
                # logical sequence, retaining its physical line count.
                logical_tail=unsplice(token[index:])
                if escaped=='N' or logical_tail.startswith('\\u{'):
                    closing=logical_tail.find('}')
                    if not logical_tail.startswith('\\'+escaped+'{') or closing<0: raise EncodingError('malformed braced UCN')
                    sequence=logical_tail[:closing+1]
                    try: value=unicodedata.lookup(sequence[3:-1]) if escaped=='N' else chr(int(sequence[3:-1],16))
                    except (ValueError,KeyError) as error: raise EncodingError('invalid universal character name') from error
                else:
                    length=6 if escaped=='u' else 10;sequence=logical_tail[:length]
                    if len(sequence)!=length: raise EncodingError('short universal character name')
                    try: value=chr(int(sequence[2:],16))
                    except ValueError as error: raise EncodingError('invalid universal character name') from error
                physical_end=index;logical_count=0
                while logical_count<len(sequence):
                    end=splice_end(token,physical_end)
                    if end is not None: physical_end=end
                    else: physical_end+=1;logical_count+=1
                original=token[index:physical_end]
                if ord(value)>127:
                    output.append(cp932(value));output.append(physical_splices(original));changed=True
                else: output.append(original)
                index=physical_end;continue
            # Keep numeric/simple escapes as authored. In particular a literal
            # backslash followed by 'u' must not become a universal character.
            output.append(token[index:index+2]);index+=2;continue
        if ord(char)>127: output.append(cp932(char));changed=True
        else: output.append(char)
        index+=1
    raise EncodingError('unterminated ordinary literal')

def transform(data: bytes, tokens: list[dict]) -> tuple[bytes, dict]:
    protected = set()
    run = []
    def close():
        if any(token['kind'] != 'string_literal' for token in run):
            protected.update(token['begin'] for token in run)
        else:
            # ASCII can join either domain. Joining independently encoded
            # non-ASCII Game and host literals cannot produce one coherent
            # ordinary string; require an explicit boundary in the host source.
            domains = set()
            for token in run:
                spelling = data[token['begin']:token['end']].decode('utf-8')
                if any(ord(char) > 127 for char in spelling) or any(
                        marker in spelling for marker in ('\\u', '\\U', '\\N')):
                    domains.add(token['game'])
            if len(domains) > 1:
                token = run[0]
                raise EncodingError(f"{token['file']}:{token['line']}: adjacent non-ASCII literals mix Game and host execution encodings")
        run.clear()
    previous = None
    for token in tokens:
        if token['kind'] not in STRINGS:
            close(); previous = None; continue
        if previous is None or token['sequence'] != previous + 1:
            close()
        run.append(token)
        previous = token['sequence']
    close()

    parts = []
    cursor = 0
    converted = 0
    for token in tokens:
        start, end = token['begin'], token['end']
        if not token['game'] or token['kind'] not in NARROW or start in protected:
            continue
        original = data[start:end].decode('utf-8', errors='strict')
        try:
            view = convert(original).encode('utf-8')
        except EncodingError as error:
            raise EncodingError(f"{token['file']}:{token['line']}: {error}") from error
        if view == data[start:end]:
            continue
        if view.count(b'\n') != data[start:end].count(b'\n'):
            raise EncodingError('preprocessed source line count changed')
        parts.extend((data[cursor:start], view))
        cursor = end
        converted += 1
    parts.append(data[cursor:])
    return b''.join(parts), {'converted_literals': converted, 'literal_tokens': len(tokens)}


def codegen_arguments(arguments: list[str], source: str, preprocessed: Path) -> list[str]:
    # The helper already processed includes, macros and dependencies. Retain
    # target/ABI/optimization/debug flags for the real compiler's code generation.
    paired = {'-include', '-imacros', '-include-pch', '-D', '-U', '-I', '-F', '-isystem',
              '-iquote', '-idirafter', '-iframework', '-iprefix', '-iwithprefix',
              '-iwithprefixbefore', '-MF', '-MT', '-MQ', '-x'}
    single = {'-MD', '-MMD', '-MP', '-MG', '-nostdinc', '-nostdinc++', '-undef', '-P'}
    output = []
    index = 0
    while index < len(arguments):
        value = arguments[index]
        if value in paired:
            index += 2; continue
        if value in single or value == source:
            index += 1; continue
        if any(value.startswith(prefix) and len(value) > len(prefix)
               for prefix in ('-D', '-U', '-I', '-F', '-MF', '-MT', '-MQ', '-Wp,')):
            index += 1; continue
        output.append(value)
        index += 1
    output.extend(['-x', 'c++-cpp-output', str(preprocessed), '-Qunused-arguments'])
    return output


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--helper', type=Path, required=True)
    parser.add_argument('--game-root', type=Path, action='append', default=[])
    parser.add_argument('--game-file', type=Path, action='append', default=[])
    parser.add_argument('--report', type=Path)
    parser.add_argument('arguments', nargs=argparse.REMAINDER)
    options = parser.parse_args()
    arguments = options.arguments
    if arguments and arguments[0] == '--': arguments = arguments[1:]
    compiler = str(options.compiler)
    if any(arg.startswith('@') for arg in arguments):
        raise EncodingError('expand compiler response files before the Game execution charset wrapper')
    sources = [arg for arg in arguments if Path(arg).suffix in {'.cpp', '.cxx', '.cc', '.c++', '.C'} and Path(arg).is_file()]
    if '-c' not in arguments or not sources:
        return subprocess.run([compiler, *arguments]).returncode
    if not options.game_root and not options.game_file:
        raise EncodingError('at least one explicit Game source root or file is required')
    if len(sources) != 1:
        raise EncodingError('Game execution charset requires one C++ compilation input')
    if any(arg.startswith(('-fmodules', '-fmodule-file', '-include-pch')) for arg in arguments):
        raise EncodingError('Game execution charset requires textual headers, without PCH or modules')
    source = sources[0]
    with tempfile.TemporaryDirectory(prefix='smgpc-game-charset-') as temporary:
        directory = Path(temporary)
        preprocessed = directory / (Path(source).stem + '.ii')
        metadata = directory / 'literals.json'
        command = [str(options.helper), '--output', str(preprocessed), '--metadata', str(metadata)]
        for root in options.game_root:
            command.extend(['--game-root', str(root.resolve())])
        for file in options.game_file:
            command.extend(['--game-file', str(file.resolve())])
        command.extend(['--', compiler, *arguments])
        result = subprocess.run(command)
        if result.returncode:
            return result.returncode
        tokens = json.loads(metadata.read_text())['tokens']
        encoded, stats = transform(preprocessed.read_bytes(), tokens)
        preprocessed.write_bytes(encoded)
        result = subprocess.run([compiler, *codegen_arguments(arguments, source, preprocessed)])
        if options.report:
            options.report.parent.mkdir(parents=True, exist_ok=True)
            options.report.write_text(json.dumps({
                'source': source, 'source_sha256': sha(Path(source).read_bytes()),
                'arguments': arguments, 'exit': result.returncode, **stats
            }, indent=2) + '\n')
        return result.returncode


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (EncodingError, OSError, ValueError) as error:
        print(f'Game execution charset: {error}', file=sys.stderr)
        raise SystemExit(1)
