#!/usr/bin/env python3
"""Check the reviewed source boundary; source hashes are not binary matches."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent
report=json.loads((HERE/'source-correspondence.json').read_text())
for name,expected in report['sha256'].items():
 assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==expected,name
source=(ROOT/'pc-port/src/resource/J3dMaterialTableData.cpp').read_text()
assert 'reinterpret_cast<J3DModelLoader' not in source
assert '0x4C' in source and '0x51100000U' in source and '0x50100000U' in source
assert 'factory.create(existing, type, index, flags)' in source
print(f"[pass] {len(report['sha256'])} reviewed material-table source hashes")
