#!/usr/bin/env python3
"""Check the four reviewed root bodies without requiring native compilation."""

import hashlib
import importlib.util
import json
from pathlib import Path


HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("entrypoints", HERE / "entrypoints-verify-object.py")
verifier = importlib.util.module_from_spec(spec)
spec.loader.exec_module(verifier)


def main():
    evidence = json.loads((HERE / "entrypoints-compiler-evidence.json").read_text())
    source = verifier.SOURCE.read_text()
    for function in evidence["functions"]:
        digest = hashlib.sha256(verifier.body(source, function["method"]).encode()).hexdigest()
        assert digest == function["source_body_sha256"], function["method"]
        print(function["method"], digest)
    print("Four reviewed root XanimeCore bodies agree with the original-compiler evidence.")


if __name__ == "__main__":
    main()
