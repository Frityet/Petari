"""One-shot restoration of the original SMG JPA/shape override set.

Historical mutation recipe, not a build or validation tool.
"""
from pathlib import Path

N = Path(__file__).parent
B = N / 'before'
donor = (B / 'decomp/src/Game/System/Overwrite.cpp').read_text()
start = donor.index('void J3DShapeMtx::loadMtxIndx_PNGP(')
end = donor.index('void JKRAramPiece::startDMA(', start)
body = donor[start:end].rstrip()
assert body.count('    mpUserWork = 0;') == 1
body = body.replace('    mpUserWork = 0;', '''    mpUserWork = 0;
    // Native callback retirement tracks an emitter's last borrowed owner.
    mLastNonzeroUserWork = 0;''')
source = '''#include "Game/System/ShapePacketUserData.hpp"
#include "Game/Util/MathUtil.hpp"

#include <JSystem/J3DGraphBase/J3DFifo.hpp>
#include <JSystem/J3DGraphBase/J3DPacket.hpp>
#include <JSystem/J3DGraphBase/J3DShape.hpp>
#include <JSystem/J3DGraphBase/J3DShapeMtx.hpp>
#include <JSystem/J3DGraphBase/J3DSys.hpp>
#include <JSystem/JMath/JMATrigonometric.hpp>
#include <JSystem/JParticle/JPABaseShape.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <JSystem/JParticle/JPAEmitterManager.hpp>
#include <JSystem/JParticle/JPAFieldBlock.hpp>
#include <JSystem/JParticle/JPAParticle.hpp>
#include <revolution/gx/GXVert.h>

// Preserve the original override unit's separate scalar arithmetic operations.
#if defined(__clang__)
#pragma clang fp contract(off)
#elif defined(__GNUC__)
#pragma GCC optimize("fp-contract=off")
#elif defined(_MSC_VER)
#pragma fp_contract(off)
#endif

''' + body + '\n'
path = Path('src/Game/System/Overwrite.cpp')
assert not path.exists()
path.write_text(source)
for name in ['OriginalJPADraw.cpp', 'OriginalJPAEmitterInit.cpp', 'OriginalJPAFields.cpp', 'J3DShapeMtxGameCompat.cpp']:
    path = Path('src/compat') / name
    assert path.read_bytes() == (B / path).read_bytes()
    path.unlink()
print('Restored the complete 16-method original JPA/shape override set and removed four compat providers.')
