#!/usr/bin/env python3
"""Expose the real compiler to source editors after Xmake exports wrapped builds."""
import argparse
import json
from pathlib import Path
import shlex


def normalize(entries, wrapper):
    changed = 0
    for entry in entries:
        arguments = entry.get('arguments', [])
        if not arguments:
            continue
        command = shlex.split(arguments[0])
        if len(command) < 3 or Path(command[1]).resolve() != wrapper:
            continue
        if '--compiler' not in command or '--' not in command:
            raise ValueError('Malformed original Game compiler wrapper in compilation database')
        compiler = command[command.index('--compiler') + 1]
        entry['arguments'] = [compiler, *command[command.index('--') + 1:], *arguments[1:]]
        changed += 1
    return changed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('database', type=Path)
    args = parser.parse_args()
    wrapper = Path(__file__).resolve().with_name('game_execution_charset.py')
    entries = json.loads(args.database.read_text())
    if normalize(entries, wrapper):
        temporary = args.database.with_suffix('.json.new')
        temporary.write_text(json.dumps(entries, indent=2) + '\n')
        temporary.replace(args.database)


if __name__ == '__main__':
    main()
