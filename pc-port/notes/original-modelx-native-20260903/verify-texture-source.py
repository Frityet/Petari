#!/usr/bin/env python3
"""Check the owned constructor's declared native allocation-only adaptation."""
import importlib.util
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
spec = importlib.util.spec_from_file_location('modelx', HERE / 'verify-source.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
signature = 'JUTTexture::JUTTexture(int width, int height, GXTexFmt format)'
original = module.body((ROOT / 'src/JSystem/JUtility/JUTTexture.cpp').read_text(), signature)
native = module.body((ROOT / 'pc-port/src/runtime/jut/JutTexture.cpp').read_text(), signature)
old = 'ResTIMG* texBuf = reinterpret_cast< ResTIMG* >(new (sizeof(ResTIMG)) u8[bufSize + sizeof(ResTIMG)]);'
replacement = '''auto allocation = smgpc::compat::allocate_owned_jut_texture(*this, static_cast<std::size_t>(bufSize) + sizeof(ResTIMG));
    ResTIMG* texBuf = static_cast<ResTIMG*>(allocation.data());'''
assert old in original and replacement in native
native = native.replace(replacement, old).replace('    allocation.commit();\n', '')
assert module.tokens(original) == module.tokens(native)
header = (ROOT / 'pc-port/src/JSystem/JUtility/JUTTexture.hpp').read_text()
assert 'u8 mFlag = 0U;' in header
print('PASS: original owned JUTTexture constructor statements preserved except explicit mapped allocation/commit; mFlag initialized')
