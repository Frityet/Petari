import json, re, subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2]
work=Path(__file__).resolve().parent
checkpoint='c3d4a5901'
baseline=subprocess.check_output(['git','show',checkpoint+':src/Game/LiveActor/Nerve.hpp'],cwd=root,text=True)
current=(root/'src/Game/LiveActor/Nerve.hpp').read_text()
instantiate='''
NERVE(One)
NERVE_EXECEND(Two)
NERVE_DECL(Three, Actor, run)
NERVE_DECL_EXE(Four, Actor, Wait)
NERVE_DECL_ONEND(Five, Actor, run, end)
NERVE_DECL_NULL(Six)
INIT_NERVE(One)
INIT_NERVE_NEW(Two, Actor, run)
NEW_NERVE(Seven, Actor, Wait)
NEW_NERVE_ONEND(Eight, Actor, Wait, Wait)
'''
results={}
for name,content in [('before',baseline),('after',current)]:
    content=content.replace('#pragma once','').replace('#include "Game/LiveActor/Spine.hpp"','class Spine;')
    source=work/('retail-macros-'+name+'.cpp')
    source.write_text('#define NO_INLINE __attribute__((noinline))\n'+content+instantiate)
    args=['/opt/homebrew/opt/llvm/bin/clang++','-E','-P','-x','c++',str(source)]
    text=subprocess.check_output(args,cwd=root,text=True)
    (work/('retail-macros-'+name+'.i')).write_text(text)
    results[name]=re.findall(r'\w+|[^\s\w]',text)
assert results['before']==results['after']
(work/'retail-macro-proof.json').write_text(json.dumps({'baseline':checkpoint,'equal_preprocessed_tokens':True,'token_count':len(results['before'])},indent=2)+'\n')
print('PASS: non-TARGET_PC Nerve macro expansion is token-identical to '+checkpoint)
