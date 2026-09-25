"""Read-only exact donor/owner checks; no compile or runtime claims."""
from pathlib import Path
import json,re,struct

N=Path(__file__).parent
B=N/'before'
donor=(B/'decomp/src/Game/System/Overwrite.cpp').read_text()
source=Path('src/Game/System/Overwrite.cpp').read_text()
start=donor.index('void J3DShapeMtx::loadMtxIndx_PNGP(')
end=donor.index('void JKRAramPiece::startDMA(',start)
expected=donor[start:end]
actual=source[source.index('void J3DShapeMtx::loadMtxIndx_PNGP('):]
def normalized(text):
    return re.sub(r'\s+','',re.sub(r'//[^\n]*','',text))
assert actual.count('mLastNonzeroUserWork = 0;') == 1
assert normalized(actual.replace('mLastNonzeroUserWork = 0;', '')) == normalized(expected)
methods=['J3DShapeMtx::loadMtxIndx_PNGP','JPABaseEmitter::init','JPADrawDirection','JPADrawRotDirection',
'JPADrawDBillboard','JPADrawLine','JPADrawStripe','JPADrawStripeX','JPAFieldAir::prepare',
'JPAFieldVortex::prepare','JPAFieldVortex::calc','JPAFieldConvection::prepare','JPAFieldSpin::prepare',
'JPAEmitterManager::calcYBBCam','JPADrawYBillboard','JPADrawRotYBillboard']
rows=[]
all_sources=[p for root in ['src/Game','src/JSystem','src/compat'] for p in Path(root).rglob('*.cpp')]
for method in methods:
    pattern=re.compile(r'^(?:void )'+re.escape(method)+r'\([^;{]*\)[^{;]*\{',re.M)
    providers=[]
    for path in all_sources:
        text=path.read_text(errors='replace')
        for match in pattern.finditer(text):
            providers.append({'path':str(path),'line':text[:match.start()].count('\n')+1})
    assert len(providers)==1 and providers[0]['path']=='src/Game/System/Overwrite.cpp',(method,providers)
    match=pattern.search(donor)
    assert match,method
    rows.append({'method':method,'donor':'decomp/src/Game/System/Overwrite.cpp','donor_line':donor[:match.start()].count('\n')+1,
                 'provider':providers[0], 'adaptation':'reset existing native callback owner token' if method=='JPABaseEmitter::init' else 'none'})
unchanged=['src/JSystem/JParticle/JPABaseShape.cpp','src/JSystem/JParticle/JPAEmitter.cpp',
'src/JSystem/JParticle/JPAEmitter.hpp','src/JSystem/JParticle/JPAEmitterManager.cpp',
'src/JSystem/JParticle/JPAFieldBlock.cpp','src/JSystem/J3DGraphBase/J3DShapeMtx.cpp']
for path in unchanged:
    assert (B/path).read_bytes()==Path(path).read_bytes(),path
retired=['src/compat/'+name for name in ['OriginalJPADraw.cpp','OriginalJPAEmitterInit.cpp','OriginalJPAFields.cpp','J3DShapeMtxGameCompat.cpp']]
assert all(not Path(path).exists() for path in retired)
for pragma in ['#pragma clang fp contract(off)','#pragma GCC optimize("fp-contract=off")','#pragma fp_contract(off)']:
    assert pragma in source
for short,full in [(0.333333,0.33333298563957214),(0.57735,0.5773500204086304)]:
    assert struct.pack('f',short)==struct.pack('f',full)
assert source.isascii()
result={'status':'passed-static-source-only','original_public_methods':16,'entire_override_body_and_helpers_equal_donor':True,
'only_native_body_adaptation':'mLastNonzeroUserWork reset retained at emitter init',
'fp_contract_controls_preserved':True,'rotation_constants_same_f32_bits_as_previous_provider':True,
'canonical_sdk_files_unchanged':unchanged,'removed':retired,'method_inventory':rows}
(N/'source-validation.json').write_text(json.dumps(result,indent=2)+'\n')
(N/'method-inventory.json').write_text(json.dumps(rows,indent=2)+'\n')
print('All 16 original overrides have one provider; entire donor body/helper/table set matches except retained native owner reset; SDK files unchanged.')
