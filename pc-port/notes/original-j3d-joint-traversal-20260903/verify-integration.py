#!/usr/bin/env python3
"""Check the additional original joint-tree bodies imported by the renderer."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[3]


def function(path, signature):
    text = (ROOT / path).read_text()
    start = text.index(signature)
    brace = text.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return re.sub(r'\s+', '', text[start:end])


provider = 'pc-port/src/compat/J3DJointTreeCompat.cpp'
groups = {
    'src/JSystem/J3DGraphAnimator/J3DJointTree.cpp': [
        'J3DJointTree::J3DJointTree()',
        'void J3DJointTree::calc(',
        'void J3DMtxCalc::setMtxBuffer(',
        'J3DJointTree::~J3DJointTree()',
    ],
    'src/JSystem/J3DGraphAnimator/J3DMtxBuffer.cpp': ['void J3DMtxBuffer::initialize()'],
    'src/JSystem/J3DGraphBase/J3DVertex.cpp': [
        'J3DDrawMtxData::J3DDrawMtxData()',
        'J3DDrawMtxData::~J3DDrawMtxData()',
    ],
}
for source, signatures in groups.items():
    for signature in signatures:
        assert function(source, signature) == function(provider, signature), signature
        print(f'Original body: {signature}')

header = 'JSystem/J3DGraphAnimator/J3DJointTree.hpp'
assert (ROOT / 'libs/JSystem/include' / header).read_bytes() == (
    ROOT / 'pc-port/src' / header).read_bytes()
print('Original JointTree header: byte identical')
