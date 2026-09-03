from pathlib import Path
import json,shutil,subprocess,hashlib,difflib
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-particle-draw-executor-20260903';S=B/'staged'
owned=['src/Game/Effect/ParticleDrawExecutor.cpp','include/Game/Effect/ParticleDrawExecutor.hpp','include/Game/Util/Functor.hpp','libs/JSystem/include/JSystem/JParticle/JPADrawInfo.hpp']
headers=['include/Game/Effect/EffectSystem.hpp','include/Game/NameObj/NameObjAdaptor.hpp','src/Game/NameObj/NameObjAdaptor.cpp']
manifest=[];patch=[]
for name in owned+headers:
 src=R/name;relative=Path(*Path(name).parts[3:]) if name.startswith('libs/') else Path(*Path(name).parts[1:]);dst=S/relative;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,dst)
 manifest.append({'source':name,'staged':str(dst.relative_to(R)),'owned':name in owned,'sha256':hashlib.sha256(src.read_bytes()).hexdigest()})
 if name in owned:
  before=subprocess.run(['git','show','HEAD:'+name],cwd=R,capture_output=True,text=True);old=before.stdout if before.returncode==0 else '';new=src.read_text();patch.append(''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),'a/'+name if before.returncode==0 else '/dev/null','b/'+name)))
  out=N/'root'/name;out.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,out)
(N/'root.patch').write_text(''.join(patch));(N/'native-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
# Preserve the established native FunctorBase destructor ABI while completing
# the original callable families used by draw registration.
functor=(R/'pc-port/src/Game/Util/Functor.hpp').read_text().replace('class JKRHeap;', '#include <JSystem/JKernel/JKRHeap.hpp>')
functor=functor.replace('FunctorBase* clone(JKRHeap*) const override {\n            return new FunctorV0M(*this);', 'FunctorBase* clone(JKRHeap* pHeap) const override {\n            return new (pHeap, 0) FunctorV0M(*this);')
addition="""
    template < class T >
    inline static FunctorV0M< const T*, void (T::*)() const > Functor(const T* caller, void (T::*callee)() const) {
        return FunctorV0M< const T*, void (T::*)() const >(caller, callee);
    }

    class FunctorV0F : public FunctorBase {
    public:
        explicit FunctorV0F(void (*func)()) : mFunc(func) {}
        void operator()() const override { (*mFunc)(); }
        FunctorBase* clone(JKRHeap* heap) const override { return new (heap, 0) FunctorV0F(*this); }
        void (*mFunc)();
    };

    inline static FunctorV0F Functor(void (*func)()) { return FunctorV0F(func); }
    inline static FunctorV0F Functor_Inline(void (*func)()) { return FunctorV0F(func); }
"""
functor=functor.replace('}  // namespace MR',addition+'}  // namespace MR')
objutil=(R/'pc-port/src/Game/Util/ObjUtil.hpp').read_text().replace('namespace MR {','namespace MR {\n    class FunctorBase;\n    void registerPreDrawFunction(const MR::FunctorBase&, int);',1)
patch=[]
for rel,new in [('Game/Util/Functor.hpp',functor),('Game/Util/ObjUtil.hpp',objutil)]:
 dst=S/rel;dst.parent.mkdir(parents=True,exist_ok=True);dst.write_text(new)
 dst=N/'native'/rel;dst.parent.mkdir(parents=True,exist_ok=True);dst.write_text(new)
 old=(R/'pc-port/src'/rel).read_text();patch.append(''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),'a/pc-port/src/'+rel,'b/pc-port/src/'+rel)))
(N/'native-headers.patch').write_text(''.join(patch))
cmd=json.loads((R/'pc-port/notes/original-jpa-resource-loader-20260903/native-compiles.json').read_text())[0]['command'][:-4]
cmd=[a.replace(str(R/'build/original-jpa-resource-loader-20260903/staged'),str(S)) for a in cmd]
rows=[]
for name in ['ParticleDrawExecutor','NameObjAdaptor']:
 src=S/('Game/NameObj' if name=='NameObjAdaptor' else 'Game/Effect')/(name+'.cpp');c=cmd+['-c',str(src),'-o',str(B/(name+'-native.o'))];p=subprocess.run(c,cwd=R/'pc-port',capture_output=True,text=True);(B/(name+'-native.log')).write_text(p.stdout+p.stderr);rows.append({'command':c,'returncode':p.returncode});print(name,p.returncode)
 if p.returncode:print(p.stdout,p.stderr);p.check_returncode()
 c=['/opt/homebrew/opt/llvm/bin/llvm-nm','-u','--demangle',str(B/(name+'-native.o'))];p=subprocess.run(c,capture_output=True,text=True);(N/(name+'-undefined.txt')).write_text(p.stdout)
(N/'native-compiles.json').write_text(json.dumps(rows,indent=2)+'\n')

for row in manifest:
 row['source_sha256']=row.pop('sha256')
 row['staged_sha256']=hashlib.sha256((R/row['staged']).read_bytes()).hexdigest()
for rel in ['Game/Effect/ParticleDrawExecutor.cpp','Game/Effect/ParticleDrawExecutor.hpp','Game/NameObj/NameObjAdaptor.cpp','Game/NameObj/NameObjAdaptor.hpp','JSystem/JParticle/JPADrawInfo.hpp']:
 src=S/rel;dst=N/'native'/rel;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,dst)
(N/'native-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
