#!/usr/bin/env python3
"""Standalone compiler tests; no source preprocessing is part of a game build."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cxx", default="clang++")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    data = (root / "src/resource/detail/Cp932Mapping.tsv").read_bytes()
    expected_hash = "46778ae55afa614d3ece7e4f7160d9f1205626f09a461e6d7a933b57b582d600"
    assert hashlib.sha256(data).hexdigest() == expected_hash
    entries = []
    for line in data.decode().splitlines():
        if line and not line.startswith("#"):
            point, encoded = line.split()
            entries.append((int(point, 16), bytes.fromhex(encoded)))
    assert len(entries) == 9280
    assert len({point for point, _ in entries}) == len(entries)
    assert entries == sorted(entries)
    for point, encoded in entries:
        assert chr(point).encode("cp932") == encoded
    table = (root / "src/resource/detail/Cp932Table.hpp").read_text()
    actual = [(int(point, 16), int(encoded, 16)) for point, encoded in
              re.findall(r"\{0x([0-9A-F]+), 0x([0-9A-F]+)\}", table)]
    assert actual == [(point, int.from_bytes(encoded, "big")) for point, encoded in entries]

    def literal(value):
        return '"' + ''.join(f"\\x{byte:02x}" for byte in value) + '"'

    results = {"mapping_sha256": expected_hash, "mapping_entries": len(entries), "negative_cases": []}
    with tempfile.TemporaryDirectory(prefix="petari-cp932-") as directory:
        temp = Path(directory)
        common = [args.cxx, "-std=c++23", "-I", str(root / "src"), "-Wall", "-Wextra", "-Werror"]
        second = temp / "second.cpp"
        second.write_text('#include "resource/TextEncoding.hpp"\nconst char* cp932_pointer_from_second_translation_unit() { return CP932("日本語"); }\n')
        binary = temp / "literals"
        subprocess.run(common + [str(root / "tests/Cp932LiteralTests.cpp"), str(second), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)

        complete = [(i, bytes([i])) for i in range(128)] + entries
        mapping_test = temp / "mapping.cpp"
        header = (root / "tests/Cp932LiteralTests.cpp").read_text().split("static_assert(", 1)[0]
        checks = [header]
        for start in range(0, len(complete), 64):
            batch = complete[start:start + 64]
            source = ''.join(chr(point) for point, _ in batch).encode("utf-8")
            expected = b''.join(encoded for _, encoded in batch)
            checks.append(f"static_assert(same_bytes(CP932({literal(source)}), {literal(expected)}));\n")
        mapping_test.write_text(''.join(checks))
        subprocess.run(common + ["-fsyntax-only", str(mapping_test)], check=True)

        negatives = {
            "unrepresentable_emoji": ('CP932("😀")', "not representable in CP932"),
            "unrepresentable_yen": ('CP932("¥")', "not representable in CP932"),
            "isolated_continuation": ('CP932("\\x80")', "invalid leading byte"),
            "raw_cp932_is_not_utf8": ('CP932("\\x82\\xa0")', "invalid leading byte"),
            "truncated_utf8": ('CP932("\\xe3\\x81")', "truncated sequence"),
            "bad_continuation": ('CP932("\\xe3" "A" "\\x81")', "invalid continuation byte"),
            "overlong_two_byte": ('CP932("\\xc0\\x80")', "invalid leading byte"),
            "overlong_three_byte": ('CP932("\\xe0\\x80\\x80")', "invalid scalar value"),
            "surrogate": ('CP932("\\xed\\xa0\\x80")', "invalid scalar value"),
            "out_of_unicode_range": ('CP932("\\xf4\\x90\\x80\\x80")', "invalid scalar value"),
            "invalid_final_byte": ('CP932("ok\\xff")', "invalid leading byte"),
            "missing_nul": ('CP932(unterminated)', "terminating NUL"),
        }
        for name, (expression, diagnostic) in negatives.items():
            path = temp / f"{name}.cpp"
            path.write_text('#include "resource/TextEncoding.hpp"\nconstexpr char unterminated[] = {65};\n'
                            f"constexpr const auto& result = {expression};\n")
            process = subprocess.run(common + ["-fsyntax-only", str(path)], capture_output=True, text=True)
            assert process.returncode != 0, f"{name} unexpectedly compiled"
            assert diagnostic in process.stderr, f"{name}: missing diagnostic {diagnostic}\n{process.stderr}"
            results["negative_cases"].append(name)
        results["status"] = "passed"
        results["compiled_codepoints"] = len(complete)
    if args.output:
        args.output.write_text(json.dumps(results, indent=2) + "\n")
    print(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()
