#!/usr/bin/env python3
"""Check reviewed native Exp edits, exact original scope methods and frozen sources."""
import hashlib,json,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
NOTES=Path(__file__).resolve().parent
root=(ROOT/'src/JSystem/JKernel/JKRExpHeap.cpp').read_text()
expected=root
for edit in reversed(json.loads((NOTES/'native-exp-edits.json').read_text())):
 a,b=edit['root_start'],edit['root_end']
 assert expected[a:b]==edit['before']
 expected=expected[:a]+edit['after']+expected[b:]
assert expected==(ROOT/'pc-port/src/compat/JKRExpHeapCompat.cpp').read_text()
assert (ROOT/'libs/JSystem/include/JSystem/JKernel/JKRExpHeap.hpp').read_bytes()==(ROOT/'pc-port/src/JSystem/JKernel/JKRExpHeap.hpp').read_bytes()
assert (ROOT/'include/Game/Util/MutexHolder.hpp').read_bytes()==(ROOT/'pc-port/src/Game/Util/MutexHolder.hpp').read_bytes()
def body(text,signature):
 start=text.index(signature);at=text.index('{',start)+1;end=at;depth=1
 while depth:
  depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return re.sub(r'\s+','',text[start:end])
original=(ROOT/'src/Game/Util/MemoryUtil.cpp').read_text()
native=(ROOT/'pc-port/src/compat/MemoryHeapScopeCompat.cpp').read_text()
for signature in ['CurrentHeapRestorer::CurrentHeapRestorer(','CurrentHeapRestorer::~CurrentHeapRestorer(',
                  'JKRHeap* getCurrentHeap(', 'void becomeCurrentHeap(', 'bool isEqualCurrentHeap(']:
 assert body(original,signature)==body(native,signature),signature
manifest=json.loads((NOTES/'native-evidence.json').read_text())
for path,expected_sha in manifest['source_sha256'].items():
 assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==expected_sha,path
for suffix in ['', '-asan','-tsan']:
 text=(NOTES/('native-tests'+suffix+'.log')).read_text()
 assert '[pass] 7 retained JKR allocation domain groups' in text
 assert not any(value in text for value in ['runtime error:', 'ERROR: AddressSanitizer', 'WARNING: ThreadSanitizer'])
print('Verified native architecture edits, 5 original heap-scope methods, source hashes and 3 clean test variants')
