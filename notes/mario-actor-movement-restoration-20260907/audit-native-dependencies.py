import pathlib,json,re,subprocess,hashlib
root=pathlib.Path.cwd();p=root/'notes/mario-actor-movement-restoration-20260907';nm='/opt/homebrew/opt/llvm/bin/llvm-nm';cxx='/opt/homebrew/opt/llvm/bin/llvm-cxxfilt'
pat=re.compile(r'@(?:"([^"\n]+)"|([-a-zA-Z$._0-9]+))')
def refs(text):return {a or b for a,b in pat.findall(text)}
def irdefs(path):
 text=path.read_text();defs={}
 for match in re.finditer(r'^define [^\n]*\{\n.*?^\}',text,re.M|re.S):
  name=next(iter(pat.findall(match.group().splitlines()[0])));name=name[0] or name[1];defs[name]=refs(match.group())-{name}
 for line in text.splitlines():
  if line.startswith('@'):
   name=next(iter(pat.findall(line)));name=name[0] or name[1]
   if '= external ' not in line:defs[name]=refs(line)-{name}
 return defs
D=irdefs(p/'MarioActor.probe.ll'); B=irdefs(p/'MarioActor.baseline.ll');providers={};archives=[]
for path in sorted((root/'build/macosx/arm64/debug').glob('*.a')):
 archives.append(dict(path=str(path.relative_to(root)),mtime_ns=path.stat().st_mtime_ns))
 out=subprocess.check_output([nm,'--defined-only','--format=posix',str(path)],text=True)
 for line in out.splitlines():
  parts=line.split()
  if len(parts)>=4 and parts[1] in 'TWDSVBCtwdsb':providers.setdefault(parts[0].removeprefix('_'),[]).append(path.name)
keys=['_ZN10MarioActorC2EPKc','_ZN10MarioActor5init2ERKN9JGeometry5TVec3IfEES4_l','_ZN10MarioActor18initAfterPlacementEv','_ZN10MarioActor8movementEv','_ZN10MarioActor7controlEv']
keys=[k for k in D if (k.startswith('_ZN10MarioActorC2') or k.startswith('_ZN10MarioActor5init2') or k in keys[2:])]
def closure(defs,key):
 seen=set();todo=[key];ext=set()
 while todo:
  n=todo.pop()
  if n in seen:continue
  seen.add(n)
  if n in defs:todo.extend(defs[n])
  else:ext.add(n)
 return ext,seen
allnames=set(D)|set(providers)
for refs_ in D.values():allnames|=refs_
ordered=sorted(allnames);demangled=subprocess.check_output([cxx],input='\n'.join(ordered),text=True).splitlines();names=dict(zip(ordered,demangled))
def record(s):return dict(symbol=s,name=names.get(s,s),providers=providers.get(s,[]))
results=[]
for key in keys:
 ext,seen=closure(D,key);bext,_=closure(B,key)
 direct=D[key]
 missing={s for s in direct if s not in D and s not in providers and s.startswith('_Z') and not s.startswith(('_ZSt','_ZNSt','_ZT'))}
 results.append(dict(root=key,name=names[key],direct_dependencies=[record(s) for s in sorted(direct) if not s.startswith(('.','llvm.'))],direct_project_gaps=[record(s) for s in sorted(missing)],new_external_project_dependencies=[record(s) for s in sorted(ext-bext) if s.startswith('_Z') and not s.startswith(('_ZSt','_ZNSt','_ZT'))],local_graph_external_count=len(ext)))
(p/'native-provider-closure.json').write_text(json.dumps(dict(scope='LLVM IR references from five restored methods; local function/global graph only. Archive availability snapshot does not prove transitive archive closure, initialization or runtime correctness.',archives=archives,methods=results),indent=2)+'\n')
for r in results:
 print(r['name']);print('direct gaps:',[x['name'] for x in r['direct_project_gaps']]);print('new local-closure project gaps:',[x['name'] for x in r['new_external_project_dependencies'] if not x['providers']])
sp=json.loads((p/'source-probe.json').read_text());sp['probe_sha256']=hashlib.sha256((p/'probe/MarioActor.cpp').read_bytes()).hexdigest();sp['native_adaptations']=['Reference _1D8/_1DC mapped to existing native mRasterBuffers[0]/[1]'];(p/'source-probe.json').write_text(json.dumps(sp,indent=2)+'\n')
